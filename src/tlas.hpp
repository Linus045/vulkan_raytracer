#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "src/deletion_queue.hpp"
#include "src/device_procedures.hpp"
#include "src/types.hpp"
#include "src/vk_utils.hpp"

namespace ltracer
{
namespace rt
{

inline void createAndBuildTopLevelAccelerationStructure(
    const std::vector<VkAccelerationStructureInstanceKHR>& instances,
    DeletionQueue& deletionQueue,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    RaytracingInfo& raytracingInfo,
    VkQueue queueHandle)
{
	// std::vector<VkAccelerationStructureInstanceKHR> blasGeometryInstances;
	// for (const auto& instance : blasInstances)
	// {
	// 	blasGeometryInstances.push_back(instance.info.bottomLevelGeometryInstance);
	// }
	// std::cout << "blasGeometryInstances.size(): " << blasGeometryInstances.size() << "\n";

	//=================================================================================================
	// copy acceleration structure geometry instances from array to a buffer
	VkBuffer blasGeometryInstancesBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory blasGeometryInstancesDeviceMemoryHandle = VK_NULL_HANDLE;
	VkDeviceSize bufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             bufferSize,
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             blasGeometryInstancesBufferHandle,
	             blasGeometryInstancesDeviceMemoryHandle);

	void* hostAccelerationStructureInstancesMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              blasGeometryInstancesDeviceMemoryHandle,
	                              0,
	                              bufferSize,
	                              0,
	                              &hostAccelerationStructureInstancesMemoryBuffer);

	memcpy(hostAccelerationStructureInstancesMemoryBuffer, instances.data(), bufferSize);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("createAndBuildTopLevelAccelerationStructure - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, blasGeometryInstancesDeviceMemoryHandle);

	//=================================================================================================
	// create top level acceleration structure geometry
	VkBufferDeviceAddressInfoKHR blasGeometryInstancesAddressesInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
	    .pNext = NULL,
	    .buffer = blasGeometryInstancesBufferHandle,
	};

	VkDeviceAddress blasGeometryInstancesDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(logicalDevice,
	                                                        &blasGeometryInstancesAddressesInfo);

	// VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData = {};
	// topLevelAccelerationStructureGeometryData.aabbs = {
	//     .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
	//     .pnext = NULL,
	//     .data = {},
	//     .stride = sizeof(AABB),
	// };

	VkAccelerationStructureGeometryKHR topLevelAccelerationStructureGeometry = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.pNext = NULL,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.pNext = NULL,
				.arrayOfPointers = VK_FALSE,
				.data = {
					.deviceAddress = blasGeometryInstancesDeviceAddress,
				},
			},
		},
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
	    .geometryCount = 1,
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

	std::vector<uint32_t> topLevelMaxPrimitiveCountList
	    = {static_cast<unsigned int>(instances.size())};

	ltracer::procedures::pvkGetAccelerationStructureBuildSizesKHR(
	    logicalDevice,
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &topLevelAccelerationStructureBuildGeometryInfo,
	    // Vulkan Docs: pMaxPrimitiveCounts is a pointer to an array of pBuildInfo->geometryCount
	    // uint32_t values defining the number of primitives built into each geometry.
	    topLevelMaxPrimitiveCountList.data(),
	    &topLevelAccelerationStructureBuildSizesInfo);

	VkBuffer topLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory topLevelAccelerationStructureDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             topLevelAccelerationStructureBufferHandle,
	             topLevelAccelerationStructureDeviceMemoryHandle);

	//=================================================================================================
	// create top level acceleration structure from geometry
	VkAccelerationStructureCreateInfoKHR topLevelAccelerationStructureCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .createFlags = 0,
	    .buffer = topLevelAccelerationStructureBufferHandle,
	    .offset = 0,
	    .size = topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .deviceAddress = 0,
	};

	result = ltracer::procedures::pvkCreateAccelerationStructureKHR(
	    logicalDevice,
	    &topLevelAccelerationStructureCreateInfo,
	    NULL,
	    &raytracingInfo.topLevelAccelerationStructureHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateAccelerationStructureKHR");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    {
		    ltracer::procedures::pvkDestroyAccelerationStructureKHR(
		        logicalDevice, raytracingInfo.topLevelAccelerationStructureHandle, NULL);
	    });

	//=================================================================================================
	// Build Top Level Acceleration Structure on device
	// VkAccelerationStructureDeviceAddressInfoKHR topLevelAccelerationStructureDeviceAddressInfo =
	// {
	//     .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	//     .pNext = NULL,
	//     .accelerationStructure = raytracingInfo.topLevelAccelerationStructureHandle,
	// };

	// VkDeviceAddress topLevelAccelerationStructureDeviceAddress
	//     = ltracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
	//         logicalDevice, &topLevelAccelerationStructureDeviceAddressInfo);

	VkBuffer topLevelAccelerationStructureScratchBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory topLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             topLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             topLevelAccelerationStructureScratchBufferHandle,
	             topLevelAccelerationStructureDeviceScratchMemoryHandle);

	VkBufferDeviceAddressInfo topLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = topLevelAccelerationStructureScratchBufferHandle,
	};

	VkDeviceAddress topLevelAccelerationStructureScratchBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &topLevelAccelerationStructureScratchBufferDeviceAddressInfo);

	topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
	    = raytracingInfo.topLevelAccelerationStructureHandle;

	topLevelAccelerationStructureBuildGeometryInfo.scratchData = {
	    .deviceAddress = topLevelAccelerationStructureScratchBufferDeviceAddress,
	};

	VkAccelerationStructureBuildRangeInfoKHR topLevelAccelerationStructureBuildRangeInfo = {
	    .primitiveCount = static_cast<uint32_t>(instances.size()),
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	const VkAccelerationStructureBuildRangeInfoKHR* topLevelAccelerationStructureBuildRangeInfos
	    = &topLevelAccelerationStructureBuildRangeInfo;

	VkCommandBufferBeginInfo topLevelCommandBufferBeginInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = NULL,
	};

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

	VkSubmitInfo topLevelAccelerationStructureBuildSubmitInfo = {
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

	VkFenceCreateInfo topLevelAccelerationStructureBuildFenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};

	VkFence topLevelAccelerationStructureBuildFenceHandle = VK_NULL_HANDLE;
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

} // namespace rt
} // namespace ltracer
