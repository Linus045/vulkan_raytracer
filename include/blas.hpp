#pragma once

#include <cstring>
#include <vulkan/vulkan_core.h>

#include "common_types.h"
#include "deletion_queue.hpp"
#include "types.hpp"
#include "device_procedures.hpp"
#include "vk_utils.hpp"

namespace ltracer
{
namespace rt
{

struct BLASBuildData
{
	// NOTE: maybe make this a vector of geometries
	const VkAccelerationStructureGeometryKHR geometry;
	const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
	const int instanceCustomIndex;

	const VkTransformMatrixKHR transformMatrix = {
		.matrix = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
		},
	};
};

[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataAABB(VkDevice logicalDevice,
                                                    VkBuffer aabbBufferHandle,
                                                    const int customIndex,
                                                    VkTransformMatrixKHR modelMatrix
                                                    /*, const uint32_t primitiveCount*/)
{
	VkBufferDeviceAddressInfo aabbPositionsDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = aabbBufferHandle,
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
			.stride = sizeof(VkAabbPositionsKHR),
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
	    .primitiveCount = 1, // primitiveCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	return BLASBuildData{
	    .geometry = bottomLevelAccelerationStructureGeometry,
	    .buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
	    .instanceCustomIndex = customIndex,
	    .transformMatrix = modelMatrix,
	};
}

[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataTriangle(uint32_t primitiveCount,
                                                        uint32_t verticesCount,
                                                        VkDeviceAddress vertexBufferDeviceAddress,
                                                        VkDeviceAddress indexBufferDeviceAddress,
                                                        const int customIndex,
                                                        VkTransformMatrixKHR transformMatrix)
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

	return BLASBuildData{
	    .geometry = bottomLevelAccelerationStructureGeometry,
	    .buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
	    .instanceCustomIndex = customIndex,
	    .transformMatrix = transformMatrix,
	};
}

inline VkAccelerationStructureKHR
buildBottomLevelAccelerationStructure(VkPhysicalDevice physicalDevice,
                                      VkDevice logicalDevice,
                                      DeletionQueue& deletionQueue,
                                      const RaytracingInfo& raytracingInfo,
                                      const BLASBuildData blasBuildData)
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
	        .pGeometries = &blasBuildData.geometry,
	        .ppGeometries = NULL,
	        .scratchData = {.deviceAddress = 0},
	    };

	std::vector<uint32_t> bottomLevelMaxPrimitiveCountList
	    = {blasBuildData.buildRangeInfo.primitiveCount};

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
	             bottomLevelAccelerationStructureMemoryHandle);

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
	             bottomLevelAccelerationStructureDeviceScratchMemoryHandle);

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
	    = &blasBuildData.buildRangeInfo;

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

	result = vkQueueSubmit(raytracingInfo.graphicsQueueHandle,
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

template <typename T>
inline std::pair<VkBuffer, VkDeviceMemory> createObjectBuffer(VkPhysicalDevice physicalDevice,
                                                              VkDevice logicalDevice,
                                                              DeletionQueue& deletionQueue,
                                                              const T& object)
{
	VkBuffer bufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(T),
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             bufferHandle,
	             deviceMemoryHandle);

	copyDataToBuffer(logicalDevice, deviceMemoryHandle, &object, sizeof(T));

	return std::make_pair(bufferHandle, deviceMemoryHandle);
}

/**
 * @brief Creates a buffer for the VkAabbPositionsKHR objects from a vector of AABB objects
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue The buffer will be added to the deletion queue
 * @param aabbs Vector of AABB objects to copy to the buffer
 * @return VkBuffer The buffer handle
 */
// inline VkBuffer createAABBBuffer(VkPhysicalDevice physicalDevice,
//                                  VkDevice logicalDevice,
//                                  DeletionQueue& deletionQueue,
//                                  const std::vector<ltracer::AABB>& aabbs)
// {
// 	std::vector<VkAabbPositionsKHR> aabbPositions = std::vector<VkAabbPositionsKHR>(0);
// 	aabbPositions.reserve(aabbs.size());
// 	for (const auto& aabb : aabbs)
// 	{
// 		aabbPositions.push_back(aabb.getAabbPositions());
// 	}

// 	// we need an alignment of 8 bytes, VkAabbPositionsKHR is 24 bytes
// 	uint32_t blockSize = sizeof(VkAabbPositionsKHR);

// 	VkBuffer bufferHandle = VK_NULL_HANDLE;
// 	VkDeviceMemory aabbDeviceMemoryHandle = VK_NULL_HANDLE;
// 	createBuffer(physicalDevice,
// 	             logicalDevice,
// 	             deletionQueue,
// 	             blockSize,
// 	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
// 	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
// 	             memoryAllocateFlagsInfo,
// 	             bufferHandle,
// 	             aabbDeviceMemoryHandle);

// 	copyDataToBuffer(logicalDevice, aabbDeviceMemoryHandle, &aabbPositions, blockSize);

// 	return bufferHandle;
// }

/**
 * @brief creates the bottom level acceleration structure for the model
 *
 * @tparam T the type of the objects, e.g. Sphere, Tetrahedron, etc.
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue the acceleration structure is added to the deletion queue
 * @param objects the list of objects that will be stored in the acceleration structure
 * @param objectType the type of the objects, this is passed to the shader to determine what
 * intersection calculation to use
 * @param aabbs the list of Axis Aligned Bounding Boxes for the objects, see AABB::from* method,
 * it extracts the aabb from the object
 * @param blasInstancesData the vector that holds all BLAS (Bottom Level Acceleration Structure)
 * instances, to which the acceleration structure is added
 * @param objectsBufferHandle the buffer handle of the objects
 * @param aabbObjectsBufferHandle the buffer handle of the AABBs for the objects
 */
template <typename T>
[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataForObject(VkDevice logicalDevice,
                                                         const int customIndex,
                                                         VkTransformMatrixKHR modelMatrix,
                                                         VkBuffer& aabbObjectBufferHandle)
{
	// TODO: Make it possible to have multiple BLAS with their individual AABBs
	return createBottomLevelAccelerationStructureBuildDataAABB(
	    logicalDevice, aabbObjectBufferHandle, customIndex, modelMatrix);
}

} // namespace rt
} // namespace ltracer
