

#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
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
#include "src/model.hpp"
#include "src/shader_module.hpp"
#include "src/tlas.hpp"
#include "src/types.hpp"
#include "src/window.hpp"
#include "src/device_procedures.hpp"

#include "tiny_obj_loader.h"

namespace ltracer
{
namespace rt
{

// TODO: figure out a better way to handle this
static VkQueue graphicsQueueHandle = VK_NULL_HANDLE;

static std::vector<MeshObject> meshObjects;

inline void updateUniformStructure(RaytracingInfo& raytracingInfo,
                                   const glm::vec3& position,
                                   const glm::vec3& forward,
                                   const glm::vec3& up,
                                   const glm::vec3& right)
{
	raytracingInfo.uniformStructure.cameraPosition[0] = position[0];
	raytracingInfo.uniformStructure.cameraPosition[1] = position[1];
	raytracingInfo.uniformStructure.cameraPosition[2] = position[2];

	raytracingInfo.uniformStructure.cameraForward[0] = forward[0];
	raytracingInfo.uniformStructure.cameraForward[1] = forward[1];
	raytracingInfo.uniformStructure.cameraForward[2] = forward[2];

	raytracingInfo.uniformStructure.cameraUp[0] = up[0];
	raytracingInfo.uniformStructure.cameraUp[1] = up[1];
	raytracingInfo.uniformStructure.cameraUp[2] = up[2];

	raytracingInfo.uniformStructure.cameraRight[0] = right[0];
	raytracingInfo.uniformStructure.cameraRight[1] = right[1];
	raytracingInfo.uniformStructure.cameraRight[2] = right[2];
}

inline void resetFrameCount(RaytracingInfo& raytracingInfo)
{
	raytracingInfo.uniformStructure.frameCount = 0;
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
                                                  RaytracingInfo& raytracingInfo,
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
                              RaytracingInfo& raytracingInfo,
                              VkCommandPool& commandBufferPoolHandle)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
	};

	VkResult result = vkCreateCommandPool(
	    logicalDevice, &commandPoolCreateInfo, NULL, &commandBufferPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateCommandPool failed");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyCommandPool(logicalDevice, commandBufferPoolHandle, nullptr); });
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
		                         commandBufferPoolHandle,
		                         1,
		                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
	    });
}

inline void createDescriptorPool(DeletionQueue& deletionQueue,
                                 VkDevice logicalDevice,
                                 VkDescriptorPool& descriptorPoolHandle)
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

inline void createDescriptorSetLayout(DeletionQueue& deletionQueue,
                                      VkDevice logicalDevice,
                                      VkDescriptorSetLayout& descriptorSetLayoutHandle)
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

inline void
createMaterialDescriptorSetLayout(DeletionQueue& deletionQueue,
                                  VkDevice logicalDevice,
                                  VkDescriptorSetLayout& materialDescriptorSetLayoutHandle)
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
                             VkDevice logicalDevice,
                             RaytracingInfo& raytracingInfo,
                             VkDescriptorPool& descriptorPoolHandle)
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

inline void loadShaderModules(DeletionQueue& deletionQueue,
                              VkDevice logicalDevice,
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
	        .module = raytracingInfo.rayClosestHitShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	        .module = raytracingInfo.rayGenerateShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	        .module = raytracingInfo.rayMissShaderModuleHandle,
	        .pName = "main",
	        .pSpecializationInfo = NULL},
	       {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .pNext = NULL,
	        .flags = 0,
	        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	        .module = raytracingInfo.rayMissShadowShaderModuleHandle,
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

inline void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                                     RaytracingInfo& raytracingInfo,
                                                     const std::vector<MeshObject>& meshObjects)
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

	std::vector<VkWriteDescriptorSet> writeDescriptorSetList;

	// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
	// descriptor set, for now we are using the same vertex and index buffer
	VkDescriptorBufferInfo indexDescriptorInfo = {
	    .buffer = meshObjects[0].indexBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
	// descriptor set, for now we are using the same vertex and index buffer
	VkDescriptorBufferInfo vertexDescriptorInfo = {
	    .buffer = meshObjects[0].vertexBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorImageInfo rayTraceImageDescriptorInfo = {
	    .sampler = VK_NULL_HANDLE,
	    .imageView = raytracingInfo.rayTraceImageViewHandle,
	    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	writeDescriptorSetList = {
	    {
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
	    },
	    {
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
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 2,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &indexDescriptorInfo,
	        .pTexelBufferView = NULL,
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = raytracingInfo.descriptorSetHandleList[0],
	        .dstBinding = 3,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .pImageInfo = NULL,
	        .pBufferInfo = &vertexDescriptorInfo,
	        .pTexelBufferView = NULL,
	    },
	    {
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
	    },
	};

	vkUpdateDescriptorSets(
	    logicalDevice, writeDescriptorSetList.size(), writeDescriptorSetList.data(), 0, NULL);
}

inline void loadAndCreateVertexAndIndexBufferForModel(VkDevice logicalDevice,
                                                      VkPhysicalDevice physicalDevice,
                                                      DeletionQueue& deletionQueue,
                                                      RaytracingInfo& raytracingInfo,
                                                      MeshObject& meshObject)
{

	// =========================================================================
	// Vertex Buffer
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(float) * meshObject.vertices.size() * 3,
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             meshObject.vertexBufferHandle,
	             meshObject.vertexBufferDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostVertexMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              meshObject.vertexBufferDeviceMemoryHandle,
	                              0,
	                              sizeof(float) * meshObject.vertices.size() * 3,
	                              0,
	                              &hostVertexMemoryBuffer);

	memcpy(hostVertexMemoryBuffer,
	       meshObject.vertices.data(),
	       sizeof(float) * meshObject.vertices.size() * 3);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, meshObject.vertexBufferDeviceMemoryHandle);

	VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = meshObject.vertexBufferHandle};

	meshObject.vertexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &vertexBufferDeviceAddressInfo);

	// =========================================================================
	// Index Buffer

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(uint32_t) * meshObject.indices.size(),
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             meshObject.indexBufferHandle,
	             meshObject.indexBufferDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostIndexMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     meshObject.indexBufferDeviceMemoryHandle,
	                     0,
	                     sizeof(uint32_t) * meshObject.indices.size(),
	                     0,
	                     &hostIndexMemoryBuffer);

	memcpy(hostIndexMemoryBuffer,
	       meshObject.indices.data(),
	       sizeof(uint32_t) * meshObject.indices.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, meshObject.indexBufferDeviceMemoryHandle);

	VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = meshObject.indexBufferHandle};

	meshObject.indexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &indexBufferDeviceAddressInfo);
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
	if (raytracingInfo.queueFamilyIndices.graphicsFamily.has_value())
	{
		vkGetDeviceQueue(logicalDevice,
		                 raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
		                 0,
		                 &graphicsQueueHandle);
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

	VkCommandPool commandBufferPoolHandle = VK_NULL_HANDLE;
	createCommandPool(logicalDevice, deletionQueue, raytracingInfo, commandBufferPoolHandle);

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

	VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
	createDescriptorPool(deletionQueue, logicalDevice, descriptorPoolHandle);

	// =========================================================================
	// Descriptor Set Layout

	VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
	createDescriptorSetLayout(deletionQueue, logicalDevice, descriptorSetLayoutHandle);

	// =========================================================================
	// Material Descriptor Set Layout

	VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
	createMaterialDescriptorSetLayout(
	    deletionQueue, logicalDevice, materialDescriptorSetLayoutHandle);

	// =========================================================================
	// Allocate Descriptor Sets

	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList
	    = {descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

	allocateDescriptorSetLayouts(
	    descriptorSetLayoutHandleList, logicalDevice, raytracingInfo, descriptorPoolHandle);

	// =========================================================================
	// Pipeline Layout
	createPipelineLayout(
	    deletionQueue, raytracingInfo, descriptorSetLayoutHandleList, logicalDevice);
	// =========================================================================
	// Ray Closest Hit Shader Module
	loadShaderModules(deletionQueue, logicalDevice, raytracingInfo);

	// =========================================================================
	// Ray Tracing Pipeline
	createRaytracingPipeline(deletionQueue, raytracingInfo, logicalDevice);

	// =========================================================================
	// load OBJ Model

	std::vector<VkDeviceAddress> bottomLevelAccelerationStructureDeviceAddresses;

	std::string scenePath = "3d-models/cube_scene.obj";
	{
		MeshObject meshObject = MeshObject::loadFromPath(scenePath);
		meshObjects.push_back(std::move(meshObject));
	}
	{
		MeshObject meshObject2 = MeshObject::loadFromPath(scenePath);
		// meshObjects.push_back(std::move(meshObject2));
	}

	for (auto&& obj : meshObjects)
	{
		loadAndCreateVertexAndIndexBufferForModel(
		    logicalDevice, physicalDevice, deletionQueue, raytracingInfo, obj);

		// =========================================================================
		// Bottom Level Acceleration Structure
		auto triangleBLAS
		    = createAndBuildBottomLevelAccelerationStructureTriangle(deletionQueue,
		                                                             obj.indexBufferDeviceAddress,
		                                                             logicalDevice,
		                                                             physicalDevice,
		                                                             obj.primitiveCount,
		                                                             obj.vertices.size(),
		                                                             obj.vertexBufferDeviceAddress,
		                                                             raytracingInfo,
		                                                             graphicsQueueHandle);
		bottomLevelAccelerationStructureDeviceAddresses.push_back(triangleBLAS);
	}

	// auto aabbBLAS = createAndBuildBottomLevelAccelerationStructureAABB(
	//     deletionQueue, logicalDevice, physicalDevice, raytracingInfo);
	// bottomLevelAccelerationStructureDeviceAddresses.push_back(aabbBLAS);

	// vkResetCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
	// 0);

	// =========================================================================
	// Top Level Acceleration Structure
	std::vector<VkDeviceAddress> accelerationStructureInstanceAddresses;
	createAndBuildTopLevelAccelerationStructure(bottomLevelAccelerationStructureDeviceAddresses,
	                                            deletionQueue,
	                                            logicalDevice,
	                                            physicalDevice,
	                                            raytracingInfo,
	                                            graphicsQueueHandle,
	                                            accelerationStructureInstanceAddresses);

	// =========================================================================
	// Uniform Buffer

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(UniformStructure),
	             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             raytracingInfo.uniformBufferHandle,
	             raytracingInfo.uniformDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostUniformMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     raytracingInfo.uniformDeviceMemoryHandle,
	                     0,
	                     sizeof(UniformStructure),
	                     0,
	                     &hostUniformMemoryBuffer);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}
	memcpy(hostUniformMemoryBuffer, &raytracingInfo.uniformStructure, sizeof(UniformStructure));

	vkUnmapMemory(logicalDevice, raytracingInfo.uniformDeviceMemoryHandle);

	// =========================================================================
	// Update Descriptor Set
	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingInfo, meshObjects);

	// =========================================================================
	// Material Index Buffer

	// TODO: Material list needs to include all possible materials from all objects, for now we only
	// have one object
	std::vector<uint32_t> materialIndexList;
	for (tinyobj::shape_t shape : meshObjects[0].shapes)
	{
		for (int index : shape.mesh.material_ids)
		{
			materialIndexList.push_back(index);
		}
	}

	VkBuffer materialIndexBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory materialIndexDeviceMemoryHandle = VK_NULL_HANDLE;
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

	// TODO: Material list needs to include all possible materials from all objects, for now we only
	// have one object
	std::vector<Material> materialList(meshObjects[0].materials.size());
	for (uint32_t i = 0; i < meshObjects[0].materials.size(); i++)
	{
		memcpy(materialList[i].ambient, meshObjects[0].materials[i].ambient, sizeof(float) * 3);
		memcpy(materialList[i].diffuse, meshObjects[0].materials[i].diffuse, sizeof(float) * 3);
		memcpy(materialList[i].specular, meshObjects[0].materials[i].specular, sizeof(float) * 3);
		memcpy(materialList[i].emission, meshObjects[0].materials[i].emission, sizeof(float) * 3);
	}

	VkBuffer materialBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory materialDeviceMemoryHandle = VK_NULL_HANDLE;
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
inline void updateRaytraceBuffer(VkDevice logicalDevice,
                                 RaytracingInfo& raytracingInfo,
                                 std::shared_ptr<ltracer::Camera> camera)
{
	if (camera->isCameraMoved())
	{
		updateUniformStructure(raytracingInfo,
		                       camera->transform.position,
		                       camera->transform.getForward(),
		                       camera->transform.getUp(),
		                       camera->transform.getRight());
		resetFrameCount(raytracingInfo);

		camera->resetCameraMoved();
	}
	else
	{
		raytracingInfo.uniformStructure.frameCount += 1;
	}

	// TODO: we need some synchronization here probably
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

inline void freeRaytraceImageAndImageView(VkDevice logicalDevice,
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

inline void createRaytracingImage(VkPhysicalDevice physicalDevice,
                                  VkDevice logicalDevice,
                                  VkFormat swapChainFormat,
                                  VkExtent2D currentExtent,
                                  ltracer::QueueFamilyIndices queueFamilyIndices,
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
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndices.graphicsFamily.value(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

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
	freeRaytraceImageAndImageView(logicalDevice,
	                              raytracingInfo.rayTraceImageHandle,
	                              raytracingInfo.rayTraceImageViewHandle,
	                              raytracingInfo.rayTraceImageDeviceMemoryHandle);

	createRaytracingImage(physicalDevice,
	                      logicalDevice,
	                      window->getSwapChainImageFormat(),
	                      window->getSwapChainExtent(),
	                      queueFamilyIndices,
	                      raytracingInfo);

	createRaytracingImageView(logicalDevice,
	                          window->getSwapChainImageFormat(),
	                          raytracingInfo.rayTraceImageHandle,
	                          raytracingInfo.rayTraceImageViewHandle);

	updateAccelerationStructureDescriptorSet(logicalDevice, raytracingInfo, meshObjects);

	// reset frame count so the window gets refreshed properly
	resetFrameCount(raytracingInfo);
}

} // namespace rt
} // namespace ltracer
