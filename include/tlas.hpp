#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "deletion_queue.hpp"
#include "device_procedures.hpp"
#include "vk_utils.hpp"

namespace tracer
{
namespace rt
{

// struct TLASInstance
// {
// 	// const VkAccelerationStructureGeometryKHR geometry;
// 	// const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
// 	// const ObjectType objectType;

// 	VkAccelerationStructureInstanceKHR instance;
// };

inline void createAndBuildTopLevelAccelerationStructure(
    const std::vector<VkAccelerationStructureInstanceKHR>& instances,
    DeletionQueue& deletionQueue,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VmaAllocator vmaAllocator,
    const bool onlyUpdate,
    VmaAllocation& blasGeometryInstancesBufferAllocation,
    VkAccelerationStructureGeometryKHR& topLevelAccelerationStructureGeometry,
    VkAccelerationStructureBuildGeometryInfoKHR& topLevelAccelerationStructureBuildGeometryInfo,
    VkAccelerationStructureKHR& topLevelAccelerationStructureHandle,
    VkAccelerationStructureBuildSizesInfoKHR& topLevelAccelerationStructureBuildSizesInfo,
    VkBuffer& topLevelAccelerationStructureBufferHandle,
    VmaAllocation& topLevelAccelerationStructureDeviceMemoryHandle,
    VkBuffer& topLevelAccelerationStructureScratchBufferHandle,
    VmaAllocation& topLevelAccelerationStructureScratchBufferAllocation,
    VkAccelerationStructureBuildRangeInfoKHR& topLevelAccelerationStructureBuildRangeInfo,
    VkCommandBuffer commandBufferBuildTopAndBottomLevel,
    VkQueue graphicsQueueHandle)

{
	// std::vector<VkAccelerationStructureInstanceKHR> blasGeometryInstances;
	// for (const auto& instance : blasInstances)
	// {
	// 	blasGeometryInstances.push_back(instance.info.bottomLevelGeometryInstance);
	// }
	// std::cout << "blasGeometryInstances.size(): " << blasGeometryInstances.size() << "\n";

	//=================================================================================================
	// copy acceleration structure geometry instances from array to a buffer
	if (onlyUpdate)
	{
		// Copy new instances to the buffer
		copyDataToBuffer(vmaAllocator,
		                 blasGeometryInstancesBufferAllocation,
		                 instances.data(),
		                 sizeof(VkAccelerationStructureInstanceKHR) * instances.size());

		// set flags to update instead of rebuild the acceleration structure
		topLevelAccelerationStructureBuildGeometryInfo.mode
		    = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		topLevelAccelerationStructureBuildGeometryInfo.srcAccelerationStructure
		    = topLevelAccelerationStructureHandle;
		topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
		    = topLevelAccelerationStructureHandle;
	}
	else
	{
		VkBuffer blasGeometryInstancesBufferHandle = VK_NULL_HANDLE;
		VkDeviceSize bufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

		createBuffer(physicalDevice,
		             logicalDevice,
		             vmaAllocator,
		             deletionQueue,
		             bufferSize,
		             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
		                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		             memoryAllocateFlagsInfo,
		             blasGeometryInstancesBufferHandle,
		             blasGeometryInstancesBufferAllocation);

		copyDataToBuffer(
		    vmaAllocator, blasGeometryInstancesBufferAllocation, instances.data(), bufferSize);

		//=================================================================================================
		// create top level acceleration structure geometry
		VkBufferDeviceAddressInfoKHR blasGeometryInstancesAddressesInfo = {
		    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
		    .pNext = NULL,
		    .buffer = blasGeometryInstancesBufferHandle,
		};

		VkDeviceAddress blasGeometryInstancesDeviceAddress
		    = tracer::procedures::pvkGetBufferDeviceAddressKHR(logicalDevice,
		                                                       &blasGeometryInstancesAddressesInfo);

		// VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData = {};
		// topLevelAccelerationStructureGeometryData.aabbs = {
		//     .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
		//     .pnext = NULL,
		//     .data = {},
		//     .stride = sizeof(AABB),
		// };

		topLevelAccelerationStructureGeometry = {
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

		topLevelAccelerationStructureBuildGeometryInfo = {
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		    .pNext = NULL,
		    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		    .flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
		    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		    .srcAccelerationStructure = VK_NULL_HANDLE,
		    .dstAccelerationStructure = VK_NULL_HANDLE,
		    .geometryCount = 1,
		    .pGeometries = &topLevelAccelerationStructureGeometry,
		    .ppGeometries = NULL,
		    .scratchData = {.deviceAddress = 0},
		};

		topLevelAccelerationStructureBuildSizesInfo = {
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
		    .pNext = NULL,
		    .accelerationStructureSize = 0,
		    .updateScratchSize = 0,
		    .buildScratchSize = 0,
		};

		std::vector<uint32_t> topLevelMaxPrimitiveCountList
		    = {static_cast<unsigned int>(instances.size())};

		tracer::procedures::pvkGetAccelerationStructureBuildSizesKHR(
		    logicalDevice,
		    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		    &topLevelAccelerationStructureBuildGeometryInfo,
		    // Vulkan Docs: pMaxPrimitiveCounts is a pointer to an array of
		    // pBuildInfo->geometryCount uint32_t values defining the number of primitives built
		    // into each geometry.
		    topLevelMaxPrimitiveCountList.data(),
		    &topLevelAccelerationStructureBuildSizesInfo);

		createBuffer(physicalDevice,
		             logicalDevice,
		             vmaAllocator,
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

		VK_CHECK_RESULT(tracer::procedures::pvkCreateAccelerationStructureKHR(
		    logicalDevice,
		    &topLevelAccelerationStructureCreateInfo,
		    NULL,
		    &topLevelAccelerationStructureHandle));

		deletionQueue.push_function(
		    [=]()
		    {
			    tracer::procedures::pvkDestroyAccelerationStructureKHR(
			        logicalDevice, topLevelAccelerationStructureHandle, NULL);
		    });

		//=================================================================================================
		// Build Top Level Acceleration Structure on device
		// VkAccelerationStructureDeviceAddressInfoKHR
		// topLevelAccelerationStructureDeviceAddressInfo =
		// {
		//     .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		//     .pNext = NULL,
		//     .accelerationStructure = raytracingInfo.topLevelAccelerationStructureHandle,
		// };

		// VkDeviceAddress topLevelAccelerationStructureDeviceAddress
		//     = tracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
		//         logicalDevice, &topLevelAccelerationStructureDeviceAddressInfo);

		createBuffer(physicalDevice,
		             logicalDevice,
		             vmaAllocator,
		             deletionQueue,
		             topLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		             memoryAllocateFlagsInfo,
		             topLevelAccelerationStructureScratchBufferHandle,
		             topLevelAccelerationStructureScratchBufferAllocation);

		VkBufferDeviceAddressInfo topLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
		    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		    .pNext = NULL,
		    .buffer = topLevelAccelerationStructureScratchBufferHandle,
		};

		VkDeviceAddress topLevelAccelerationStructureScratchBufferDeviceAddress
		    = tracer::procedures::pvkGetBufferDeviceAddressKHR(
		        logicalDevice, &topLevelAccelerationStructureScratchBufferDeviceAddressInfo);

		topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
		    = topLevelAccelerationStructureHandle;

		// if (onlyUpdate)
		// {
		topLevelAccelerationStructureBuildGeometryInfo.srcAccelerationStructure
		    = topLevelAccelerationStructureHandle;
		// }

		topLevelAccelerationStructureBuildGeometryInfo.scratchData = {
		    .deviceAddress = topLevelAccelerationStructureScratchBufferDeviceAddress,
		};

		topLevelAccelerationStructureBuildRangeInfo = {
		    .primitiveCount = static_cast<uint32_t>(instances.size()),
		    .primitiveOffset = 0,
		    .firstVertex = 0,
		    .transformOffset = 0,
		};
	}

	const VkAccelerationStructureBuildRangeInfoKHR* topLevelAccelerationStructureBuildRangeInfos
	    = &topLevelAccelerationStructureBuildRangeInfo;

	VkCommandBufferBeginInfo topLevelCommandBufferBeginInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = NULL,
	};

	VK_CHECK_RESULT(
	    vkBeginCommandBuffer(commandBufferBuildTopAndBottomLevel, &topLevelCommandBufferBeginInfo));

	tracer::procedures::pvkCmdBuildAccelerationStructuresKHR(
	    commandBufferBuildTopAndBottomLevel,
	    1,
	    &topLevelAccelerationStructureBuildGeometryInfo,
	    &topLevelAccelerationStructureBuildRangeInfos);

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferBuildTopAndBottomLevel));

	VkSubmitInfo topLevelAccelerationStructureBuildSubmitInfo = {
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = NULL,
	    .waitSemaphoreCount = 0,
	    .pWaitSemaphores = NULL,
	    .pWaitDstStageMask = NULL,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &commandBufferBuildTopAndBottomLevel,
	    .signalSemaphoreCount = 0,
	    .pSignalSemaphores = NULL,
	};

	VkFenceCreateInfo topLevelAccelerationStructureBuildFenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	};

	VkFence topLevelAccelerationStructureBuildFenceHandle = VK_NULL_HANDLE;
	VK_CHECK_RESULT(vkCreateFence(logicalDevice,
	                              &topLevelAccelerationStructureBuildFenceCreateInfo,
	                              NULL,
	                              &topLevelAccelerationStructureBuildFenceHandle));

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueueHandle,
	                              1,
	                              &topLevelAccelerationStructureBuildSubmitInfo,
	                              topLevelAccelerationStructureBuildFenceHandle));
	VK_CHECK_RESULT(vkWaitForFences(
	    logicalDevice, 1, &topLevelAccelerationStructureBuildFenceHandle, true, UINT32_MAX));

	vkDestroyFence(logicalDevice, topLevelAccelerationStructureBuildFenceHandle, NULL);
}

} // namespace rt
} // namespace tracer
