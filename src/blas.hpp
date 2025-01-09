#pragma once

#include <cstring>
#include <vulkan/vulkan_core.h>

#include "src/aabb.hpp"
#include "src/deletion_queue.hpp"
#include "src/types.hpp"
#include "src/device_procedures.hpp"

namespace ltracer
{
namespace rt
{

struct BLASInstance
{
	// NOTE: maybe make this a vector of geometries
	VkAccelerationStructureGeometryKHR geometry;
	VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;

	VkTransformMatrixKHR transformMatrix = {
		.matrix = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
		},
	};
};

inline BLASInstance createBottomLevelAccelerationStructureAABB(DeletionQueue& deletionQueue,
                                                               VkDevice logicalDevice,
                                                               VkPhysicalDevice physicalDevice,
                                                               RaytracingInfo& raytracingInfo,
                                                               const std::vector<AABB>& aabbs)
{
	std::vector<VkAabbPositionsKHR> aabbPositions;
	for (auto&& aabb : aabbs)
	{
		VkAabbPositionsKHR aabbPosition = {
		    .minX = aabb.min.x,
		    .minY = aabb.min.y,
		    .minZ = aabb.min.z,
		    .maxX = aabb.max.x,
		    .maxY = aabb.max.y,
		    .maxZ = aabb.max.z,
		};

		aabbPositions.push_back(aabbPosition);
	}

	// we need an alignment of 8 bytes
	uint32_t blockSize = ceil(sizeof(VkAabbPositionsKHR) / 8.0) * 8;
	// TODO: remove this assert?
	assert(blockSize == 24 && "This should be 24");

	VkBuffer aabbPositionsBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory aabbPositionsDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             blockSize * aabbPositions.size(),
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             aabbPositionsBufferHandle,
	             aabbPositionsDeviceMemoryHandle,
	             {raytracingInfo.queueFamilyIndices.presentFamily.value()});

	void* aabbPositionsMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              aabbPositionsDeviceMemoryHandle,
	                              0,
	                              blockSize * aabbPositions.size(),
	                              0,
	                              &aabbPositionsMemoryBuffer);

	memcpy(aabbPositionsMemoryBuffer, aabbPositions.data(), blockSize * aabbPositions.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error(
		    "createAndBuildBottomLevelAccelerationStructureAABB - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, aabbPositionsDeviceMemoryHandle);

	VkBufferDeviceAddressInfo aabbPositionsDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = aabbPositionsBufferHandle,
	};

	VkDeviceAddress aabbPositionsDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &aabbPositionsDeviceAddressInfo);

	// create Bottom Level Acceleration Structure
	VkAccelerationStructureGeometryDataKHR bottomLevelAccelerationStructureGeometryData = {
		.aabbs = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
			.pNext = NULL,
			.data = {
				.deviceAddress = aabbPositionsDeviceAddress
			},
			.stride = blockSize,
		},
	};

	VkAccelerationStructureGeometryKHR bottomLevelAccelerationStructureGeometry = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = NULL,
	    .geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
	    .geometry = bottomLevelAccelerationStructureGeometryData,
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR bottomLevelAccelerationStructureBuildRangeInfo = {
	    .primitiveCount = static_cast<uint32_t>(aabbs.size()),
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	return BLASInstance{
		.geometry = bottomLevelAccelerationStructureGeometry,
		.buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
		.transformMatrix = {
			.matrix = {
				{1, 0, 0, 0},
				{0, 1, 0, 0},
				{0, 0, 1, 0},
			},
		},
	};
}

inline BLASInstance
createBottomLevelAccelerationStructureTriangle(DeletionQueue& deletionQueue,
                                               VkDevice logicalDevice,
                                               VkPhysicalDevice physicalDevice,
                                               uint32_t primitiveCount,
                                               uint32_t verticesCount,
                                               VkDeviceAddress vertexBufferDeviceAddress,
                                               VkDeviceAddress indexBufferDeviceAddress,
                                               VkTransformMatrixKHR transformMatrix,
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

	VkAccelerationStructureBuildRangeInfoKHR bottomLevelAccelerationStructureBuildRangeInfo = {
	    .primitiveCount = primitiveCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	return BLASInstance{
	    .geometry = bottomLevelAccelerationStructureGeometry,
	    .buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
	    .transformMatrix = transformMatrix,
	};
}

inline VkAccelerationStructureKHR
buildBottomLevelAccelerationStructure(VkPhysicalDevice physicalDevice,
                                      VkDevice logicalDevice,
                                      DeletionQueue& deletionQueue,
                                      const RaytracingInfo& raytracingInfo,
                                      VkQueue graphicsQueueHandle,
                                      const BLASInstance& blas)
{
	VkAccelerationStructureKHR bottomLevelAccelerationStructureHandle = VK_NULL_HANDLE;

	// Create buffer for the bottom level acceleration structure
	VkAccelerationStructureBuildGeometryInfoKHR bottomLevelAccelerationStructureBuildGeometryInfo
	    = {
	        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	        .flags = 0,
	        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	        .srcAccelerationStructure = VK_NULL_HANDLE,
	        .dstAccelerationStructure = VK_NULL_HANDLE,
	        .geometryCount = 1,
	        .pGeometries = &blas.geometry,
	        .ppGeometries = NULL,
	        .scratchData = {.deviceAddress = 0},
	    };

	std::vector<uint32_t> bottomLevelMaxPrimitiveCountList = {blas.buildRangeInfo.primitiveCount};

	VkAccelerationStructureBuildSizesInfoKHR bottomLevelAccelerationStructureBuildSizesInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	    .pNext = NULL,
	    .accelerationStructureSize = 0,
	    .updateScratchSize = 0,
	    .buildScratchSize = 0,
	};

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

	VkAccelerationStructureCreateInfoKHR bottomLevelAccelerationStructureCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .createFlags = 0,
	    .buffer = bottomLevelAccelerationStructureBufferHandle,
	    .offset = 0,
	    .size = bottomLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	    .deviceAddress = 0,
	};

	VkResult result = ltracer::procedures::pvkCreateAccelerationStructureKHR(
	    logicalDevice,
	    &bottomLevelAccelerationStructureCreateInfo,
	    NULL,
	    &bottomLevelAccelerationStructureHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("buildBLASInstance - vkCreateAccelerationStructureKHR");
	}

	deletionQueue.push_function(
	    [=]()
	    {
		    ltracer::procedures::pvkDestroyAccelerationStructureKHR(
		        logicalDevice, bottomLevelAccelerationStructureHandle, NULL);
	    });

	// Build Bottom Level Acceleration Structure
	VkAccelerationStructureDeviceAddressInfoKHR bottomLevelAccelerationStructureDeviceAddressInfo
	    = {
	        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	        .pNext = NULL,
	        .accelerationStructure = bottomLevelAccelerationStructureHandle,
	    };

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

	VkBufferDeviceAddressInfo bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = bottomLevelAccelerationStructureScratchBufferHandle,
	};

	VkDeviceAddress bottomLevelAccelerationStructureScratchBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo);

	bottomLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
	    = bottomLevelAccelerationStructureHandle;

	bottomLevelAccelerationStructureBuildGeometryInfo.scratchData = {
	    .deviceAddress = bottomLevelAccelerationStructureScratchBufferDeviceAddress,
	};

	const VkAccelerationStructureBuildRangeInfoKHR* bottomLevelAccelerationStructureBuildRangeInfos
	    = &blas.buildRangeInfo;

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

	VkSubmitInfo bottomLevelAccelerationStructureBuildSubmitInfo = {
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = NULL,
	    .waitSemaphoreCount = 0,
	    .pWaitSemaphores = NULL,
	    .pWaitDstStageMask = NULL,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &raytracingInfo.commandBufferBuildTopAndBottomLevel,
	    .signalSemaphoreCount = 0,
	    .pSignalSemaphores = NULL,
	};

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

	result = vkQueueSubmit(graphicsQueueHandle,
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

	return bottomLevelAccelerationStructureHandle;
}

} // namespace rt
} // namespace ltracer
