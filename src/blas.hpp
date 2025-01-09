#pragma once

#include <cstring>
#include <vulkan/vulkan_core.h>

#include "src/deletion_queue.hpp"
#include "src/types.hpp"
#include "src/device_procedures.hpp"

namespace ltracer
{
namespace rt
{

inline VkDeviceAddress
createAndBuildBottomLevelAccelerationStructureAABB(DeletionQueue& deletionQueue,
                                                   VkDevice logicalDevice,
                                                   VkPhysicalDevice physicalDevice,
                                                   RaytracingInfo& raytracingInfo)
{
	throw std::runtime_error("createAndBuildBottomLevelAccelerationStructureAABB not implemented");
}

// static VkDeviceMemory bottomLevelAccelerationStructureDeviceMemoryHandle = VK_NULL_HANDLE;

inline VkDeviceAddress
createAndBuildBottomLevelAccelerationStructureTriangle(DeletionQueue& deletionQueue,
                                                       VkDeviceAddress indexBufferDeviceAddress,
                                                       VkDevice logicalDevice,
                                                       VkPhysicalDevice physicalDevice,
                                                       uint32_t primitiveCount,
                                                       uint32_t verticesCount,
                                                       VkDeviceAddress vertexBufferDeviceAddress,
                                                       RaytracingInfo& raytracingInfo,
                                                       VkQueue queueHandle)
{
	// create Bottom Level Acceleration Structure
	VkAccelerationStructureGeometryDataKHR bottomLevelAccelerationStructureGeometryData = {
	    .triangles = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
			.pNext = NULL,
			.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
			.vertexData = {.deviceAddress = vertexBufferDeviceAddress},
			.vertexStride = sizeof(float) * 3,
			.maxVertex = verticesCount,
			.indexType = VK_INDEX_TYPE_UINT32,
			.indexData = {.deviceAddress = indexBufferDeviceAddress},
			.transformData = {.deviceAddress = 0},
		},
	};

	VkAccelerationStructureGeometryKHR bottomLevelAccelerationStructureGeometry = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = NULL,
	    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
	    .geometry = bottomLevelAccelerationStructureGeometryData,
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildGeometryInfoKHR bottomLevelAccelerationStructureBuildGeometryInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	       .pNext = NULL,
	       .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	       .flags = 0,
	       .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	       .srcAccelerationStructure = VK_NULL_HANDLE,
	       .dstAccelerationStructure = VK_NULL_HANDLE,
	       .geometryCount = 1,
	       .pGeometries = &bottomLevelAccelerationStructureGeometry,
	       .ppGeometries = NULL,
	       .scratchData = {.deviceAddress = 0}};

	VkAccelerationStructureBuildSizesInfoKHR bottomLevelAccelerationStructureBuildSizesInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	       .pNext = NULL,
	       .accelerationStructureSize = 0,
	       .updateScratchSize = 0,
	       .buildScratchSize = 0};

	std::vector<uint32_t> bottomLevelMaxPrimitiveCountList = {primitiveCount};

	ltracer::procedures::pvkGetAccelerationStructureBuildSizesKHR(
	    logicalDevice,
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &bottomLevelAccelerationStructureBuildGeometryInfo,
	    bottomLevelMaxPrimitiveCountList.data(),
	    &bottomLevelAccelerationStructureBuildSizesInfo);

	VkBuffer bottomLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory bottomLevelAccelerationStructureMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             bottomLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             bottomLevelAccelerationStructureBufferHandle,
	             bottomLevelAccelerationStructureMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	VkAccelerationStructureCreateInfoKHR bottomLevelAccelerationStructureCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	       .pNext = NULL,
	       .createFlags = 0,
	       .buffer = bottomLevelAccelerationStructureBufferHandle,
	       .offset = 0,
	       .size = bottomLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	       .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	       .deviceAddress = 0};

	VkAccelerationStructureKHR bottomLevelAccelerationStructureHandle = VK_NULL_HANDLE;
	VkResult result = ltracer::procedures::pvkCreateAccelerationStructureKHR(
	    logicalDevice,
	    &bottomLevelAccelerationStructureCreateInfo,
	    NULL,
	    &bottomLevelAccelerationStructureHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateAccelerationStructureKHR");
	}

	deletionQueue.push_function(
	    [=]()
	    {
		    ltracer::procedures::pvkDestroyAccelerationStructureKHR(
		        logicalDevice, bottomLevelAccelerationStructureHandle, NULL);
	    });

	// Build Bottom Level Acceleration Structure
	VkAccelerationStructureDeviceAddressInfoKHR bottomLevelAccelerationStructureDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	       .pNext = NULL,
	       .accelerationStructure = bottomLevelAccelerationStructureHandle};

	VkDeviceAddress bottomLevelAccelerationStructureDeviceAddress
	    = ltracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
	        logicalDevice, &bottomLevelAccelerationStructureDeviceAddressInfo);

	VkDeviceMemory bottomLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;
	VkBuffer bottomLevelAccelerationStructureScratchBufferHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             bottomLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             bottomLevelAccelerationStructureScratchBufferHandle,
	             bottomLevelAccelerationStructureDeviceScratchMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	VkBufferDeviceAddressInfo bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = bottomLevelAccelerationStructureScratchBufferHandle};

	VkDeviceAddress bottomLevelAccelerationStructureScratchBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo);

	bottomLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
	    = bottomLevelAccelerationStructureHandle;

	bottomLevelAccelerationStructureBuildGeometryInfo.scratchData
	    = {.deviceAddress = bottomLevelAccelerationStructureScratchBufferDeviceAddress};

	VkAccelerationStructureBuildRangeInfoKHR bottomLevelAccelerationStructureBuildRangeInfo
	    = {.primitiveCount = primitiveCount,
	       .primitiveOffset = 0,
	       .firstVertex = 0,
	       .transformOffset = 0};

	const VkAccelerationStructureBuildRangeInfoKHR* bottomLevelAccelerationStructureBuildRangeInfos
	    = &bottomLevelAccelerationStructureBuildRangeInfo;

	VkCommandBufferBeginInfo bottomLevelCommandBufferBeginInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = NULL,
	};

	result = vkBeginCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
	                              &bottomLevelCommandBufferBeginInfo);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkBeginCommandBuffer");
	}

	ltracer::procedures::pvkCmdBuildAccelerationStructuresKHR(
	    raytracingInfo.commandBufferBuildTopAndBottomLevel,
	    1,
	    &bottomLevelAccelerationStructureBuildGeometryInfo,
	    &bottomLevelAccelerationStructureBuildRangeInfos);

	result = vkEndCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkEndCommandBuffer");
	}

	VkSubmitInfo bottomLevelAccelerationStructureBuildSubmitInfo
	    = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	       .pNext = NULL,
	       .waitSemaphoreCount = 0,
	       .pWaitSemaphores = NULL,
	       .pWaitDstStageMask = NULL,
	       .commandBufferCount = 1,
	       .pCommandBuffers = &raytracingInfo.commandBufferBuildTopAndBottomLevel,
	       .signalSemaphoreCount = 0,
	       .pSignalSemaphores = NULL};

	VkFenceCreateInfo bottomLevelAccelerationStructureBuildFenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};

	VkFence bottomLevelAccelerationStructureBuildFenceHandle = VK_NULL_HANDLE;
	result = vkCreateFence(logicalDevice,
	                       &bottomLevelAccelerationStructureBuildFenceCreateInfo,
	                       NULL,
	                       &bottomLevelAccelerationStructureBuildFenceHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateFence");
	}

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyFence(logicalDevice, bottomLevelAccelerationStructureBuildFenceHandle, NULL); });

	result = vkQueueSubmit(queueHandle,
	                       1,
	                       &bottomLevelAccelerationStructureBuildSubmitInfo,
	                       bottomLevelAccelerationStructureBuildFenceHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkQueueSubmit");
	}

	result = vkWaitForFences(
	    logicalDevice, 1, &bottomLevelAccelerationStructureBuildFenceHandle, true, UINT32_MAX);

	if (result != VK_SUCCESS && result != VK_TIMEOUT)
	{
		throw new std::runtime_error("initRayTracing - vkWaitForFences");
	}

	return bottomLevelAccelerationStructureDeviceAddress;
}

inline void
createBottomLevelGeometryInstance(VkPhysicalDevice physicalDevice,
                                  VkDevice logicalDevice,
                                  DeletionQueue& deletionQueue,
                                  RaytracingInfo& raytracingInfo,
                                  VkDeviceAddress bottomLevelAccelerationStructureDeviceAddress,
                                  VkDeviceAddress& bottomLevelGeometryInstanceDeviceAddress)
{
	// Create Top Level Acceleration Structure
	VkAccelerationStructureInstanceKHR bottomLevelAccelerationStructureInstance = {
	    .transform = {.matrix = {{1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}}},
	    .instanceCustomIndex = 0,
	    .mask = 0xFF,
	    .instanceShaderBindingTableRecordOffset = 0,
	    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
	    .accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress};

	VkBuffer bottomLevelGeometryInstanceBufferHandle = VK_NULL_HANDLE;

	VkDeviceMemory bottomLevelGeometryInstanceDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(VkAccelerationStructureInstanceKHR),
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             bottomLevelGeometryInstanceBufferHandle,
	             bottomLevelGeometryInstanceDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* hostbottomLevelGeometryInstanceMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              bottomLevelGeometryInstanceDeviceMemoryHandle,
	                              0,
	                              sizeof(VkAccelerationStructureInstanceKHR),
	                              0,
	                              &hostbottomLevelGeometryInstanceMemoryBuffer);

	memcpy(hostbottomLevelGeometryInstanceMemoryBuffer,
	       &bottomLevelAccelerationStructureInstance,
	       sizeof(VkAccelerationStructureInstanceKHR));

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, bottomLevelGeometryInstanceDeviceMemoryHandle);

	VkBufferDeviceAddressInfo bottomLevelGeometryInstanceDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = bottomLevelGeometryInstanceBufferHandle};

	bottomLevelGeometryInstanceDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &bottomLevelGeometryInstanceDeviceAddressInfo);
}

} // namespace rt
} // namespace ltracer
