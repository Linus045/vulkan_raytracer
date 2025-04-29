#include <cassert>
#include <cstdio>
#include <string>
#include <array>

#include <vulkan/vulkan_core.h>

#include <glm/detail/qualifier.hpp>

#include "blas.hpp"
#include "common_types.h"
#include "deletion_queue.hpp"
#include "raytracing.hpp"
#include "logger.hpp"
#include "raytracing_scene.hpp"
#include "raytracing_worldobject.hpp"
#include "shader_module.hpp"
#include "tetrahedron.hpp"
#include "visualizations.hpp"
#include "device_procedures.hpp"
#include "vk_utils.hpp"

namespace tracer
{
namespace rt
{

void requestRaytracingProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties,
    VkPhysicalDeviceAccelerationStructurePropertiesKHR&
        physicalDeviceAccelerationStructureProperties)
{
	VkPhysicalDeviceProperties physicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	physicalDeviceRayTracingPipelineProperties.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	physicalDeviceRayTracingPipelineProperties.pNext
	    = &physicalDeviceAccelerationStructureProperties;

	physicalDeviceAccelerationStructureProperties.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	physicalDeviceAccelerationStructureProperties.pNext = NULL;

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

	VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice,
	                                         &commandBufferAllocateInfo,
	                                         &raytracingInfo.commandBufferBuildTopAndBottomLevel));

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    {
		    vkFreeCommandBuffers(logicalDevice,
		                         commandBufferPoolHandle,
		                         1,
		                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
	    });
}

void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                              const rt::RaytracingScene& raytracingScene,
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
	    .buffer = raytracingScene.gpuObjectsBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo spheresDescriptorInfo = {
	    .buffer = raytracingScene.spheresBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	// VkDescriptorBufferInfo tetrahedronsDescriptorInfo = {
	//     .buffer = raytracingScene.tetrahedronsBufferHandle,
	//     .offset = 0,
	//     .range = VK_WHOLE_SIZE,
	// };

	VkDescriptorBufferInfo rectangularBezierSurfaces2x2DescriptorInfo = {
	    .buffer = raytracingScene.rectangularBezierSurfaces2x2BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo slicingPlanesDescriptorInfo = {
	    .buffer = raytracingScene.slicingPlanesBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo bezierTriangles2DescriptorInfo = {
	    .buffer = raytracingScene.bezierTriangles2BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo bezierTriangles3DescriptorInfo = {
	    .buffer = raytracingScene.bezierTriangles3BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo bezierTriangles4DescriptorInfo = {
	    .buffer = raytracingScene.bezierTriangles4BufferHandle,
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

	// if (raytracingScene.getObjectBuffers().tetrahedronsBufferHandle != VK_NULL_HANDLE)
	// {
	// 	writeDescriptorSetList.push_back({
	// 	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	    .pNext = NULL,
	// 	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	// 	    .dstBinding = 5,
	// 	    .dstArrayElement = 0,
	// 	    .descriptorCount = 1,
	// 	    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	// 	    .pImageInfo = NULL,
	// 	    .pBufferInfo = &tetrahedronsDescriptorInfo,
	// 	    .pTexelBufferView = NULL,
	// 	});
	// }

	if (raytracingScene.spheresBufferHandle != VK_NULL_HANDLE)
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

	if (raytracingScene.rectangularBezierSurfaces2x2BufferHandle != VK_NULL_HANDLE)
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

	if (raytracingScene.slicingPlanesBufferHandle != VK_NULL_HANDLE)
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

	if (raytracingScene.gpuObjectsBufferHandle != VK_NULL_HANDLE)
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

	if (raytracingScene.bezierTriangles2BufferHandle != VK_NULL_HANDLE)
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

	if (raytracingScene.bezierTriangles3BufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 11,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &bezierTriangles3DescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingScene.bezierTriangles4BufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 12,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &bezierTriangles4DescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	vkUpdateDescriptorSets(logicalDevice,
	                       static_cast<uint32_t>(writeDescriptorSetList.size()),
	                       writeDescriptorSetList.data(),
	                       0,
	                       NULL);
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
	    .maxPipelineRayRecursionDepth = 2, // level of 2 needed for shadow rays
	    .pLibraryInfo = NULL,
	    .pLibraryInterface = NULL,
	    .pDynamicState = NULL,
	    .layout = raytracingInfo.pipelineLayoutHandle,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = 0,
	};

	VK_CHECK_RESULT(tracer::procedures::pvkCreateRayTracingPipelinesKHR(
	    logicalDevice,
	    VK_NULL_HANDLE,
	    VK_NULL_HANDLE,
	    1,
	    &rayTracingPipelineCreateInfo,
	    NULL,
	    &raytracingInfo.rayTracingPipelineHandle));

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    { vkDestroyPipeline(logicalDevice, raytracingInfo.rayTracingPipelineHandle, NULL); });
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
// 		throw std::runtime_error("initRayTracing - vkMapMemory");
// 	}

// 	vkUnmapMemory(logicalDevice, meshObject.vertexBufferDeviceMemoryHandle);

// 	VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
// 	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
// 	       .pNext = NULL,
// 	       .buffer = meshObject.vertexBufferHandle};

// 	meshObject.vertexBufferDeviceAddress = tracer::procedures::pvkGetBufferDeviceAddressKHR(
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
// 		throw std::runtime_error("initRayTracing - vkMapMemory");
// 	}

// 	vkUnmapMemory(logicalDevice, meshObject.indexBufferDeviceMemoryHandle);

// 	VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
// 	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
// 	       .pNext = NULL,
// 	       .buffer = meshObject.indexBufferHandle};

// 	meshObject.indexBufferDeviceAddress = tracer::procedures::pvkGetBufferDeviceAddressKHR(
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
                    VmaAllocator vmaAllocator,
                    DeletionQueue& deletionQueue,
                    RaytracingInfo& raytracingInfo,
                    VkPhysicalDeviceAccelerationStructurePropertiesKHR&
                        physicalDeviceAccelerationStructureProperties)
{
	// Requesting acceleration Structure properties

	// Requesting ray tracing properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{};

	requestRaytracingProperties(physicalDevice,
	                            physicalDeviceRayTracingPipelineProperties,
	                            physicalDeviceAccelerationStructureProperties);

	// clang-format off
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxGeometryCount: {%ld}\n", physicalDeviceAccelerationStructureProperties.maxGeometryCount);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxInstanceCount: {%ld}\n", physicalDeviceAccelerationStructureProperties.maxInstanceCount);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxPrimitiveCount: {%ld}\n", physicalDeviceAccelerationStructureProperties.maxPrimitiveCount);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxPerStageDescriptorAccelerationStructures: {%d}\n", physicalDeviceAccelerationStructureProperties.maxPerStageDescriptorAccelerationStructures);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures: {%d}\n", physicalDeviceAccelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxDescriptorSetAccelerationStructures {%d}\n", physicalDeviceAccelerationStructureProperties.maxDescriptorSetAccelerationStructures);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures: {%d}\n", physicalDeviceAccelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures);
	debug_printFmt("physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment: {%d}\n", physicalDeviceAccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);
	// clang-format onn

	debug_printFmt("physicalDeviceRayTracingPipelineProperties.maxRayRecursionDepth: {%d}\n",
	               physicalDeviceRayTracingPipelineProperties.maxRayRecursionDepth);
	// make sure that a recursion depth of at least 2 is supported (used for shadow rays)
	if (physicalDeviceRayTracingPipelineProperties.maxRayRecursionDepth <= 1)
	{
		throw std::runtime_error(
		    "Error: Device fails to support ray recursion (maxRayRecursionDepth <= 1)");
	}

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

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
	// Device Pointer Functions
	tracer::procedures::grabDeviceProcAddr(logicalDevice);

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
	//   throw std::runtime_error(
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
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList = {
	    descriptorSetLayoutHandle,
	    materialDescriptorSetLayoutHandle,
	};

	raytracingInfo.descriptorSetHandleList = allocateDescriptorSetLayouts(
	    logicalDevice, descriptorPoolHandle, descriptorSetLayoutHandleList);

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
	// for (int x = 0; x <= 15; x++)
	//	for (int y = 0; y <= 15; y++)
	//}

	// float u = 0.4f;
	// float v = 0.2f;
	// float w = 1.0f - u - v;

	// glm::vec3 point = deCasteljauBezierTrianglePoint(bezierTriangleH1.controlPoints, u,
	// v, w); raytracingScene.addSphere(point, 0.1f, ColorIdx::t_white);

	// Visualize control points
	// for (auto& point : tetrahedron2.controlPoints)
	//{
	//	//		raytracingScene.addSphere(point, 0.05f, ColorIdx::t_black);
	//}

	// create fence for building acceleration structure
	VkFenceCreateInfo bottomLevelAccelerationStructureBuildFenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};

	VK_CHECK_RESULT(vkCreateFence(logicalDevice,
	                              &bottomLevelAccelerationStructureBuildFenceCreateInfo,
	                              NULL,
	                              &raytracingInfo.accelerationStructureBuildFence));
	// vkResetFences(logicalDevice, 1, &raytracingInfo.accelerationStructureBuildFence);

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyFence(logicalDevice, raytracingInfo.accelerationStructureBuildFence, NULL); });

	// TODO: for now we don't change the amount of objects in the scene
	// so we can create the buffers only once
	// if we want to add/remove objects from the scene we need to recreate the buffers
	// raytracingScene.createBuffers();
	// raytracingScene.createSlicingPlanesBuffer();

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
	// 		throw std::runtime_error("initRayTraci - vkMapMemory");
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
	// 		throw std::runtime_error("initRayTraci - vkMapMemory");
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
	VmaAllocation shaderBindingTableBufferAllocation = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             vmaAllocator,
	             deletionQueue,
	             shaderBindingTableSize,
	             VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             shaderBindingTableBufferHandle,
	             shaderBindingTableBufferAllocation);

	char* shaderHandleBuffer = new char[shaderBindingTableSize];
	VK_CHECK_RESULT(tracer::procedures::pvkGetRayTracingShaderGroupHandlesKHR(

	    logicalDevice,
	    raytracingInfo.rayTracingPipelineHandle,
	    0,
	    raytracingInfo.shaderGroupCount,
	    shaderBindingTableSize,
	    shaderHandleBuffer));

	void* hostShaderBindingTableMemoryBuffer;
	VK_CHECK_RESULT(vmaMapMemory(
	    vmaAllocator, shaderBindingTableBufferAllocation, &hostShaderBindingTableMemoryBuffer));

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

	vmaUnmapMemory(vmaAllocator, shaderBindingTableBufferAllocation);

	VkBufferDeviceAddressInfo shaderBindingTableBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = shaderBindingTableBufferHandle,
	};

	VkDeviceAddress shaderBindingTableBufferDeviceAddress
	    = tracer::procedures::pvkGetBufferDeviceAddressKHR(
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
	// auto subResourceRange = VkImageSubresourceRange{
	//     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	//     .baseMipLevel = 0,
	//     .levelCount = 1,
	//     .baseArrayLayer = 0,
	//     .layerCount = 1,
	// };
	// addImageMemoryBarrier(commandBuffer,
	//                       VK_PIPELINE_STAGE_2_NONE,
	//                       VK_ACCESS_2_SHADER_WRITE_BIT,
	//                       VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	//                       VK_ACCESS_2_SHADER_READ_BIT,
	//                       VK_IMAGE_LAYOUT_GENERAL,
	//                       VK_IMAGE_LAYOUT_GENERAL,
	//                       subResourceRange,
	//                       raytracingInfo.rayTraceImageHandle);

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

	tracer::procedures::pvkCmdTraceRaysKHR(commandBuffer,
	                                       &raytracingInfo.rgenShaderBindingTable,
	                                       &raytracingInfo.rmissShaderBindingTable,
	                                       &raytracingInfo.rchitShaderBindingTable,
	                                       &raytracingInfo.callableShaderBindingTable,
	                                       currentExtent.width,
	                                       currentExtent.height,
	                                       1);

	// addImageMemoryBarrier(commandBuffer,
	//                       VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
	//                       VK_ACCESS_2_SHADER_WRITE_BIT,
	//                       VK_PIPELINE_STAGE_2_TRANSFER_BIT,
	//                       VK_ACCESS_2_SHADER_READ_BIT,
	//                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//                       subResourceRange,
	//                       raytracingInfo.rayTraceImageHandle);
}

} // namespace rt

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
	VK_CHECK_RESULT(
	    vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, NULL, &commandBufferPoolHandle));

	deletionQueue.push_function(
	    [=]() { vkDestroyCommandPool(logicalDevice, commandBufferPoolHandle, nullptr); });

	return commandBufferPoolHandle;
}

VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, DeletionQueue& deletionQueue)
{
	VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
	    {.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 12},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1},
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = 2,
	    .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizeList.size()),
	    .pPoolSizes = descriptorPoolSizeList.data(),
	};

	VK_CHECK_RESULT(vkCreateDescriptorPool(
	    logicalDevice, &descriptorPoolCreateInfo, NULL, &descriptorPoolHandle));

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorPool(logicalDevice, descriptorPoolHandle, NULL); });

	return descriptorPoolHandle;
}

std::vector<VkDescriptorSet>
allocateDescriptorSetLayouts(VkDevice logicalDevice,
                             VkDescriptorPool& descriptorPoolHandle,
                             std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = NULL,
	    .descriptorPool = descriptorPoolHandle,
	    .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	};

	std::vector<VkDescriptorSet> sets(descriptorSetLayoutHandleList.size(), VK_NULL_HANDLE);

	VkResult result
	    = vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocateInfo, sets.data());

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("initRayTracing - vkAllocateDescriptorSets");
	}

	return sets;
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
	VK_CHECK_RESULT(vkCreatePipelineLayout(
	    logicalDevice, &pipelineLayoutCreateInfo, NULL, &raytracingInfo.pipelineLayoutHandle));

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
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR
	                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
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
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	                      | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
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
	    {
	        .binding = 11,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 12,
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

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
	    logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayoutHandle));

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

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice,
	                                            &materialDescriptorSetLayoutCreateInfo,
	                                            NULL,
	                                            &materialDescriptorSetLayoutHandle));

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyDescriptorSetLayout(logicalDevice, materialDescriptorSetLayoutHandle, NULL); });

	return materialDescriptorSetLayoutHandle;
}

VkDescriptorSetLayout createRaytracingImageDescriptorSetLayout(VkDevice logicalDevice,
                                                               DeletionQueue& deletionQueue)
{
	VkDescriptorSetLayout descriptorSet = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	        .pImmutableSamplers = NULL,
	    },
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .bindingCount = (uint32_t)descriptorSetLayoutBindings.size(),
	    .pBindings = descriptorSetLayoutBindings.data(),
	};

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(
	    logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSet));

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorSetLayout(logicalDevice, descriptorSet, NULL); });

	return descriptorSet;
}

void updateRaytraceBuffer([[maybe_unused]] VkDevice logicalDevice,
                          VmaAllocator vmaAllocator,
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
	if (raytracingInfo.uniformBufferAllocation != VK_NULL_HANDLE)
	{
		void* hostUniformMemoryBuffer;
		VK_CHECK_RESULT(vmaMapMemory(
		    vmaAllocator, raytracingInfo.uniformBufferAllocation, &hostUniformMemoryBuffer));

		memcpy(hostUniformMemoryBuffer, &raytracingInfo.uniformStructure, sizeof(UniformStructure));

		vmaUnmapMemory(vmaAllocator, raytracingInfo.uniformBufferAllocation);
	}
}

void loadShaderModules(VkDevice logicalDevice,
                       DeletionQueue& deletionQueue,
                       RaytracingInfo& raytracingInfo)
{

	// =========================================================================
	// Ray Closest Hit Shader Module
	tracer::shader::createShaderModule("shaders/shader.rchit.spv",
	                                   logicalDevice,
	                                   deletionQueue,
	                                   raytracingInfo.rayClosestHitShaderModuleHandle);

	// =========================================================================
	// Ray Generate Shader Module
	tracer::shader::createShaderModule("shaders/shader.rgen.spv",
	                                   logicalDevice,
	                                   deletionQueue,
	                                   raytracingInfo.rayGenerateShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module

	tracer::shader::createShaderModule("shaders/shader.rmiss.spv",
	                                   logicalDevice,
	                                   deletionQueue,
	                                   raytracingInfo.rayMissShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module (Shadow)

	tracer::shader::createShaderModule("shaders/shader_shadow.rmiss.spv",
	                                   logicalDevice,
	                                   deletionQueue,
	                                   raytracingInfo.rayMissShadowShaderModuleHandle);

	// =========================================================================
	// Ray AABB Intersection Module
	tracer::shader::createShaderModule("shaders/shader_aabb.rint.spv",
	                                   logicalDevice,
	                                   deletionQueue,
	                                   raytracingInfo.rayAABBIntersectionModuleHandle);
}

void createRaytracingImage([[maybe_unused]] VkPhysicalDevice physicalDevice,
                           VkDevice logicalDevice,
                           VmaAllocator vmaAllocator,
                           VkExtent2D currentExtent,
                           RaytracingInfo& raytracingInfo)
{
	VkImageCreateInfo rayTraceImageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
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
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.memoryTypeBits = 0; // no restrictions
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocInfo.flags = 0;

	VmaAllocationInfo allocationInfo = {};
	VK_CHECK_RESULT(vmaCreateImage(vmaAllocator,
	                               &rayTraceImageCreateInfo,
	                               &allocInfo,
	                               &raytracingInfo.rayTraceImageHandle,
	                               &raytracingInfo.rayTraceImageDeviceMemoryHandle,
	                               &allocationInfo));

	// transition image layout
	prepareRaytracingImageLayout(logicalDevice, raytracingInfo);
}

void prepareRaytracingImageLayout(VkDevice logicalDevice, const RaytracingInfo& raytracingInfo)
{
	VkCommandPool tmpCommandPool = VK_NULL_HANDLE;
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &tmpCommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error(
			    "prepareRaytracingImageLayout - failed to create command pool!");
		}
	}

	VkCommandBuffer tmpCommandBuffer = VK_NULL_HANDLE;
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = tmpCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &tmpCommandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Renderer::createRaytracingRenderpassAndFramebuffer - failed "
			                         "to allocate command buffers!");
		}
	}

	auto subresourceRange = VkImageSubresourceRange{
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = 0;

	VK_CHECK_RESULT(vkBeginCommandBuffer(tmpCommandBuffer, &beginInfo));

	addImageMemoryBarrier(tmpCommandBuffer,
	                      VK_PIPELINE_STAGE_2_NONE,
	                      VK_ACCESS_2_NONE,
	                      VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
	                      VK_ACCESS_2_SHADER_WRITE_BIT,
	                      VK_IMAGE_LAYOUT_UNDEFINED,
	                      VK_IMAGE_LAYOUT_GENERAL,
	                      subresourceRange,
	                      raytracingInfo.rayTraceImageHandle);

	VK_CHECK_RESULT(vkEndCommandBuffer(tmpCommandBuffer));

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &tmpCommandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(raytracingInfo.graphicsQueueHandle, 1, &submit, nullptr));
	VK_CHECK_RESULT(vkQueueWaitIdle(raytracingInfo.graphicsQueueHandle));

	vkFreeCommandBuffers(logicalDevice, tmpCommandPool, 1, &tmpCommandBuffer);
	vkDestroyCommandPool(logicalDevice, tmpCommandPool, nullptr);
}

VkImageView createRaytracingImageView(VkDevice logicalDevice, const VkImage& rayTraceImageHandle)
{
	VkImageView rayTraceImageViewHandle = VK_NULL_HANDLE;
	VkImageViewCreateInfo rayTraceImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = rayTraceImageHandle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
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

	VK_CHECK_RESULT(vkCreateImageView(
	    logicalDevice, &rayTraceImageViewCreateInfo, NULL, &rayTraceImageViewHandle));

	return rayTraceImageViewHandle;
}

void freeRaytraceImageAndImageView(VkDevice logicalDevice,
                                   VmaAllocator vmaAllocator,
                                   VkImage& rayTraceImageHandle,
                                   VkImageView& rayTraceImageViewHandle,
                                   VmaAllocation& rayTraceImageAllocation)
{
	assert(rayTraceImageHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	assert(rayTraceImageAllocation != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vmaDestroyImage(vmaAllocator, rayTraceImageHandle, rayTraceImageAllocation);

	assert(rayTraceImageViewHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkDestroyImageView(logicalDevice, rayTraceImageViewHandle, nullptr);
}

void recreateRaytracingImageBuffer(VkPhysicalDevice physicalDevice,
                                   VkDevice logicalDevice,
                                   VmaAllocator vmaAllocator,
                                   VkExtent2D windowExtent,
                                   RaytracingInfo& raytracingInfo)
{
	freeRaytraceImageAndImageView(logicalDevice,
	                              vmaAllocator,
	                              raytracingInfo.rayTraceImageHandle,
	                              raytracingInfo.rayTraceImageViewHandle,
	                              raytracingInfo.rayTraceImageDeviceMemoryHandle);

	createRaytracingImage(
	    physicalDevice, logicalDevice, vmaAllocator, windowExtent, raytracingInfo);

	raytracingInfo.rayTraceImageViewHandle
	    = createRaytracingImageView(logicalDevice, raytracingInfo.rayTraceImageHandle);
}

} // namespace tracer
