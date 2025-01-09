#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "src/blas.hpp"
#include "src/camera.hpp"
#include "src/deletion_queue.hpp"
#include "src/shader_module.hpp"
#include "src/types.hpp"
#include "src/window.hpp"
#include "src/device_procedures.hpp"

#include "tiny_obj_loader.h"

namespace ltracer
{
namespace rt
{

static std::vector<VkDeviceAddress> accelerationStructureInstanceAddresses;
static VkBuffer accelerationStructureInstancesBufferHandle = VK_NULL_HANDLE;
static VkDeviceMemory accelerationStructuresInstancesDeviceMemoryHandle = VK_NULL_HANDLE;

// TODO: move these into some struct or class for better organization
static UniformStructure uniformStructure;

static VkDeviceMemory vertexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkBuffer shaderBindingTableBufferHandle = VK_NULL_HANDLE;
static VkBuffer materialBufferHandle = VK_NULL_HANDLE;
static VkBuffer materialIndexBufferHandle = VK_NULL_HANDLE;
static VkBuffer uniformBufferHandle = VK_NULL_HANDLE;
static VkFence topLevelAccelerationStructureBuildFenceHandle = VK_NULL_HANDLE;
static VkBuffer topLevelAccelerationStructureScratchBufferHandle = VK_NULL_HANDLE;
static VkAccelerationStructureKHR topLevelAccelerationStructureHandle = VK_NULL_HANDLE;
static VkBuffer topLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
static VkBuffer indexBufferHandle = VK_NULL_HANDLE;
static VkBuffer vertexBufferHandle = VK_NULL_HANDLE;
static VkShaderModule rayMissShadowShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayMissShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayGenerateShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayClosestHitShaderModuleHandle = VK_NULL_HANDLE;
static VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
static VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
static VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
static VkDeviceMemory rayTraceImageDeviceMemoryHandle = VK_NULL_HANDLE;
static VkCommandPool commandPoolHandle = VK_NULL_HANDLE;
static VkDeviceMemory uniformDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory indexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory topLevelAccelerationStructureDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory topLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory materialIndexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory materialDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory shaderBindingTableDeviceMemoryHandle = VK_NULL_HANDLE;
static void* hostUniformMemoryBuffer;
static VkQueue queueHandle = VK_NULL_HANDLE;

inline void
updateUniformStructure(glm::vec3 position, glm::vec3 right, glm::vec3 up, glm::vec3 forward)
{
	uniformStructure.cameraPosition[0] = position[0];
	uniformStructure.cameraPosition[1] = position[1];
	uniformStructure.cameraPosition[2] = position[2];

	uniformStructure.cameraRight[0] = right[0];
	uniformStructure.cameraRight[1] = right[1];
	uniformStructure.cameraRight[2] = right[2];

	uniformStructure.cameraUp[0] = up[0];
	uniformStructure.cameraUp[1] = up[1];
	uniformStructure.cameraUp[2] = up[2];

	uniformStructure.cameraForward[0] = forward[0];
	uniformStructure.cameraForward[1] = forward[1];
	uniformStructure.cameraForward[2] = forward[2];
}

inline VkPhysicalDeviceRayTracingPipelineFeaturesKHR getRaytracingPipelineFeatures()
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

inline void buildRaytracingAccelerationStructures(VkDevice logicalDevice,
                                                  RaytracingInfo raytracingInfo,
                                                  VkCommandBuffer commandBuffer)
{
	// =========================================================================
	// Ray Trace Image Barrier
	// (VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_GENERAL)

	// VkSubmitInfo rayTraceImageBarrierAccelerationStructureBuildSubmitInfo = {
	//     .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	//     .pNext = NULL,
	//     .waitSemaphoreCount = 0,
	//     .pWaitSemaphores = NULL,
	//     .pWaitDstStageMask = NULL,
	//     .commandBufferCount = 1,
	//     .pCommandBuffers = &commandBuffer,
	//     .signalSemaphoreCount = 0,
	//     .pSignalSemaphores = NULL,
	// };

	// VkFenceCreateInfo
	//     rayTraceImageBarrierAccelerationStructureBuildFenceCreateInfo = {
	//         .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	//         .pNext = NULL,
	//         .flags = 0,
	//     };

	// VkFence rayTraceImageBarrierAccelerationStructureBuildFenceHandle =
	//     VK_NULL_HANDLE;
	// VkResult result = vkCreateFence(
	//     logicalDevice,
	//     &rayTraceImageBarrierAccelerationStructureBuildFenceCreateInfo, NULL,
	//     &rayTraceImageBarrierAccelerationStructureBuildFenceHandle);

	// if (result != VK_SUCCESS) {
	//   throw new std::runtime_error("initRayTraci - vkCreateFence");
	// }

	// result = vkQueueSubmit(
	//     queueHandle, 1,
	//     &rayTraceImageBarrierAccelerationStructureBuildSubmitInfo,
	//     rayTraceImageBarrierAccelerationStructureBuildFenceHandle);

	// if (result != VK_SUCCESS) {
	//   throw new std::runtime_error("initRayTraci - vkQueueSubmit");
	// }

	// result = vkWaitForFences(
	//     logicalDevice, 1,
	//     &rayTraceImageBarrierAccelerationStructureBuildFenceHandle, true,
	//     UINT32_MAX);

	// if (result != VK_SUCCESS && result != VK_TIMEOUT) {
	//   throw new std::runtime_error("initRayTraci - vkWaitForFences");
	// }
}

inline void createCommandPool(VkDevice logicalDevice,
                              DeletionQueue& deletionQueue,
                              RaytracingInfo& raytracingInfo)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
	};

	VkResult result
	    = vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, NULL, &commandPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateCommandPool failed");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyCommandPool(logicalDevice, commandPoolHandle, nullptr); });
}

inline void requestRaytracingProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	physicalDeviceRayTracingPipelineProperties = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
	    .pNext = NULL,
	};

	VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	    .pNext = &physicalDeviceRayTracingPipelineProperties,
	    .properties = physicalDeviceProperties,
	};

	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);
}

inline void createCommandBufferBuildTopAndBottomLevel(VkDevice logicalDevice,
                                                      DeletionQueue& deletionQueue,
                                                      RaytracingInfo& raytracingInfo)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = NULL,
	    .commandPool = commandPoolHandle,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1,
	};

	VkResult result = vkAllocateCommandBuffers(logicalDevice,
	                                           &commandBufferAllocateInfo,
	                                           &raytracingInfo.commandBufferBuildTopAndBottomLevel);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateCommandBuffers failed");
	}

	deletionQueue.push_function(
	    [=]()
	    {
		    vkFreeCommandBuffers(logicalDevice,
		                         commandPoolHandle,
		                         1,
		                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
	    });
}

inline void createDescriptorPool(DeletionQueue& deletionQueue, VkDevice logicalDevice)
{
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
	    .poolSizeCount = (uint32_t)descriptorPoolSizeList.size(),
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
}

inline void createDescriptorSetLayout(DeletionQueue& deletionQueue, VkDevice logicalDevice)
{
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList
	    = {{
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
	       }};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	       .pNext = NULL,
	       .flags = 0,
	       .bindingCount = (uint32_t)descriptorSetLayoutBindingList.size(),
	       .pBindings = descriptorSetLayoutBindingList.data()};

	VkResult result = vkCreateDescriptorSetLayout(
	    logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorSetLayout");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayoutHandle, NULL); });
}

inline void createMaterialDescriptorSetLayout(DeletionQueue& deletionQueue, VkDevice logicalDevice)
{
	std::vector<VkDescriptorSetLayoutBinding> materialDescriptorSetLayoutBindingList
	    = {{.binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL},
	       {.binding = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL}};

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
}

inline void
allocateDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList,
                             RaytracingInfo& raytracingInfo,
                             VkDevice logicalDevice)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = NULL,
	    .descriptorPool = descriptorPoolHandle,
	    .descriptorSetCount = (uint32_t)descriptorSetLayoutHandleList.size(),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	};

	VkResult result = vkAllocateDescriptorSets(
	    logicalDevice, &descriptorSetAllocateInfo, raytracingInfo.descriptorSetHandleList.data());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateDescriptorSets");
	}
}

inline void createPipelineLayout(DeletionQueue& deletionQueue,
                                 RaytracingInfo& raytracingInfo,
                                 std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList,
                                 VkDevice logicalDevice)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	       .pNext = NULL,
	       .flags = 0,
	       .setLayoutCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	       .pSetLayouts = descriptorSetLayoutHandleList.data(),
	       .pushConstantRangeCount = 0,
	       .pPushConstantRanges = NULL};

	VkResult result = vkCreatePipelineLayout(
	    logicalDevice, &pipelineLayoutCreateInfo, NULL, &raytracingInfo.pipelineLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreatePipelineLayout");
	}

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyPipelineLayout(logicalDevice, raytracingInfo.pipelineLayoutHandle, NULL); });
}

inline void loadShaderModules(DeletionQueue& deletionQueue, VkDevice logicalDevice)
{

	// =========================================================================
	// Ray Closest Hit Shader Module
	ltracer::shader::createShaderModule(
	    "shaders/shader.rchit.spv", logicalDevice, deletionQueue, rayClosestHitShaderModuleHandle);

	// =========================================================================
	// Ray Generate Shader Module
	ltracer::shader::createShaderModule(
	    "shaders/shader.rgen.spv", logicalDevice, deletionQueue, rayGenerateShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module

	ltracer::shader::createShaderModule(
	    "shaders/shader.rmiss.spv", logicalDevice, deletionQueue, rayMissShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module (Shadow)

	ltracer::shader::createShaderModule("shaders/shader_shadow.rmiss.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    rayMissShadowShaderModuleHandle);
}

inline void createRaytracingPipeline(DeletionQueue& deletionQueue,
                                     RaytracingInfo& raytracingInfo,
                                     VkDevice logicalDevice)
{
	std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfoList
	    = {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .module = rayClosestHitShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	        .module = rayGenerateShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	        .module = rayMissShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	        .module = rayMissShadowShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL}};

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rayTracingShaderGroupCreateInfoList
	    = {{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
	        .generalShader = VK_SHADER_UNUSED_KHR,
	        .closestHitShader = 0,
	        .anyHitShader = VK_SHADER_UNUSED_KHR,
	        .intersectionShader = VK_SHADER_UNUSED_KHR,
	        .pShaderGroupCaptureReplayHandle = NULL},
	       {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	        .generalShader = 1,
	        .closestHitShader = VK_SHADER_UNUSED_KHR,
	        .anyHitShader = VK_SHADER_UNUSED_KHR,
	        .intersectionShader = VK_SHADER_UNUSED_KHR,
	        .pShaderGroupCaptureReplayHandle = NULL},
	       {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	        .generalShader = 2,
	        .closestHitShader = VK_SHADER_UNUSED_KHR,
	        .anyHitShader = VK_SHADER_UNUSED_KHR,
	        .intersectionShader = VK_SHADER_UNUSED_KHR,
	        .pShaderGroupCaptureReplayHandle = NULL},
	       {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	        .generalShader = 3,
	        .closestHitShader = VK_SHADER_UNUSED_KHR,
	        .anyHitShader = VK_SHADER_UNUSED_KHR,
	        .intersectionShader = VK_SHADER_UNUSED_KHR,
	        .pShaderGroupCaptureReplayHandle = NULL}};

	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .flags = 0,
	    .stageCount = 4,
	    .pStages = pipelineShaderStageCreateInfoList.data(),
	    .groupCount = 4,
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
	    [=]() { vkDestroyPipeline(logicalDevice, raytracingInfo.rayTracingPipelineHandle, NULL); });
}

inline void loadOBJScene(std::string scenePath,
                         uint32_t& primitiveCount,
                         std::vector<float>& vertices,
                         std::vector<uint32_t>& indexList,
                         std::vector<tinyobj::shape_t>& shapes,
                         std::vector<tinyobj::material_t>& materials)
{
	// std::filesystem::path arrowPath = "3d-models/up_arrow.obj";
	// auto loaded_model = loadModel(arrowPath);

	// =========================================================================
	// OBJ Model

	tinyobj::ObjReaderConfig reader_config;
	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(scenePath, reader_config))
	{
		if (!reader.Error().empty())
		{
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty())
	{
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	const tinyobj::attrib_t& attrib = reader.GetAttrib();
	vertices = attrib.vertices;

	shapes = reader.GetShapes();
	materials = reader.GetMaterials();

	primitiveCount = 0;
	for (tinyobj::shape_t shape : shapes)
	{

		primitiveCount += shape.mesh.num_face_vertices.size();

		for (tinyobj::index_t index : shape.mesh.indices)
		{
			indexList.push_back(index.vertex_index);
		}
	}
}

inline void createAndBuildTopLevelAccelerationStructure(
    std::vector<VkDeviceAddress>& bottomLevelAccelerationStructureDeviceAddresses,
    DeletionQueue& deletionQueue,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    RaytracingInfo& raytracingInfo)
{

	// VkBufferDeviceAddressInfo bottomLevelGeometryInstanceDeviceAddressInfo;

	accelerationStructureInstanceAddresses.clear();
	// TODO: fix this indexing: bottomLevelAccelerationStructureDeviceAddresses[0]
	for (size_t i = 0; i < bottomLevelAccelerationStructureDeviceAddresses.size(); i++)
	{
		VkDeviceAddress bottomLevelGeometryInstanceDeviceAddress;
		ltracer::rt::createBottomLevelGeometryInstance(
		    physicalDevice,
		    logicalDevice,
		    deletionQueue,
		    raytracingInfo,
		    bottomLevelAccelerationStructureDeviceAddresses[i],
		    bottomLevelGeometryInstanceDeviceAddress);

		accelerationStructureInstanceAddresses.push_back(bottomLevelGeometryInstanceDeviceAddress);
	}

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(VkDeviceAddress) * accelerationStructureInstanceAddresses.size(),
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             accelerationStructureInstancesBufferHandle,
	             accelerationStructuresInstancesDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.graphicsFamily.value()});

	void* hostAccelerationStructureInstancesMemoryBuffer;
	VkResult result
	    = vkMapMemory(logicalDevice,
	                  accelerationStructuresInstancesDeviceMemoryHandle,
	                  0,
	                  sizeof(VkDeviceAddress) * accelerationStructureInstanceAddresses.size(),
	                  0,
	                  &hostAccelerationStructureInstancesMemoryBuffer);

	memcpy(hostAccelerationStructureInstancesMemoryBuffer,
	       accelerationStructureInstanceAddresses.data(),
	       sizeof(VkDeviceAddress) * accelerationStructureInstanceAddresses.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("createAndBuildTopLevelAccelerationStructure - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, accelerationStructuresInstancesDeviceMemoryHandle);

	VkBufferDeviceAddressInfoKHR accelerationStructureDeviceAddressesInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
	       .pNext = NULL,
	       .buffer = accelerationStructureInstancesBufferHandle};

	VkDeviceAddress accelerationStructureInstancesDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &accelerationStructureDeviceAddressesInfo);

	VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData = {};
	topLevelAccelerationStructureGeometryData.instances = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
		.pNext = NULL,
		.arrayOfPointers = VK_TRUE,
		.data = {
			.deviceAddress = accelerationStructureInstancesDeviceAddress,
		},
	};

	VkAccelerationStructureGeometryKHR topLevelAccelerationStructureGeometry = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = NULL,
	    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
	    .geometry = topLevelAccelerationStructureGeometryData,
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildGeometryInfoKHR topLevelAccelerationStructureBuildGeometryInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .flags = 0,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = VK_NULL_HANDLE,
	    .geometryCount = static_cast<uint32_t>(accelerationStructureInstanceAddresses.size()),
	    .pGeometries = &topLevelAccelerationStructureGeometry,
	    .ppGeometries = NULL,
	    .scratchData = {.deviceAddress = 0},
	};

	VkAccelerationStructureBuildSizesInfoKHR topLevelAccelerationStructureBuildSizesInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	    .pNext = NULL,
	    .accelerationStructureSize = 0,
	    .updateScratchSize = 0,
	    .buildScratchSize = 0,
	};

	std::vector<uint32_t> topLevelMaxPrimitiveCountList = {1};

	ltracer::procedures::pvkGetAccelerationStructureBuildSizesKHR(
	    logicalDevice,
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &topLevelAccelerationStructureBuildGeometryInfo,
	    topLevelMaxPrimitiveCountList.data(),
	    &topLevelAccelerationStructureBuildSizesInfo);

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             topLevelAccelerationStructureBufferHandle,
	             topLevelAccelerationStructureDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	VkAccelerationStructureCreateInfoKHR topLevelAccelerationStructureCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	       .pNext = NULL,
	       .createFlags = 0,
	       .buffer = topLevelAccelerationStructureBufferHandle,
	       .offset = 0,
	       .size = topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	       .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	       .deviceAddress = 0};

	result = ltracer::procedures::pvkCreateAccelerationStructureKHR(
	    logicalDevice,
	    &topLevelAccelerationStructureCreateInfo,
	    NULL,
	    &topLevelAccelerationStructureHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateAccelerationStructureKHR");
	}

	deletionQueue.push_function(
	    [=]()
	    {
		    ltracer::procedures::pvkDestroyAccelerationStructureKHR(
		        logicalDevice, topLevelAccelerationStructureHandle, NULL);
	    });

	// Build Top Level Acceleration Structure

	VkAccelerationStructureDeviceAddressInfoKHR topLevelAccelerationStructureDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	       .pNext = NULL,
	       .accelerationStructure = topLevelAccelerationStructureHandle};

	VkDeviceAddress topLevelAccelerationStructureDeviceAddress
	    = ltracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
	        logicalDevice, &topLevelAccelerationStructureDeviceAddressInfo);

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             topLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             topLevelAccelerationStructureScratchBufferHandle,
	             topLevelAccelerationStructureDeviceScratchMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	VkBufferDeviceAddressInfo topLevelAccelerationStructureScratchBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = topLevelAccelerationStructureScratchBufferHandle};

	VkDeviceAddress topLevelAccelerationStructureScratchBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &topLevelAccelerationStructureScratchBufferDeviceAddressInfo);

	topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
	    = topLevelAccelerationStructureHandle;

	topLevelAccelerationStructureBuildGeometryInfo.scratchData
	    = {.deviceAddress = topLevelAccelerationStructureScratchBufferDeviceAddress};

	VkAccelerationStructureBuildRangeInfoKHR topLevelAccelerationStructureBuildRangeInfo
	    = {.primitiveCount = 1, .primitiveOffset = 0, .firstVertex = 0, .transformOffset = 0};

	const VkAccelerationStructureBuildRangeInfoKHR* topLevelAccelerationStructureBuildRangeInfos
	    = &topLevelAccelerationStructureBuildRangeInfo;

	VkCommandBufferBeginInfo topLevelCommandBufferBeginInfo
	    = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	       .pNext = NULL,
	       .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	       .pInheritanceInfo = NULL};

	result = vkBeginCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
	                              &topLevelCommandBufferBeginInfo);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkBeginCommandBuffer");
	}

	ltracer::procedures::pvkCmdBuildAccelerationStructuresKHR(
	    raytracingInfo.commandBufferBuildTopAndBottomLevel,
	    1,
	    &topLevelAccelerationStructureBuildGeometryInfo,
	    &topLevelAccelerationStructureBuildRangeInfos);

	result = vkEndCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkEndCommandBuffer");
	}

	VkSubmitInfo topLevelAccelerationStructureBuildSubmitInfo
	    = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	       .pNext = NULL,
	       .waitSemaphoreCount = 0,
	       .pWaitSemaphores = NULL,
	       .pWaitDstStageMask = NULL,
	       .commandBufferCount = 1,
	       .pCommandBuffers = &raytracingInfo.commandBufferBuildTopAndBottomLevel,
	       .signalSemaphoreCount = 0,
	       .pSignalSemaphores = NULL};

	VkFenceCreateInfo topLevelAccelerationStructureBuildFenceCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};

	result = vkCreateFence(logicalDevice,
	                       &topLevelAccelerationStructureBuildFenceCreateInfo,
	                       NULL,
	                       &topLevelAccelerationStructureBuildFenceHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateFence");
	}

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyFence(logicalDevice, topLevelAccelerationStructureBuildFenceHandle, NULL); });

	result = vkQueueSubmit(queueHandle,
	                       1,
	                       &topLevelAccelerationStructureBuildSubmitInfo,
	                       topLevelAccelerationStructureBuildFenceHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkQueueSubmit");
	}

	result = vkWaitForFences(
	    logicalDevice, 1, &topLevelAccelerationStructureBuildFenceHandle, true, UINT32_MAX);

	if (result != VK_SUCCESS && result != VK_TIMEOUT)
	{
		throw new std::runtime_error("initRayTracing - vkWaitForFences");
	}
}

inline void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                                     RaytracingInfo& raytracingInfo)
{
	VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptorInfo = {
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
	    .pNext = NULL,
	    .accelerationStructureCount = 1,
	    .pAccelerationStructures = &topLevelAccelerationStructureHandle,
	};

	VkDescriptorBufferInfo uniformDescriptorInfo
	    = {.buffer = uniformBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	VkDescriptorBufferInfo indexDescriptorInfo
	    = {.buffer = indexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	VkDescriptorBufferInfo vertexDescriptorInfo
	    = {.buffer = vertexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	VkDescriptorImageInfo rayTraceImageDescriptorInfo
	    = {.sampler = VK_NULL_HANDLE,
	       .imageView = raytracingInfo.rayTraceImageViewHandle,
	       .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

	std::vector<VkWriteDescriptorSet> writeDescriptorSetList
	    = {{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = &accelerationStructureDescriptorInfo,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 0,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	        .pImageInfo = NULL,
	        .pBufferInfo = NULL,
	        .pTexelBufferView = NULL},
	       {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 1,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &uniformDescriptorInfo,
	        .pTexelBufferView = NULL},
	       {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 2,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &indexDescriptorInfo,
	        .pTexelBufferView = NULL},
	       {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 3,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &vertexDescriptorInfo,
	        .pTexelBufferView = NULL},
	       {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 4,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .pImageInfo = &rayTraceImageDescriptorInfo,
	        .pBufferInfo = NULL,
	        .pTexelBufferView = NULL}};

	vkUpdateDescriptorSets(
	    logicalDevice, writeDescriptorSetList.size(), writeDescriptorSetList.data(), 0, NULL);
}

// TODO: Split into multiple functions for each step
inline void initRayTracing(VkPhysicalDevice physicalDevice,
                           VkDevice logicalDevice,
                           DeletionQueue& deletionQueue,
                           std::shared_ptr<ltracer::Window> window,
                           std::vector<VkImageView>& swapChainImageViews,
                           RaytracingInfo& raytracingInfo)
{

	// Requesting ray trcing properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties;
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
	vkGetDeviceQueue(
	    logicalDevice, raytracingInfo.queueFamilyIndices.graphicsFamily.value(), 0, &queueHandle);

	// =========================================================================
	// Device Pointer Functions
	ltracer::procedures::grabDeviceProcAddr(logicalDevice);

	// =========================================================================
	// Command Pool
	createCommandPool(logicalDevice, deletionQueue, raytracingInfo);

	// =========================================================================
	// Command Buffers

	createCommandBufferBuildTopAndBottomLevel(logicalDevice, deletionQueue, raytracingInfo);

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

	createDescriptorPool(deletionQueue, logicalDevice);

	// =========================================================================
	// Descriptor Set Layout

	createDescriptorSetLayout(deletionQueue, logicalDevice);

	// =========================================================================
	// Material Descriptor Set Layout
	createMaterialDescriptorSetLayout(deletionQueue, logicalDevice);

	// =========================================================================
	// Allocate Descriptor Sets
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList
	    = {descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

	allocateDescriptorSetLayouts(descriptorSetLayoutHandleList, raytracingInfo, logicalDevice);

	// =========================================================================
	// Pipeline Layout
	createPipelineLayout(
	    deletionQueue, raytracingInfo, descriptorSetLayoutHandleList, logicalDevice);
	// =========================================================================
	// Ray Closest Hit Shader Module
	loadShaderModules(deletionQueue, logicalDevice);

	// =========================================================================
	// Ray Tracing Pipeline
	createRaytracingPipeline(deletionQueue, raytracingInfo, logicalDevice);

	// =========================================================================
	// load OBJ Model

	uint32_t primitiveCount = 0;
	std::vector<float> vertices;
	std::vector<uint32_t> indexList;
	std::string scenePath = "3d-models/cube_scene.obj";
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	loadOBJScene(scenePath, primitiveCount, vertices, indexList, shapes, materials);

	// =========================================================================
	// Vertex Buffer
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(float) * vertices.size() * 3,
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             vertexBufferHandle,
	             vertexDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostVertexMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     vertexDeviceMemoryHandle,
	                     0,
	                     sizeof(float) * vertices.size() * 3,
	                     0,
	                     &hostVertexMemoryBuffer);

	memcpy(hostVertexMemoryBuffer, vertices.data(), sizeof(float) * vertices.size() * 3);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, vertexDeviceMemoryHandle);

	VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = vertexBufferHandle};

	VkDeviceAddress vertexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &vertexBufferDeviceAddressInfo);

	// =========================================================================
	// Index Buffer

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(uint32_t) * indexList.size(),
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             indexBufferHandle,
	             indexDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostIndexMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     indexDeviceMemoryHandle,
	                     0,
	                     sizeof(uint32_t) * indexList.size(),
	                     0,
	                     &hostIndexMemoryBuffer);

	memcpy(hostIndexMemoryBuffer, indexList.data(), sizeof(uint32_t) * indexList.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, indexDeviceMemoryHandle);

	VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = indexBufferHandle};

	VkDeviceAddress indexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &indexBufferDeviceAddressInfo);

	// =========================================================================
	// Bottom Level Acceleration Structure
	std::vector<VkDeviceAddress> bottomLevelAccelerationStructureDeviceAddresses;
	auto triangleBLAS
	    = createAndBuildBottomLevelAccelerationStructureTriangle(deletionQueue,
	                                                             indexBufferDeviceAddress,
	                                                             logicalDevice,
	                                                             physicalDevice,
	                                                             primitiveCount,
	                                                             vertices.size(),
	                                                             vertexBufferDeviceAddress,
	                                                             raytracingInfo,
	                                                             queueHandle);
	bottomLevelAccelerationStructureDeviceAddresses.push_back(triangleBLAS);

	// auto aabbBLAS = createAndBuildBottomLevelAccelerationStructureAABB(
	//     deletionQueue, logicalDevice, physicalDevice, raytracingInfo);
	// bottomLevelAccelerationStructureDeviceAddresses.push_back(aabbBLAS);

	// vkResetCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
	// 0);

	// =========================================================================
	// Top Level Acceleration Structure
	createAndBuildTopLevelAccelerationStructure(bottomLevelAccelerationStructureDeviceAddresses,
	                                            deletionQueue,
	                                            logicalDevice,
	                                            physicalDevice,
	                                            raytracingInfo);

	// =========================================================================
	// Uniform Buffer
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(UniformStructure),
	             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             uniformBufferHandle,
	             uniformDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	result = vkMapMemory(logicalDevice,
	                     uniformDeviceMemoryHandle,
	                     0,
	                     sizeof(UniformStructure),
	                     0,
	                     &hostUniformMemoryBuffer);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}
	memcpy(hostUniformMemoryBuffer, &uniformStructure, sizeof(UniformStructure));

	vkUnmapMemory(logicalDevice, uniformDeviceMemoryHandle);

	// =========================================================================
	// Update Descriptor Set
	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingInfo);

	// =========================================================================
	// Material Index Buffer
	std::vector<uint32_t> materialIndexList;
	for (tinyobj::shape_t shape : shapes)
	{
		for (int index : shape.mesh.material_ids)
		{
			materialIndexList.push_back(index);
		}
	}

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(uint32_t) * materialIndexList.size(),
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             materialIndexBufferHandle,
	             materialIndexDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostMaterialIndexMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     materialIndexDeviceMemoryHandle,
	                     0,
	                     sizeof(uint32_t) * materialIndexList.size(),
	                     0,
	                     &hostMaterialIndexMemoryBuffer);

	memcpy(hostMaterialIndexMemoryBuffer,
	       materialIndexList.data(),
	       sizeof(uint32_t) * materialIndexList.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, materialIndexDeviceMemoryHandle);

	// =========================================================================
	// Material Buffer

	struct Material
	{
		float ambient[4] = {0, 0, 0, 0};
		float diffuse[4] = {0, 0, 0, 0};
		float specular[4] = {0, 0, 0, 0};
		float emission[4] = {0, 0, 0, 0};
	};

	std::vector<Material> materialList(materials.size());
	for (uint32_t x = 0; x < materials.size(); x++)
	{
		memcpy(materialList[x].ambient, materials[x].ambient, sizeof(float) * 3);
		memcpy(materialList[x].diffuse, materials[x].diffuse, sizeof(float) * 3);
		memcpy(materialList[x].specular, materials[x].specular, sizeof(float) * 3);
		memcpy(materialList[x].emission, materials[x].emission, sizeof(float) * 3);
	}

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(Material) * materialList.size(),
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             materialBufferHandle,
	             materialDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostMaterialMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     materialDeviceMemoryHandle,
	                     0,
	                     sizeof(Material) * materialList.size(),
	                     0,
	                     &hostMaterialMemoryBuffer);

	memcpy(hostMaterialMemoryBuffer, materialList.data(), sizeof(Material) * materialList.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, materialDeviceMemoryHandle);

	// =========================================================================
	// Update Material Descriptor Set

	VkDescriptorBufferInfo materialIndexDescriptorInfo
	    = {.buffer = materialIndexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	VkDescriptorBufferInfo materialDescriptorInfo
	    = {.buffer = materialBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

	std::vector<VkWriteDescriptorSet> materialWriteDescriptorSetList = {
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[1],
	        .dstBinding = 0,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &materialIndexDescriptorInfo,
	        .pTexelBufferView = NULL,
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[1],
	        .dstBinding = 1,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &materialDescriptorInfo,
	        .pTexelBufferView = NULL,
	    },
	};

	vkUpdateDescriptorSets(logicalDevice,
	                       materialWriteDescriptorSetList.size(),
	                       materialWriteDescriptorSetList.data(),
	                       0,
	                       NULL);

	// =========================================================================
	// Shader Binding Table
	VkDeviceSize progSize = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;

	VkDeviceSize shaderBindingTableSize = progSize * 4;

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             shaderBindingTableSize,
	             VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             shaderBindingTableBufferHandle,
	             shaderBindingTableDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	char* shaderHandleBuffer = new char[shaderBindingTableSize];
	result = ltracer::procedures::pvkGetRayTracingShaderGroupHandlesKHR(
	    logicalDevice,
	    raytracingInfo.rayTracingPipelineHandle,
	    0,
	    4,
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

	VkBufferDeviceAddressInfo shaderBindingTableBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = shaderBindingTableBufferHandle};

	VkDeviceAddress shaderBindingTableBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &shaderBindingTableBufferDeviceAddressInfo);

	VkDeviceSize hitGroupOffset = 0u * progSize;
	VkDeviceSize rayGenOffset = 1u * progSize;
	VkDeviceSize missOffset = 2u * progSize;

	raytracingInfo.rchitShaderBindingTable
	    = {.deviceAddress = shaderBindingTableBufferDeviceAddress + hitGroupOffset,
	       .stride = progSize,
	       .size = progSize};

	raytracingInfo.rgenShaderBindingTable
	    = {.deviceAddress = shaderBindingTableBufferDeviceAddress + rayGenOffset,
	       .stride = progSize,
	       .size = progSize};

	raytracingInfo.rmissShaderBindingTable
	    = {.deviceAddress = shaderBindingTableBufferDeviceAddress + missOffset,
	       .stride = progSize,
	       .size = progSize * 2};

	raytracingInfo.callableShaderBindingTable = {};
}

inline void recordRaytracingCommandBuffer(VkPhysicalDevice physicalDevice,
                                          VkDevice logicalDevice,
                                          VkCommandBuffer commandBuffer,
                                          RaytracingInfo& raytracingInfo,
                                          std::shared_ptr<ltracer::Window> window)
{

	buildRaytracingAccelerationStructures(logicalDevice, raytracingInfo, commandBuffer);

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
	                        raytracingInfo.descriptorSetHandleList.size(),
	                        raytracingInfo.descriptorSetHandleList.data(),
	                        0,
	                        NULL);

	auto currentExtent = window->getSwapChainExtent();
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

// does the raytracing onto the provided image
inline void
updateRaytraceBuffer(VkDevice logicalDevice, std::shared_ptr<ltracer::Camera> camera, VkImage image)
{
	if (camera->isCameraMoved())
	{
		uniformStructure.cameraPosition[0] = camera->transform.position[0];
		uniformStructure.cameraPosition[1] = camera->transform.position[1];
		uniformStructure.cameraPosition[2] = camera->transform.position[2];

		uniformStructure.cameraForward[0] = camera->transform.getForward()[0];
		uniformStructure.cameraForward[1] = camera->transform.getForward()[1];
		uniformStructure.cameraForward[2] = camera->transform.getForward()[2];

		uniformStructure.cameraRight[0] = camera->transform.getRight()[0];
		uniformStructure.cameraRight[1] = camera->transform.getRight()[1];
		uniformStructure.cameraRight[2] = camera->transform.getRight()[2];

		uniformStructure.cameraUp[0] = camera->transform.getUp()[0];
		uniformStructure.cameraUp[1] = camera->transform.getUp()[1];
		uniformStructure.cameraUp[2] = camera->transform.getUp()[2];

		uniformStructure.frameCount = 0;

		camera->resetCameraMoved();
	}
	else
	{
		uniformStructure.frameCount += 1;
	}

	// TODO: we need some synchronization here probably
	VkResult result = vkMapMemory(logicalDevice,
	                              uniformDeviceMemoryHandle,
	                              0,
	                              sizeof(UniformStructure),
	                              0,
	                              &hostUniformMemoryBuffer);

	memcpy(hostUniformMemoryBuffer, &uniformStructure, sizeof(UniformStructure));

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, uniformDeviceMemoryHandle);
}

inline void freeRaytraceImageAndImageView(VkDevice logicalDevice,
                                          VkImage& rayTraceImageHandle,
                                          VkImageView& rayTraceImageViewHandle)
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

inline void createRaytracingImage(VkPhysicalDevice physicalDevice,
                                  VkDevice logicalDevice,
                                  VkFormat swapChainFormat,
                                  VkExtent2D currentExtent,
                                  ltracer::QueueFamilyIndices queueFamilyIndices,
                                  VkImage& rayTraceImageHandle)
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
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndices.graphicsFamily.value(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

	VkResult result
	    = vkCreateImage(logicalDevice, &rayTraceImageCreateInfo, NULL, &rayTraceImageHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkCreateImage");
	}

	VkMemoryRequirements rayTraceImageMemoryRequirements;
	vkGetImageMemoryRequirements(
	    logicalDevice, rayTraceImageHandle, &rayTraceImageMemoryRequirements);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	uint32_t rayTraceImageMemoryTypeIndex = -1;
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
	result = vkAllocateMemory(
	    logicalDevice, &rayTraceImageMemoryAllocateInfo, NULL, &rayTraceImageDeviceMemoryHandle);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkAllocateMemory");
	}

	result
	    = vkBindImageMemory(logicalDevice, rayTraceImageHandle, rayTraceImageDeviceMemoryHandle, 0);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkBindImageMemory");
	}
}

inline void createRaytracingImageView(VkDevice logicalDevice,
                                      VkFormat swapChainFormat,
                                      VkImage& rayTraceImageHandle,
                                      VkImageView& rayTraceImageViewHandle)
{
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
}

inline void recreateRaytracingImageBuffer(VkDevice logicalDevice,
                                          VkPhysicalDevice physicalDevice,
                                          std::shared_ptr<Window> window,
                                          QueueFamilyIndices queueFamilyIndices,
                                          RaytracingInfo& raytracingInfo)
{
	freeRaytraceImageAndImageView(
	    logicalDevice, raytracingInfo.rayTraceImageHandle, raytracingInfo.rayTraceImageViewHandle);

	createRaytracingImage(physicalDevice,
	                      logicalDevice,
	                      window->getSwapChainImageFormat(),
	                      window->getSwapChainExtent(),
	                      queueFamilyIndices,
	                      raytracingInfo.rayTraceImageHandle);

	createRaytracingImageView(logicalDevice,
	                          window->getSwapChainImageFormat(),
	                          raytracingInfo.rayTraceImageHandle,
	                          raytracingInfo.rayTraceImageViewHandle);

	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingInfo);

	// reset frame count so the window gets refreshed properly
	uniformStructure.frameCount = 0;
}

} // namespace rt
} // namespace ltracer
