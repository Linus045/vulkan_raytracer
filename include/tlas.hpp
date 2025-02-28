#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "deletion_queue.hpp"
#include "device_procedures.hpp"
#include "vk_utils.hpp"

namespace ltracer
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
    RaytracingInfo& raytracingInfo,
    const bool onlyUpdate)
{
	// std::vector<VkAccelerationStructureInstanceKHR> blasGeometryInstances;
	// for (const auto& instance : blasInstances)
	// {
	// 	blasGeometryInstances.push_back(instance.info.bottomLevelGeometryInstance);
	// }
	// std::cout << "blasGeometryInstances.size(): " << blasGeometryInstances.size() << "\n";

	VkResult result;

	//=================================================================================================
	// copy acceleration structure geometry instances from array to a buffer
	if (onlyUpdate)
	{
		// Copy new instances to the buffer
		VkDeviceSize bufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
		void* hostAccelerationStructureInstancesMemoryBuffer;
		result = vkMapMemory(logicalDevice,
		                     raytracingInfo.blasGeometryInstancesDeviceMemoryHandle,
		                     0,
		                     bufferSize,
		                     0,
		                     &hostAccelerationStructureInstancesMemoryBuffer);

		memcpy(hostAccelerationStructureInstancesMemoryBuffer, instances.data(), bufferSize);

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error(
			    "createAndBuildTopLevelAccelerationStructure - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, raytracingInfo.blasGeometryInstancesDeviceMemoryHandle);

		// set flags to update instead of rebuild the acceleration structure
		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.mode
		    = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.srcAccelerationStructure
		    = raytracingInfo.topLevelAccelerationStructureHandle;
		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
		    = raytracingInfo.topLevelAccelerationStructureHandle;
	}
	else
	{
		VkBuffer blasGeometryInstancesBufferHandle = VK_NULL_HANDLE;
		raytracingInfo.blasGeometryInstancesDeviceMemoryHandle = VK_NULL_HANDLE;
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
		             raytracingInfo.blasGeometryInstancesDeviceMemoryHandle);

		void* hostAccelerationStructureInstancesMemoryBuffer;
		result = vkMapMemory(logicalDevice,
		                     raytracingInfo.blasGeometryInstancesDeviceMemoryHandle,
		                     0,
		                     bufferSize,
		                     0,
		                     &hostAccelerationStructureInstancesMemoryBuffer);

		memcpy(hostAccelerationStructureInstancesMemoryBuffer, instances.data(), bufferSize);

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error(
			    "createAndBuildTopLevelAccelerationStructure - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, raytracingInfo.blasGeometryInstancesDeviceMemoryHandle);

		//=================================================================================================
		// create top level acceleration structure geometry
		VkBufferDeviceAddressInfoKHR blasGeometryInstancesAddressesInfo = {
		    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
		    .pNext = NULL,
		    .buffer = blasGeometryInstancesBufferHandle,
		};

		VkDeviceAddress blasGeometryInstancesDeviceAddress
		    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
		        logicalDevice, &blasGeometryInstancesAddressesInfo);

		// VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData = {};
		// topLevelAccelerationStructureGeometryData.aabbs = {
		//     .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
		//     .pnext = NULL,
		//     .data = {},
		//     .stride = sizeof(AABB),
		// };

		raytracingInfo.topLevelAccelerationStructureGeometry = {
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

		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo = {
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		    .pNext = NULL,
		    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		    .flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
		    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		    .srcAccelerationStructure = VK_NULL_HANDLE,
		    .dstAccelerationStructure = VK_NULL_HANDLE,
		    .geometryCount = 1,
		    .pGeometries = &raytracingInfo.topLevelAccelerationStructureGeometry,
		    .ppGeometries = NULL,
		    .scratchData = {.deviceAddress = 0},
		};

		raytracingInfo.topLevelAccelerationStructureBuildSizesInfo = {
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
		    &raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo,
		    // Vulkan Docs: pMaxPrimitiveCounts is a pointer to an array of
		    // pBuildInfo->geometryCount uint32_t values defining the number of primitives built
		    // into each geometry.
		    topLevelMaxPrimitiveCountList.data(),
		    &raytracingInfo.topLevelAccelerationStructureBuildSizesInfo);

		createBuffer(
		    physicalDevice,
		    logicalDevice,
		    deletionQueue,
		    raytracingInfo.topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
		    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
		        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		    memoryAllocateFlagsInfo,
		    raytracingInfo.topLevelAccelerationStructureBufferHandle,
		    raytracingInfo.topLevelAccelerationStructureDeviceMemoryHandle);

		//=================================================================================================
		// create top level acceleration structure from geometry
		VkAccelerationStructureCreateInfoKHR topLevelAccelerationStructureCreateInfo = {
		    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		    .pNext = NULL,
		    .createFlags = 0,
		    .buffer = raytracingInfo.topLevelAccelerationStructureBufferHandle,
		    .offset = 0,
		    .size
		    = raytracingInfo.topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
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
		    [=]()
		    {
			    ltracer::procedures::pvkDestroyAccelerationStructureKHR(
			        logicalDevice, raytracingInfo.topLevelAccelerationStructureHandle, NULL);
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
		//     = ltracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
		//         logicalDevice, &topLevelAccelerationStructureDeviceAddressInfo);

		createBuffer(physicalDevice,
		             logicalDevice,
		             deletionQueue,
		             raytracingInfo.topLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		             memoryAllocateFlagsInfo,
		             raytracingInfo.topLevelAccelerationStructureScratchBufferHandle,
		             raytracingInfo.topLevelAccelerationStructureDeviceScratchMemoryHandle);

		VkBufferDeviceAddressInfo topLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
		    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		    .pNext = NULL,
		    .buffer = raytracingInfo.topLevelAccelerationStructureScratchBufferHandle,
		};

		VkDeviceAddress topLevelAccelerationStructureScratchBufferDeviceAddress
		    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
		        logicalDevice, &topLevelAccelerationStructureScratchBufferDeviceAddressInfo);

		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
		    = raytracingInfo.topLevelAccelerationStructureHandle;

		// if (onlyUpdate)
		// {
		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.srcAccelerationStructure
		    = raytracingInfo.topLevelAccelerationStructureHandle;
		// }

		raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo.scratchData = {
		    .deviceAddress = topLevelAccelerationStructureScratchBufferDeviceAddress,
		};

		raytracingInfo.topLevelAccelerationStructureBuildRangeInfo = {
		    .primitiveCount = static_cast<uint32_t>(instances.size()),
		    .primitiveOffset = 0,
		    .firstVertex = 0,
		    .transformOffset = 0,
		};
	}

	const VkAccelerationStructureBuildRangeInfoKHR* topLevelAccelerationStructureBuildRangeInfos
	    = &raytracingInfo.topLevelAccelerationStructureBuildRangeInfo;

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
	    &raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo,
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

	result = vkQueueSubmit(raytracingInfo.graphicsQueueHandle,
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

	vkDestroyFence(logicalDevice, topLevelAccelerationStructureBuildFenceHandle, NULL);
}

} // namespace rt
} // namespace ltracer
