#include <cassert>
#include <glm/detail/qualifier.hpp>
#include <string>
#include <array>
#include <vulkan/vulkan_core.h>

#include "blas.hpp"
#include "common_types.h"
#include "deletion_queue.hpp"
#include "raytracing.hpp"
#include "raytracing_scene.hpp"
#include "raytracing_worldobject.hpp"
#include "shader_module.hpp"
#include "tetrahedron.hpp"
#include "visualizations.hpp"
#include "device_procedures.hpp"
#include "vk_utils.hpp"

namespace ltracer
{
namespace rt
{

VkPhysicalDeviceRayTracingPipelineFeaturesKHR getRaytracingPipelineFeatures()
{
	// =========================================================================
	// Physical Device Features

	VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	    .pNext = NULL,
	    .bufferDeviceAddress = VK_TRUE,
	    .bufferDeviceAddressCaptureReplay = VK_FALSE,
	    .bufferDeviceAddressMultiDevice = VK_FALSE,
	};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
	    .pNext = &physicalDeviceBufferDeviceAddressFeatures,
	    .accelerationStructure = VK_TRUE,
	    .accelerationStructureCaptureReplay = VK_FALSE,
	    .accelerationStructureIndirectBuild = VK_FALSE,
	    .accelerationStructureHostCommands = VK_FALSE,
	    .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE,
	};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
	    .pNext = &physicalDeviceAccelerationStructureFeatures,
	    .rayTracingPipeline = VK_TRUE,
	    .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
	    .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
	    .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
	    .rayTraversalPrimitiveCulling = VK_FALSE,
	};

	// NOTE: is hardcoded into createLogicalDevice
	// VkPhysicalDeviceFeatures deviceFeatures{
	//    .geometryShader = VK_TRUE,
	// };

	return physicalDeviceRayTracingPipelineFeatures;
}

VkCommandPool createCommandPool(VkDevice logicalDevice,
                                DeletionQueue& deletionQueue,
                                RaytracingInfo& raytracingInfo)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
	};

	VkCommandPool commandBufferPoolHandle = VK_NULL_HANDLE;
	VkResult result = vkCreateCommandPool(
	    logicalDevice, &commandPoolCreateInfo, NULL, &commandBufferPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateCommandPool failed");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyCommandPool(logicalDevice, commandBufferPoolHandle, nullptr); });

	return commandBufferPoolHandle;
}

void requestRaytracingProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	physicalDeviceRayTracingPipelineProperties.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	physicalDeviceRayTracingPipelineProperties.pNext = NULL;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	    .pNext = &physicalDeviceRayTracingPipelineProperties,
	    .properties = physicalDeviceProperties,
	};

	// TODO: The VkPhysicalDeviceRayTracingPipelinePropertiesKHR object is passed in via pNext and
	// gets filled.
	// Because we are only interested in the VkPhysicalDeviceRayTracingPipelinePropertiesKHR
	// object it is fine that the VkPhysicalDeviceProperties2 object gets deleted when this
	// function scope ends.
	// However, maybe we wanna instead pass in the VkPhysicalDeviceProperties2
	// object as a reference as well and fill it, for now though its not needed
	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);
}

void createCommandBufferBuildTopAndBottomLevel(VkDevice logicalDevice,
                                               DeletionQueue& deletionQueue,
                                               RaytracingInfo& raytracingInfo,
                                               VkCommandPool& commandBufferPoolHandle)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = NULL,
	    .commandPool = commandBufferPoolHandle,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1,
	};

	// TODO: make this function return the command buffer instead and store it inside the
	// raytracingInfo object outside of this function
	VkResult result = vkAllocateCommandBuffers(logicalDevice,
	                                           &commandBufferAllocateInfo,
	                                           &raytracingInfo.commandBufferBuildTopAndBottomLevel);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateCommandBuffers failed");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    {
		    vkFreeCommandBuffers(logicalDevice,
		                         commandBufferPoolHandle,
		                         1,
		                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
	    });
}

VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, DeletionQueue& deletionQueue)
{
	VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
	    {.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 4},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = 2,
	    .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizeList.size()),
	    .pPoolSizes = descriptorPoolSizeList.data(),
	};

	VkResult result = vkCreateDescriptorPool(
	    logicalDevice, &descriptorPoolCreateInfo, NULL, &descriptorPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorPool");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorPool(logicalDevice, descriptorPoolHandle, NULL); });

	return descriptorPoolHandle;
}

void allocateDescriptorSetLayouts(VkDevice logicalDevice,
                                  VkDescriptorPool& descriptorPoolHandle,
                                  RaytracingInfo& raytracingInfo,
                                  std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = NULL,
	    .descriptorPool = descriptorPoolHandle,
	    .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	};

	VkResult result = vkAllocateDescriptorSets(
	    logicalDevice, &descriptorSetAllocateInfo, raytracingInfo.descriptorSetHandleList.data());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateDescriptorSets");
	}
}

void createPipelineLayout(VkDevice logicalDevice,
                          DeletionQueue& deletionQueue,
                          RaytracingInfo& raytracingInfo,
                          const std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList)
{
	std::vector<VkPushConstantRange> pushConstantRanges{
	    {
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	                      | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .offset = 0,
	        .size = sizeof(RaytracingDataConstants),
	    },
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	    .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
	    .pPushConstantRanges = pushConstantRanges.data(),
	};

	// TODO: consider returning the pipeline layout handle instead of storing it in the
	// raytracingInfo directly and assigning it to the raytracingInfo object outside of this
	// function
	VkResult result = vkCreatePipelineLayout(
	    logicalDevice, &pipelineLayoutCreateInfo, NULL, &raytracingInfo.pipelineLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreatePipelineLayout");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    { vkDestroyPipelineLayout(logicalDevice, raytracingInfo.pipelineLayoutHandle, NULL); });
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice,
                                                DeletionQueue& deletionQueue)

{
	VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 2,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 3,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 4,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 5,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 6,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 7,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 8,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 9,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 10,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	};

	std::vector<VkDescriptorBindingFlags> bindingFlags = std::vector<VkDescriptorBindingFlags>(
	    descriptorSetLayoutBindingList.size(), VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlags = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
	    .pNext = NULL,
	    .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
	    .pBindingFlags = bindingFlags.data(),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = &layoutBindingFlags,
	    .flags = 0,
	    .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindingList.size()),
	    .pBindings = descriptorSetLayoutBindingList.data(),
	};

	VkResult result = vkCreateDescriptorSetLayout(
	    logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorSetLayout");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayoutHandle, NULL); });

	return descriptorSetLayoutHandle;
}

VkDescriptorSetLayout createMaterialDescriptorSetLayout(VkDevice logicalDevice,
                                                        DeletionQueue& deletionQueue)
{
	VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> materialDescriptorSetLayoutBindingList = {
	    {.binding = 0,
	     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	     .descriptorCount = 1,
	     .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	     .pImmutableSamplers = NULL},
	    {.binding = 1,
	     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	     .descriptorCount = 1,
	     .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	     .pImmutableSamplers = NULL},
	};

	VkDescriptorSetLayoutCreateInfo materialDescriptorSetLayoutCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	       .pNext = NULL,
	       .flags = 0,
	       .bindingCount = (uint32_t)materialDescriptorSetLayoutBindingList.size(),
	       .pBindings = materialDescriptorSetLayoutBindingList.data()};

	VkResult result = vkCreateDescriptorSetLayout(logicalDevice,
	                                              &materialDescriptorSetLayoutCreateInfo,
	                                              NULL,
	                                              &materialDescriptorSetLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorSetLayout");
	}

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyDescriptorSetLayout(logicalDevice, materialDescriptorSetLayoutHandle, NULL); });

	return materialDescriptorSetLayoutHandle;
}

void loadShaderModules(VkDevice logicalDevice,
                       DeletionQueue& deletionQueue,
                       RaytracingInfo& raytracingInfo)
{

	// =========================================================================
	// Ray Closest Hit Shader Module
	ltracer::shader::createShaderModule("shaders/shader.rchit.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayClosestHitShaderModuleHandle);

	// =========================================================================
	// Ray Generate Shader Module
	ltracer::shader::createShaderModule("shaders/shader.rgen.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayGenerateShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module

	ltracer::shader::createShaderModule("shaders/shader.rmiss.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayMissShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module (Shadow)

	ltracer::shader::createShaderModule("shaders/shader_shadow.rmiss.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayMissShadowShaderModuleHandle);

	// =========================================================================
	// Ray AABB Intersection Module
	ltracer::shader::createShaderModule("shaders/shader_aabb.rint.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayAABBIntersectionModuleHandle);
}

void createRaytracingPipeline(VkDevice logicalDevice,
                              DeletionQueue& deletionQueue,
                              RaytracingInfo& raytracingInfo)
{
	enum shaderIndices
	{
		ClosestHit = 0,
		RayGen = 1,
		RayMiss = 2,
		RayMissShadow = 3,
		RayAABBIntersection = 4,
		ShaderStageCount
	};

	std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfoList;
	pipelineShaderStageCreateInfoList.resize(ShaderStageCount);

	pipelineShaderStageCreateInfoList[shaderIndices::ClosestHit] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	    .module = raytracingInfo.rayClosestHitShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayGen] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	    .module = raytracingInfo.rayGenerateShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayMiss] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	    .module = raytracingInfo.rayMissShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayMissShadow] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	    .module = raytracingInfo.rayMissShadowShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayAABBIntersection] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	    .module = raytracingInfo.rayAABBIntersectionModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rayTracingShaderGroupCreateInfoList;

	raytracingInfo.shaderGroupCount = 4;
	rayTracingShaderGroupCreateInfoList.resize(raytracingInfo.shaderGroupCount);

	// Group 1
	raytracingInfo.hitGroupSize = 1;
	raytracingInfo.hitGroupOffset = 0;
	// Ray Closest Hit & Ray Intersection
	rayTracingShaderGroupCreateInfoList[raytracingInfo.hitGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
	    .generalShader = VK_SHADER_UNUSED_KHR,
	    .closestHitShader = shaderIndices::ClosestHit,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = shaderIndices::RayAABBIntersection,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 2
	raytracingInfo.rayGenGroupSize = 1;
	raytracingInfo.rayGenGroupOffset = raytracingInfo.hitGroupOffset + raytracingInfo.hitGroupSize;
	// Ray Gen
	rayTracingShaderGroupCreateInfoList[raytracingInfo.rayGenGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayGen,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 3
	raytracingInfo.missGroupSize = 2;
	raytracingInfo.missGroupOffset
	    = raytracingInfo.rayGenGroupOffset + raytracingInfo.rayGenGroupSize;
	// Ray Miss
	rayTracingShaderGroupCreateInfoList[raytracingInfo.missGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayMiss,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 4
	// Ray Miss Shadow
	rayTracingShaderGroupCreateInfoList[raytracingInfo.missGroupOffset + 1] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayMissShadow,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	//==========================================================================
	// Create Ray Tracing Pipeline
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .flags = 0,
	    .stageCount = static_cast<uint32_t>(pipelineShaderStageCreateInfoList.size()),
	    .pStages = pipelineShaderStageCreateInfoList.data(),
	    .groupCount = static_cast<uint32_t>(rayTracingShaderGroupCreateInfoList.size()),
	    .pGroups = rayTracingShaderGroupCreateInfoList.data(),
	    .maxPipelineRayRecursionDepth = 1,
	    .pLibraryInfo = NULL,
	    .pLibraryInterface = NULL,
	    .pDynamicState = NULL,
	    .layout = raytracingInfo.pipelineLayoutHandle,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = 0,
	};

	VkResult result = ltracer::procedures::pvkCreateRayTracingPipelinesKHR(
	    logicalDevice,
	    VK_NULL_HANDLE,
	    VK_NULL_HANDLE,
	    1,
	    &rayTracingPipelineCreateInfo,
	    NULL,
	    &raytracingInfo.rayTracingPipelineHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateRayTracingPipelinesKHR");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    { vkDestroyPipeline(logicalDevice, raytracingInfo.rayTracingPipelineHandle, NULL); });
}

void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                              const RaytracingScene& raytracingScene,
                                              RaytracingInfo& raytracingInfo)
{
	VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptorInfo = {
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
	    .pNext = NULL,
	    .accelerationStructureCount = 1,
	    .pAccelerationStructures = &raytracingInfo.topLevelAccelerationStructureHandle,
	};

	VkDescriptorBufferInfo uniformDescriptorInfo = {
	    .buffer = raytracingInfo.uniformBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo gpuObjectsDescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().gpuObjectsBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo spheresDescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().spheresBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo tetrahedronsDescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().tetrahedronsBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo rectangularBezierSurfaces2x2DescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().rectangularBezierSurfaces2x2BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo slicingPlanesDescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().slicingPlanesBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo bezierTriangles2DescriptorInfo = {
	    .buffer = raytracingScene.getObjectBuffers().bezierTriangles2BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	std::vector<VkWriteDescriptorSet> writeDescriptorSetList;

	// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
	// descriptor set, for now we are using the same vertex and index buffer
	// VkDescriptorBufferInfo indexDescriptorInfo = {};
	// VkDescriptorBufferInfo vertexDescriptorInfo = {};

	// TODO: put meshObjects back
	// if (meshObjects.size() > 0)
	// {
	// 	indexDescriptorInfo.buffer = meshObjects[0].indexBufferHandle;
	// 	indexDescriptorInfo.offset = 0;
	// 	indexDescriptorInfo.range = VK_WHOLE_SIZE;

	// 	// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
	// 	// descriptor set, for now we are using the same vertex and index buffer
	// 	vertexDescriptorInfo.buffer = meshObjects[0].vertexBufferHandle;
	// 	vertexDescriptorInfo.offset = 0;
	// 	vertexDescriptorInfo.range = VK_WHOLE_SIZE;

	// 	writeDescriptorSetList.push_back({
	// 	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	    .pNext = NULL,
	// 	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	// 	    .dstBinding = 2,
	// 	    .dstArrayElement = 0,
	// 	    .descriptorCount = 1,
	// 	    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	// 	    .pImageInfo = NULL,
	// 	    .pBufferInfo = &indexDescriptorInfo,
	// 	    .pTexelBufferView = NULL,
	// 	});
	// 	writeDescriptorSetList.push_back({
	// 	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	    .pNext = NULL,
	// 	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	// 	    .dstBinding = 3,
	// 	    .dstArrayElement = 0,
	// 	    .descriptorCount = 1,
	// 	    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	// 	    .pImageInfo = NULL,
	// 	    .pBufferInfo = &vertexDescriptorInfo,
	// 	    .pTexelBufferView = NULL,
	// 	});
	// }

	VkDescriptorImageInfo rayTraceImageDescriptorInfo = {
	    .sampler = VK_NULL_HANDLE,
	    .imageView = raytracingInfo.rayTraceImageViewHandle,
	    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	if (raytracingInfo.topLevelAccelerationStructureHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = &accelerationStructureDescriptorInfo,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 0,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		    .pImageInfo = NULL,
		    .pBufferInfo = NULL,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingInfo.uniformBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 1,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &uniformDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingInfo.rayTraceImageViewHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 4,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		    .pImageInfo = &rayTraceImageDescriptorInfo,
		    .pBufferInfo = NULL,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().tetrahedronsBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 5,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &tetrahedronsDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().spheresBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 6,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &spheresDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().rectangularBezierSurfaces2x2BufferHandle
	    != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 7,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &rectangularBezierSurfaces2x2DescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().slicingPlanesBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 8,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &slicingPlanesDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().gpuObjectsBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 9,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &gpuObjectsDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.getObjectBuffers().bezierTriangles2BufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 10,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &bezierTriangles2DescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	vkUpdateDescriptorSets(logicalDevice,
	                       static_cast<uint32_t>(writeDescriptorSetList.size()),
	                       writeDescriptorSetList.data(),
	                       0,
	                       NULL);
}

// void loadAndCreateVertexAndIndexBufferForModel(VkDevice logicalDevice,
//                                                VkPhysicalDevice physicalDevice,
//                                                DeletionQueue& deletionQueue,
//                                                MeshObject& meshObject)
// {

// 	// =========================================================================
// 	// Vertex Buffer
// 	createBuffer(physicalDevice,
// 	             logicalDevice,
// 	             deletionQueue,
// 	             sizeof(float) * meshObject.vertices.size() * 3,
// 	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
// 	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
// 	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
// 	             memoryAllocateFlagsInfo,
// 	             meshObject.vertexBufferHandle,
// 	             meshObject.vertexBufferDeviceMemoryHandle);

// 	void* hostVertexMemoryBuffer;
// 	VkResult result = vkMapMemory(logicalDevice,
// 	                              meshObject.vertexBufferDeviceMemoryHandle,
// 	                              0,
// 	                              sizeof(float) * meshObject.vertices.size() * 3,
// 	                              0,
// 	                              &hostVertexMemoryBuffer);

// 	memcpy(hostVertexMemoryBuffer,
// 	       meshObject.vertices.data(),
// 	       sizeof(float) * meshObject.vertices.size() * 3);

// 	if (result != VK_SUCCESS)
// 	{
// 		throw new std::runtime_error("initRayTracing - vkMapMemory");
// 	}

// 	vkUnmapMemory(logicalDevice, meshObject.vertexBufferDeviceMemoryHandle);

// 	VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
// 	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
// 	       .pNext = NULL,
// 	       .buffer = meshObject.vertexBufferHandle};

// 	meshObject.vertexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
// 	    logicalDevice, &vertexBufferDeviceAddressInfo);

// 	// =========================================================================
// 	// Index Buffer

// 	createBuffer(physicalDevice,
// 	             logicalDevice,
// 	             deletionQueue,
// 	             sizeof(uint32_t) * meshObject.indices.size(),
// 	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
// 	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
// 	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
// 	             memoryAllocateFlagsInfo,
// 	             meshObject.indexBufferHandle,
// 	             meshObject.indexBufferDeviceMemoryHandle);

// 	void* hostIndexMemoryBuffer;
// 	result = vkMapMemory(logicalDevice,
// 	                     meshObject.indexBufferDeviceMemoryHandle,
// 	                     0,
// 	                     sizeof(uint32_t) * meshObject.indices.size(),
// 	                     0,
// 	                     &hostIndexMemoryBuffer);

// 	memcpy(hostIndexMemoryBuffer,
// 	       meshObject.indices.data(),
// 	       sizeof(uint32_t) * meshObject.indices.size());

// 	if (result != VK_SUCCESS)
// 	{
// 		throw new std::runtime_error("initRayTracing - vkMapMemory");
// 	}

// 	vkUnmapMemory(logicalDevice, meshObject.indexBufferDeviceMemoryHandle);

// 	VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
// 	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
// 	       .pNext = NULL,
// 	       .buffer = meshObject.indexBufferHandle};

// 	meshObject.indexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
// 	    logicalDevice, &indexBufferDeviceAddressInfo);
// }

/**
 * @brief [TODO:description]
 *
 * @param physicalDevice [TODO:parameter]
 * @param logicalDevice [TODO:parameter]
 * @param deletionQueue [TODO:parameter]
 * @param raytracingInfo [TODO:parameter]
 */
void initRayTracing(VkPhysicalDevice physicalDevice,
                    VkDevice logicalDevice,
                    DeletionQueue& deletionQueue,
                    RaytracingInfo& raytracingInfo,
                    RaytracingScene& raytracingScene)
{
	// Requesting ray tracing properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{};
	requestRaytracingProperties(physicalDevice, physicalDeviceRayTracingPipelineProperties);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	VkResult result;

	// std::vector<float> queuePrioritiesList = {1.0f};
	// VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
	//     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	//     .pNext = NULL,
	//     .flags = 0,
	//     .queueFamilyIndex =
	//         raytracingInfo.queueFamilyIndices.presentFamily.value(),
	//     .queueCount = 1,
	//     .pQueuePriorities = queuePrioritiesList.data(),
	// };

	// =========================================================================
	// Submission Queue
	if (raytracingInfo.queueFamilyIndices.graphicsFamily.has_value())
	{
		vkGetDeviceQueue(logicalDevice,
		                 raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
		                 0,
		                 &raytracingInfo.graphicsQueueHandle);
	}
	else
	{
		throw std::runtime_error("initRayTracing - vkGetDeviceQueue");
	}

	// =========================================================================
	// Device Pointer Functions
	ltracer::procedures::grabDeviceProcAddr(logicalDevice);

	// =========================================================================
	// Command Pool

	VkCommandPool commandBufferPoolHandle{
	    createCommandPool(logicalDevice, deletionQueue, raytracingInfo)};

	// =========================================================================
	// Command Buffers

	createCommandBufferBuildTopAndBottomLevel(
	    logicalDevice, deletionQueue, raytracingInfo, commandBufferPoolHandle);

	// =========================================================================
	// Surface Features
	// VkSurfaceCapabilitiesKHR surfaceCapabilities;
	// result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	//     physicalDevice, window->getVkSurface(), &surfaceCapabilities);

	// if (result != VK_SUCCESS) {
	//   throw new std::runtime_error(
	//       "initRayTracing - vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	// }

	// =========================================================================
	// Descriptor Pool

	VkDescriptorPool descriptorPoolHandle{createDescriptorPool(logicalDevice, deletionQueue)};

	// =========================================================================
	// Descriptor Set Layout

	VkDescriptorSetLayout descriptorSetLayoutHandle{
	    createDescriptorSetLayout(logicalDevice, deletionQueue)};

	// =========================================================================
	// Material Descriptor Set Layout

	VkDescriptorSetLayout materialDescriptorSetLayoutHandle{
	    createMaterialDescriptorSetLayout(logicalDevice, deletionQueue)};

	// =========================================================================
	// Allocate Descriptor Sets

	// TODO: currently we hardcoded that we use 2 descriptor set layouts, make this more dynamic or
	// at least dont hardcode it
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList
	    = {descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

	allocateDescriptorSetLayouts(
	    logicalDevice, descriptorPoolHandle, raytracingInfo, descriptorSetLayoutHandleList);

	// =========================================================================
	// Pipeline Layout
	createPipelineLayout(
	    logicalDevice, deletionQueue, raytracingInfo, descriptorSetLayoutHandleList);
	// =========================================================================
	// Shader Modules
	loadShaderModules(logicalDevice, deletionQueue, raytracingInfo);

	// =========================================================================
	// Ray Tracing Pipeline
	createRaytracingPipeline(logicalDevice, deletionQueue, raytracingInfo);

	// =========================================================================
	// load OBJ Models
	std::string scenePath = "3d-models/cube_scene.obj";
	{
		// MeshObject meshObject = MeshObject::loadFromPath(scenePath);
		// meshObject.translate(20, 0, 0);
		// raytracingInfo.meshObjects.push_back(std::move(meshObject));
	}
	{
		// MeshObject meshObject2 = MeshObject::loadFromPath(scenePath);
		// // TODO: figure out why this movement causes artifacts
		// meshObject2.translate(-5, 0, 0);
		// meshObject2.transform.scale = glm::vec3(1, 1, 1);
		// meshObjects.push_back(std::move(meshObject2));
	}

	// =========================================================================
	// create aabbs (temporary)

	// std::vector<Tetrahedron1> tetrahedrons = {
	//     // createTetrahedron1({
	//     //     glm::vec3(0.0f, 0.0f, 0.5f),
	//     //     glm::vec3(0.5f, 0.0f, -0.5f),
	//     //     glm::vec3(-0.5f, 0.0f, -0.5f),
	//     //     glm::vec3(0.0f, 0.5f, 0.0f),
	//     // }),
	// };

	// std::vector<Sphere> spheres{
	//     // show origin
	//     // Sphere{glm::vec3(), 0.15f, static_cast<int>(ColorIdx::t_black)},
	//     // Sphere{glm::vec3(1, 0, 0), 0.1f, static_cast<int>(ColorIdx::t_red)},
	//     // Sphere{glm::vec3(0, 0, 1), 0.1f, static_cast<int>(ColorIdx::t_green)},
	//     // Sphere{glm::vec3(0, 1, 0), 0.1f, static_cast<int>(ColorIdx::t_blue)},
	// };

	// {
	// 	std::random_device rd;
	// 	std::mt19937 gen(rd());

	// 	std::uniform_int_distribution<> positionDist(-20, 20);
	// 	std::uniform_int_distribution<> sizeDist(-20, 20);
	// 	for (int i = 0; i < 20; i++)
	// 	{
	// 		auto randomX = positionDist(gen);
	// 		auto randomY = positionDist(gen);
	// 		auto randomZ = positionDist(gen);
	// 		if (false)
	// 		{
	// 			tetrahedrons.emplace_back(
	// 			    glm::vec3(randomX, randomY, randomZ),
	// 			    glm::vec3(
	// 			        randomX + sizeDist(gen), randomY + sizeDist(gen), randomZ + sizeDist(gen)),
	// 			    glm::vec3(
	// 			        randomX + sizeDist(gen), randomY + sizeDist(gen), randomZ + sizeDist(gen)));
	// 		}
	// 		else
	// 		{
	// 			float sphereScaling = 0.1f;
	// 			spheres.emplace_back(glm::vec3(randomX, randomY, randomZ),
	// 			                     static_cast<float>(std::abs(sizeDist(gen))) * sphereScaling);
	// 		}
	// 	}
	// }

	// auto surfaceTest = RectangularBezierSurface(3,
	//                                             3,
	//                                             {
	//                                                 glm::vec3(0, 0, 0),
	//                                                 glm::vec3(5, 0, 0),
	//                                                 glm::vec3(10, 0, 0),
	//                                                 glm::vec3(15, 0, 0),

	//                                                 glm::vec3(0, 0, 5),
	//                                                 glm::vec3(5, 0, 5),
	//                                                 glm::vec3(10, 0, 5),
	//                                                 glm::vec3(15, 0, 5),

	//                                                 glm::vec3(0, 0, 10),
	//                                                 glm::vec3(5, 0, 10),
	//                                                 glm::vec3(10, 0, 10),
	//                                                 glm::vec3(15, 0, 10),

	//                                                 glm::vec3(0, 5, 15),
	//                                                 glm::vec3(5, 5, 15),
	//                                                 glm::vec3(10, 5, 15),
	//                                                 glm::vec3(15, 5, 15),
	//                                             });

	// std::vector<RectangularBezierSurface> rectangularBezierSurfaces = {
	//     // surfaceTest,
	// };

	// =========================================================================
	// Create AABB Buffer and BLAS for Tetrahedrons, Spheres...

	float scalar = 1.0f;
	glm::vec3 offset = glm::vec3(0, 0, 0);
	[[maybe_unused]] auto tetrahedron2 = createTetrahedron2(std::to_array({
	    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset,
	    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset,
	    glm::vec3(0.0f, 2.0f, 0.0f) * scalar + offset,
	    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset,

	    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset,
	    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset,
	    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset,

	    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset,
	    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset,
	    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset,
	}));

	auto bezierTriangleH1 = extractBezierTriangleFromTetrahedron(tetrahedron2, 1);

	{
		auto obj = RaytracingWorldObject(ObjectType::t_BezierTriangle2,
		                                 AABB::fromBezierTriangle2(bezierTriangleH1),
		                                 bezierTriangleH1,
		                                 glm::vec3(0));
		raytracingScene.addWorldObject(obj);
	}

	{
		// auto obj = RaytracingWorldObject(ObjectType::t_Tetrahedron2,
		//                                  AABB::fromTetrahedron2(tetrahedron2),
		//                                  tetrahedron2,
		//                                  glm::vec3(0));
		// raytracingScene.addWorldObject(obj);
	}

	// Visualize control points
	for (auto& point : tetrahedron2.controlPoints)
	{
		raytracingScene.addSphere(point, 0.05f, ColorIdx::t_black);
	}

	// if (tetrahedrons2.size() > 0)
	{
		// auto rayPos = glm::vec3(-0.2f, 0.4f, 0.3f);
		// auto rayDirection = glm::vec3(1, -0.1, -0.8);

		// auto rayPos = glm::vec3(1.5f, -0.8f, 0.5f);
		// auto rayDirection = glm::normalize(glm::vec3(0.6, 0.1, 0.2) - rayPos);

		// spheres.emplace_back(rayPos, 0.04f, static_cast<int>(ColorIdx::t_red));
		// visualizeVector(spheres, rayPos, rayDirection, 2.8f, 0.01f);

		// glm::vec3 N1, N2;
		//{
		//	glm::vec3 v = (glm::abs(rayDirection.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
		//	                                                 : glm::vec3(1.0f, 0.0f, 0.0f);
		//	N1 = glm::normalize(glm::cross(rayDirection, v));
		//	N2 = glm::normalize(glm::cross(rayDirection, N1));
		// }

		// TODO: This is the way described in the paper but it does not work properly if the
		// direction is e.g. (1,0,0)
		// if (glm::abs(rayDirection.x) > glm::abs(rayDirection.y)
		//     && glm::abs(rayDirection.x) > glm::abs(rayDirection.z))
		// {
		// 	N1 = glm::normalize(glm::vec3(rayDirection.y, -rayDirection.z, 0));
		// }
		// else
		// {
		// 	N1 = glm::normalize(glm::vec3(0, rayDirection.z, -rayDirection.y));
		// }
		// N2 = glm::normalize(glm::cross(N1, rayDirection));

		// logVec3("Ray Position", rayPos);
		// logVec3("Ray Direction", rayDirection);
		// std::cout << "N1: " << N1.x << ", " << N1.y << ", " << N1.z
		//           << " Length: " << glm::length(N1) << std::endl;
		// std::cout << "N2: " << N2.x << ", " << N2.y << ", " << N2.z
		//           << " Length: " << glm::length(N2) << std::endl;
		// assert(glm::abs(glm::length(N1) - 1.0) < 1e-5 && "N1 is not normalized or zero");
		// assert(glm::abs(glm::length(N2) - 1.0) < 1e-5 && "N2 is not normalized or zero");

		// visualizeVector(spheres, rayPos + rayDirection * 0.2f, N1, 0.4f, 0.008f);
		// visualizeVector(spheres, rayPos + rayDirection * 0.2f, N2, 0.1f, 0.008f);

		// TODO: do some intersection calculations here

		// visualizePlane(spheres, N1, rayPos, 1, 1);
		// visualizePlane(spheres, N2, rayPos, 1, 1);

		visualizeTetrahedron2(raytracingScene, tetrahedron2);

		// glm::vec3 intersectionPoint{};
		//  TODO: figure out how to get a good initial guess
		//  maybe calculate the intersection with the AABB box? but how do we then transform that
		//  hitpos into the u,v parameter space? i guess getting the offset from the zero-position
		//  of the aabb and then mapping to 0-1 will work?

		// auto guesses = {
		//     glm::vec2(0.0, 0.0), glm::vec2(0.1, 0.1), glm::vec2(0.2, 0.2), glm::vec2(0.3, 0.3),
		//     glm::vec2(0.4, 0.4), glm::vec2(0.5, 0.5), glm::vec2(0.6, 0.6), glm::vec2(0.7, 0.7),
		//     glm::vec2(0.8, 0.8), glm::vec2(0.9, 0.9), glm::vec2(1.0, 1.0), glm::vec2(1.5, 1.5),

		//     glm::vec2(0, 0.0),   glm::vec2(0, 0.1),   glm::vec2(0, 0.2),   glm::vec2(0, 0.3),
		//     glm::vec2(0, 0.4),   glm::vec2(0, 0.5),   glm::vec2(0, 0.6),   glm::vec2(0, 0.7),
		//     glm::vec2(0, 0.8),   glm::vec2(0, 0.9),   glm::vec2(0, 1.0),   glm::vec2(0, 1.5),

		//     glm::vec2(0.0, 0),   glm::vec2(0.1, 0),   glm::vec2(0.2, 0),   glm::vec2(0.3, 0),
		//     glm::vec2(0.4, 0),   glm::vec2(0.5, 0),   glm::vec2(0.6, 0),   glm::vec2(0.7, 0),
		//     glm::vec2(0.8, 0),   glm::vec2(0.9, 0),   glm::vec2(1.0, 0),   glm::vec2(1.5, 0),
		// };

		// float t;
		// glm::vec2 guess = glm::vec3(0);

		// glm::vec3 v0 = tetrahedron2.controlPoints[getControlPointIndices(0, 0, 0)];
		// glm::vec3 v1 = tetrahedron2.controlPoints[getControlPointIndices(0, 2, 0)];
		// glm::vec3 v2 = tetrahedron2.controlPoints[getControlPointIndices(0, 0, 2)];
		//// spheres.emplace_back(v1, 0.4f, static_cast<int>(ColorIdx::t_red));
		//// spheres.emplace_back(v2, 0.4f, static_cast<int>(ColorIdx::t_red));
		//// spheres.emplace_back(v3, 0.4f, static_cast<int>(ColorIdx::t_red));

		// if (IntersectTriangle(rayPos, rayDirection, v0, v1, v2, t))
		//{
		//	vec3 P = rayPos + rayDirection * t;
		//	glm::vec3 a = v1 - v0;
		//	glm::vec3 b = v2 - v0;
		//	glm::vec3 c = P - v0;

		//	auto dot00 = glm::dot(a, a);
		//	auto dot01 = glm::dot(a, b);
		//	auto dot11 = glm::dot(b, b);
		//	auto dot20 = glm::dot(c, a);
		//	auto dot21 = glm::dot(c, b);
		//	auto denom = dot00 * dot11 - dot01 * dot01;
		//	auto v = (dot11 * dot20 - dot01 * dot21) / denom;
		//	auto w = (dot00 * dot21 - dot01 * dot20) / denom;
		//	// auto alpha = 1 - v - w;
		//	auto beta = v;
		//	auto gamma = w;

		//	std::cout << "Intersection with triangle!" << std::endl;
		//	guess = glm::vec2(beta, gamma);
		//	std::cout << "Calculated guess: " << guess.x << ", " << guess.y << std::endl;
		//}

		// for (auto guess : guesses)
		// {
		// if (newtonsMethod2(spheres,
		//                   intersectionPoint,
		//                   H1,
		//                   partialH1v2,
		//                   partialH1w2,
		//                   guess,
		//                   rayPos,
		//                   std::to_array(tetrahedron2.controlPoints),
		//                   N1,
		//                   N2))
		//{
		//	spheres.emplace_back(intersectionPoint, 0.1f, static_cast<int>(ColorIdx::t_orange));
		//}

		// 	std::cout << "Testing for Side 2 with H2" << std::endl;
		// 	if (newtonsMethod2(spheres,
		// 	                   intersectionPoint,
		// 	                   H2,
		// 	                   partialH2u2,
		// 	                   partialH2w2,
		// 	                   guess,
		// 	                   rayPos,
		// 	                   std::to_array(tetrahedron2.controlPoints),
		// 	                   N1,
		// 	                   N2))
		// 	{
		// 		spheres.emplace_back(intersectionPoint, 0.1f, static_cast<int>(ColorIdx::t_orange));
		// 	}

		// 	std::cout << "Testing for Side 3 with H3" << std::endl;
		// 	if (newtonsMethod2(spheres,
		// 	                   intersectionPoint,
		// 	                   H3,
		// 	                   partialH3u2,
		// 	                   partialH3v2,
		// 	                   guess,
		// 	                   rayPos,
		// 	                   std::to_array(tetrahedron2.controlPoints),
		// 	                   N1,
		// 	                   N2))
		// 	{
		// 		spheres.emplace_back(intersectionPoint, 0.1f, static_cast<int>(ColorIdx::t_orange));
		// 	}

		// 	std::cout << "Testing for Side 4 with H4" << std::endl;
		// 	if (newtonsMethod2(spheres,
		// 	                   intersectionPoint,
		// 	                   H4,
		// 	                   partialH4r2,
		// 	                   partialH4t2,
		// 	                   guess,
		// 	                   rayPos,
		// 	                   std::to_array(tetrahedron2.controlPoints),
		// 	                   N1,
		// 	                   N2))
		// 	{
		// 		spheres.emplace_back(intersectionPoint, 0.1f, static_cast<int>(ColorIdx::t_orange));
		// 	}
		// }
	}

	{
		// Visualize surface using spheres
		// std::vector<RectangularBezierSurface2x2> rectangularBezierSurfaces2x2;
		// if (!surfaceTest.tryConvertToRectangularSurfaces2x2(rectangularBezierSurfaces2x2))
		// {
		// 	throw std::runtime_error("failed to convert rectangularBezierSurfaces[0] to 2x2");
		// }

		// visualizeBezierSurface(spheres, rectangularBezierSurfaces2x2[0]);
	}

	// {
	// 	// Visualize slicing planes
	// 	auto slicingPlanes = std::vector<SlicingPlane>{
	// 	    {
	// 	        glm::vec3(5, 0, 0),
	// 	        glm::vec3(-1, 0, 0),
	// 	    },
	// 	};

	// 	raytracingInfo.objectBuffers.slicingPlanesBufferHandle
	// 	    = createObjectsBuffer(physicalDevice, logicalDevice, deletionQueue, slicingPlanes);
	// }

	// create fence for building acceleration structure
	VkFenceCreateInfo bottomLevelAccelerationStructureBuildFenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};

	result = vkCreateFence(logicalDevice,
	                       &bottomLevelAccelerationStructureBuildFenceCreateInfo,
	                       NULL,
	                       &raytracingInfo.accelerationStructureBuildFence);
	// vkResetFences(logicalDevice, 1, &raytracingInfo.accelerationStructureBuildFence);

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyFence(logicalDevice, raytracingInfo.accelerationStructureBuildFence, NULL); });

	// TODO: for now we don't change the amount of objects in the scene
	// so we can create the buffers only once
	// if we want to add/remove objects from the scene we need to recreate the buffers
	raytracingScene.createBuffers();

	// =========================================================================
	// Bottom and Top Level Acceleration Structure

	raytracingScene.recreateAccelerationStructures(raytracingInfo, true);

	// =========================================================================
	// Update Descriptor Set

	// TODO: only the last updated sphere buffer is active, idk how to pass in all BLAS
	// clang-format off
	// without creating a bunch of bindings or merging all BLAS buffers into one big buffer? idk
	// Store a generic "data object" that has a type idenitifier and a index into the corresponding buffer
	// eg. {Type::Sphere, 0} would be the first sphere in the buffer
	// {Type::Tetrahedron2, 0} would be the first tetrahedron in the spheres buffer
	// {Type::Tetrahedron2, 1} would be the second tetrahedron in the tetrahedron buffer
	// this requires us to merge all BLAS buffers of one type into one big buffer
	// clang-format on
	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingScene, raytracingInfo);

	// =========================================================================
	// Material Index Buffer

	// if (raytracingInfo.meshObjects.size() > 0)
	// {
	// 	std::vector<uint32_t> materialIndexList;
	// 	// TODO: Material list needs to include all possible materials from all objects, for now
	// 	// we only have one object
	// 	for (tinyobj::shape_t shape : raytracingInfo.meshObjects[0].shapes)
	// 	{
	// 		for (int index : shape.mesh.material_ids)
	// 		{
	// 			materialIndexList.push_back(static_cast<uint32_t>(index));
	// 		}
	// 	}

	// 	VkBuffer materialIndexBufferHandle = VK_NULL_HANDLE;
	// 	VkDeviceMemory materialIndexDeviceMemoryHandle = VK_NULL_HANDLE;
	// 	createBuffer(physicalDevice,
	// 	             logicalDevice,
	// 	             deletionQueue,
	// 	             sizeof(uint32_t) * materialIndexList.size(),
	// 	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	// 	             memoryAllocateFlagsInfo,
	// 	             materialIndexBufferHandle,
	// 	             materialIndexDeviceMemoryHandle);

	// 	void* hostMaterialIndexMemoryBuffer;
	// 	result = vkMapMemory(logicalDevice,
	// 	                     materialIndexDeviceMemoryHandle,
	// 	                     0,
	// 	                     sizeof(uint32_t) * materialIndexList.size(),
	// 	                     0,
	// 	                     &hostMaterialIndexMemoryBuffer);

	// 	memcpy(hostMaterialIndexMemoryBuffer,
	// 	       materialIndexList.data(),
	// 	       sizeof(uint32_t) * materialIndexList.size());

	// 	if (result != VK_SUCCESS)
	// 	{
	// 		throw new std::runtime_error("initRayTraci - vkMapMemory");
	// 	}

	// 	vkUnmapMemory(logicalDevice, materialIndexDeviceMemoryHandle);

	// 	// =========================================================================
	// 	// Material Buffer

	// 	// TODO: Material list needs to include all possible materials from all objects, for now
	// 	// we only have one object
	// 	std::vector<Material> materialList(raytracingInfo.meshObjects[0].materials.size());
	// 	for (uint32_t i = 0; i < raytracingInfo.meshObjects[0].materials.size(); i++)
	// 	{
	// 		memcpy(&materialList[i].ambient,
	// 		       raytracingInfo.meshObjects[0].materials[i].ambient,
	// 		       sizeof(float) * 3);
	// 		memcpy(&materialList[i].diffuse,
	// 		       raytracingInfo.meshObjects[0].materials[i].diffuse,
	// 		       sizeof(float) * 3);
	// 		memcpy(&materialList[i].specular,
	// 		       raytracingInfo.meshObjects[0].materials[i].specular,
	// 		       sizeof(float) * 3);
	// 		memcpy(&materialList[i].emission,
	// 		       raytracingInfo.meshObjects[0].materials[i].emission,
	// 		       sizeof(float) * 3);
	// 	}

	// 	VkBuffer materialBufferHandle = VK_NULL_HANDLE;
	// 	VkDeviceMemory materialDeviceMemoryHandle = VK_NULL_HANDLE;
	// 	createBuffer(physicalDevice,
	// 	             logicalDevice,
	// 	             deletionQueue,
	// 	             sizeof(Material) * materialList.size(),
	// 	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	// 	             memoryAllocateFlagsInfo,
	// 	             materialBufferHandle,
	// 	             materialDeviceMemoryHandle);

	// 	void* hostMaterialMemoryBuffer;
	// 	result = vkMapMemory(logicalDevice,
	// 	                     materialDeviceMemoryHandle,
	// 	                     0,
	// 	                     sizeof(Material) * materialList.size(),
	// 	                     0,
	// 	                     &hostMaterialMemoryBuffer);

	// 	memcpy( hostMaterialMemoryBuffer, materialList.data(), sizeof(Material) *
	// materialList.size());

	// 	if (result != VK_SUCCESS)
	// 	{
	// 		throw new std::runtime_error("initRayTraci - vkMapMemory");
	// 	}

	// 	vkUnmapMemory(logicalDevice, materialDeviceMemoryHandle);
	// 	// =========================================================================
	// 	// Update Material Descriptor Set

	// 	VkDescriptorBufferInfo materialIndexDescriptorInfo
	// 	    = {.buffer = materialIndexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	// 	VkDescriptorBufferInfo materialDescriptorInfo
	// 	    = {.buffer = materialBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	// 	std::vector<VkWriteDescriptorSet> materialWriteDescriptorSetList = {
	// 	    {
	// 	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	        .pNext = NULL,
	// 	        .dstSet = raytracingInfo.descriptorSetHandleList[1],
	// 	        .dstBinding = 0,
	// 	        .dstArrayElement = 0,
	// 	        .descriptorCount = 1,
	// 	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	// 	        .pImageInfo = NULL,
	// 	        .pBufferInfo = &materialIndexDescriptorInfo,
	// 	        .pTexelBufferView = NULL,
	// 	    },
	// 	    {
	// 	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	        .pNext = NULL,
	// 	        .dstSet = raytracingInfo.descriptorSetHandleList[1],
	// 	        .dstBinding = 1,
	// 	        .dstArrayElement = 0,
	// 	        .descriptorCount = 1,
	// 	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	// 	        .pImageInfo = NULL,
	// 	        .pBufferInfo = &materialDescriptorInfo,
	// 	        .pTexelBufferView = NULL,
	// 	    },
	// 	};

	// 	vkUpdateDescriptorSets(logicalDevice,
	// 	                       static_cast<uint32_t>(materialWriteDescriptorSetList.size()),
	// 	                       materialWriteDescriptorSetList.data(),
	// 	                       0,
	// 	                       NULL);
	// }

	// =========================================================================
	// Shader Binding Table
	VkDeviceSize progSize = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;

	VkDeviceSize shaderBindingTableSize = progSize * raytracingInfo.shaderGroupCount;

	VkBuffer shaderBindingTableBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory shaderBindingTableDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             shaderBindingTableSize,
	             VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             shaderBindingTableBufferHandle,
	             shaderBindingTableDeviceMemoryHandle);

	char* shaderHandleBuffer = new char[shaderBindingTableSize];
	result = ltracer::procedures::pvkGetRayTracingShaderGroupHandlesKHR(
	    logicalDevice,
	    raytracingInfo.rayTracingPipelineHandle,
	    0,
	    raytracingInfo.shaderGroupCount,
	    shaderBindingTableSize,
	    shaderHandleBuffer);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkGetRayTracingShaderGroupHandlesKHR");
	}

	void* hostShaderBindingTableMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     shaderBindingTableDeviceMemoryHandle,
	                     0,
	                     shaderBindingTableSize,
	                     0,
	                     &hostShaderBindingTableMemoryBuffer);

	for (uint32_t x = 0; x < 4; x++)
	{
		memcpy(hostShaderBindingTableMemoryBuffer,
		       shaderHandleBuffer
		           + x * physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
		       physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);

		hostShaderBindingTableMemoryBuffer
		    = reinterpret_cast<char*>(hostShaderBindingTableMemoryBuffer)
		      + physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	}

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, shaderBindingTableDeviceMemoryHandle);

	VkBufferDeviceAddressInfo shaderBindingTableBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = shaderBindingTableBufferHandle,
	};

	VkDeviceAddress shaderBindingTableBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &shaderBindingTableBufferDeviceAddressInfo);

	VkDeviceSize hitGroupOffset = raytracingInfo.hitGroupOffset * progSize;
	VkDeviceSize rayGenOffset = raytracingInfo.rayGenGroupOffset * progSize;
	VkDeviceSize missOffset = raytracingInfo.missGroupOffset * progSize;

	raytracingInfo.rchitShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + hitGroupOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.hitGroupSize,
	};

	raytracingInfo.rgenShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + rayGenOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.rayGenGroupSize,
	};

	raytracingInfo.rmissShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + missOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.missGroupSize,
	};

	raytracingInfo.callableShaderBindingTable = {};
}

void recordRaytracingCommandBuffer(VkCommandBuffer commandBuffer,
                                   VkExtent2D currentExtent,
                                   RaytracingInfo& raytracingInfo)
{
	VkImageMemoryBarrier rayTraceGeneralMemoryBarrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = NULL,
      .srcAccessMask = 0,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex =
          raytracingInfo.queueFamilyIndices.presentFamily.value(),
      .dstQueueFamilyIndex =
          raytracingInfo.queueFamilyIndices.presentFamily.value(),
      .image = raytracingInfo.rayTraceImageHandle,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

	vkCmdPipelineBarrier(commandBuffer,
	                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                     0,
	                     0,
	                     NULL,
	                     0,
	                     NULL,
	                     1,
	                     &rayTraceGeneralMemoryBarrier);

	// =========================================================================
	// Record Render Pass Command Buffers
	vkCmdBindPipeline(commandBuffer,
	                  VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
	                  raytracingInfo.rayTracingPipelineHandle);

	vkCmdBindDescriptorSets(commandBuffer,
	                        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
	                        raytracingInfo.pipelineLayoutHandle,
	                        0,
	                        static_cast<uint32_t>(raytracingInfo.descriptorSetHandleList.size()),
	                        raytracingInfo.descriptorSetHandleList.data(),
	                        0,
	                        NULL);

	// upload the matrix to the GPU via push constants
	vkCmdPushConstants(commandBuffer,
	                   raytracingInfo.pipelineLayoutHandle,
	                   VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	                       | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	                   0,
	                   sizeof(RaytracingDataConstants),
	                   &raytracingInfo.raytracingConstants);

	ltracer::procedures::pvkCmdTraceRaysKHR(commandBuffer,
	                                        &raytracingInfo.rgenShaderBindingTable,
	                                        &raytracingInfo.rmissShaderBindingTable,
	                                        &raytracingInfo.rchitShaderBindingTable,
	                                        &raytracingInfo.callableShaderBindingTable,
	                                        currentExtent.width,
	                                        currentExtent.height,
	                                        1);

	VkImageMemoryBarrier2 barrierRaytraceImage = {};
	barrierRaytraceImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrierRaytraceImage.srcAccessMask = 0; // No prior access
	barrierRaytraceImage.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrierRaytraceImage.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrierRaytraceImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrierRaytraceImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierRaytraceImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierRaytraceImage.image = raytracingInfo.rayTraceImageHandle;
	barrierRaytraceImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierRaytraceImage.subresourceRange.baseMipLevel = 0;
	barrierRaytraceImage.subresourceRange.levelCount = 1;
	barrierRaytraceImage.subresourceRange.baseArrayLayer = 0;
	barrierRaytraceImage.subresourceRange.layerCount = 1;
	barrierRaytraceImage.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	barrierRaytraceImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

	VkDependencyInfo dependencyInfoRaytraceImage = {};
	dependencyInfoRaytraceImage.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfoRaytraceImage.dependencyFlags = 0;
	dependencyInfoRaytraceImage.imageMemoryBarrierCount = 1;
	dependencyInfoRaytraceImage.pImageMemoryBarriers = &barrierRaytraceImage;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoRaytraceImage);
}

void updateRaytraceBuffer(VkDevice logicalDevice,
                          RaytracingInfo& raytracingInfo,
                          const bool resetFrameCountRequested)
{
	if (resetFrameCountRequested)
	{
		resetFrameCount(raytracingInfo);
	}
	else
	{
		raytracingInfo.uniformStructure.frameCount += 1;
	}

	// TODO: we need some synchronization here probably
	if (raytracingInfo.uniformDeviceMemoryHandle != VK_NULL_HANDLE)
	{
		void* hostUniformMemoryBuffer;
		VkResult result = vkMapMemory(logicalDevice,
		                              raytracingInfo.uniformDeviceMemoryHandle,
		                              0,
		                              sizeof(UniformStructure),
		                              0,
		                              &hostUniformMemoryBuffer);

		memcpy(hostUniformMemoryBuffer, &raytracingInfo.uniformStructure, sizeof(UniformStructure));

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error("initRayTracing - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, raytracingInfo.uniformDeviceMemoryHandle);
	}
}

void createRaytracingImage(VkPhysicalDevice physicalDevice,
                           VkDevice logicalDevice,
                           VkFormat swapChainFormat,
                           VkExtent2D currentExtent,
                           RaytracingInfo& raytracingInfo)
{
	VkImageCreateInfo rayTraceImageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = swapChainFormat,
        .extent =
            {
                .width = currentExtent.width,
                .height = currentExtent.height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

	// TODO: return this image instead of storing it in the raytracingInfo struct directly
	VkResult result = vkCreateImage(
	    logicalDevice, &rayTraceImageCreateInfo, NULL, &raytracingInfo.rayTraceImageHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkCreateImage");
	}

	VkMemoryRequirements rayTraceImageMemoryRequirements;
	vkGetImageMemoryRequirements(
	    logicalDevice, raytracingInfo.rayTraceImageHandle, &rayTraceImageMemoryRequirements);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	uint32_t rayTraceImageMemoryTypeIndex = 0;
	for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount; x++)
	{
		if ((rayTraceImageMemoryRequirements.memoryTypeBits & (1 << x))
		    && (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags
		        & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		           == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{

			rayTraceImageMemoryTypeIndex = x;
			break;
		}
	}

	VkMemoryAllocateInfo rayTraceImageMemoryAllocateInfo
	    = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	       .pNext = &memoryAllocateFlagsInfo,
	       .allocationSize = rayTraceImageMemoryRequirements.size,
	       .memoryTypeIndex = rayTraceImageMemoryTypeIndex};

	// TODO: is reallocating memory here necessary? just rebinding should be enough
	result = vkAllocateMemory(logicalDevice,
	                          &rayTraceImageMemoryAllocateInfo,
	                          NULL,
	                          &raytracingInfo.rayTraceImageDeviceMemoryHandle);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkAllocateMemory");
	}

	result = vkBindImageMemory(logicalDevice,
	                           raytracingInfo.rayTraceImageHandle,
	                           raytracingInfo.rayTraceImageDeviceMemoryHandle,
	                           0);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkBindImageMemory");
	}
}

VkImageView createRaytracingImageView(VkDevice logicalDevice,
                                      VkFormat swapChainFormat,
                                      const VkImage& rayTraceImageHandle)
{
	VkImageView rayTraceImageViewHandle = VK_NULL_HANDLE;
	VkImageViewCreateInfo rayTraceImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = rayTraceImageHandle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapChainFormat,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

	VkResult result = vkCreateImageView(
	    logicalDevice, &rayTraceImageViewCreateInfo, NULL, &rayTraceImageViewHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkCreateImageView");
	}
	return rayTraceImageViewHandle;
}

void freeRaytraceImageAndImageView(VkDevice logicalDevice,
                                   VkImage& rayTraceImageHandle,
                                   VkImageView& rayTraceImageViewHandle,
                                   VkDeviceMemory& rayTraceImageDeviceMemoryHandle)
{
	assert(rayTraceImageHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkDestroyImage(logicalDevice, rayTraceImageHandle, NULL);

	assert(rayTraceImageViewHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkDestroyImageView(logicalDevice, rayTraceImageViewHandle, nullptr);

	assert(rayTraceImageDeviceMemoryHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkFreeMemory(logicalDevice, rayTraceImageDeviceMemoryHandle, nullptr);
}

void recreateRaytracingImageBuffer(VkPhysicalDevice physicalDevice,
                                   VkDevice logicalDevice,
                                   VkFormat swapChainImageFormat,
                                   VkExtent2D windowExtent,
                                   RaytracingScene& raytracingScene,
                                   RaytracingInfo& raytracingInfo)
{
	freeRaytraceImageAndImageView(logicalDevice,
	                              raytracingInfo.rayTraceImageHandle,
	                              raytracingInfo.rayTraceImageViewHandle,
	                              raytracingInfo.rayTraceImageDeviceMemoryHandle);

	createRaytracingImage(
	    physicalDevice, logicalDevice, swapChainImageFormat, windowExtent, raytracingInfo);

	raytracingInfo.rayTraceImageViewHandle = createRaytracingImageView(
	    logicalDevice, swapChainImageFormat, raytracingInfo.rayTraceImageHandle);

	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingScene, raytracingInfo);

	// reset frame count so the window gets refreshed properly
	resetFrameCount(raytracingInfo);
}

} // namespace rt
} // namespace ltracer
