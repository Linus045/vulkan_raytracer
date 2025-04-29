#pragma once

#include <cstring>
#include <stdexcept>
#include <fstream>

#include <vulkan/vulkan_core.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#include "deletion_queue.hpp"
#include "types.hpp"

namespace tracer
{

// see https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
inline std::string errorString(VkResult errorCode)
{
#define STR(r)                                                                                     \
	case VK_##r:                                                                                   \
		return #r

	switch (errorCode)
	{
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
		STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
		STR(SUCCESS);
	default:
		return ("Error code: " + std::to_string(errorCode)
		        + " - Vulkan error code cannot be converted to string please add it to the "
		          "errorString() function")
		    .c_str();
	}
#undef STR
}

// see https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
#define VK_CHECK_RESULT(f)                                                                         \
	{                                                                                              \
		VkResult res = (f);                                                                        \
		if (res != VK_SUCCESS)                                                                     \
		{                                                                                          \
			std::cout << "Fatal : VkResult is \"" << ::tracer::errorString(res) << "\" in "        \
			          << __FILE__ << " at line " << __LINE__ << "\n";                              \
			assert(res == VK_SUCCESS);                                                             \
		}                                                                                          \
	}

constexpr VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
    .pNext = NULL,
    .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
    .deviceMask = 0,
};

inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		bool typeMatches = typeFilter & (1 << i);
		bool propertiesMatch
		    = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
		if (typeMatches && propertiesMatch)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

// TODO: probably add some form of caching
inline tracer::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
                                                    VkSurfaceKHR vulkanSurface)
{
	tracer::QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
	    physicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	// std::cout << "------------------------------------\n";
	for (const auto& queueFamily : queueFamilies)
	{

		// std::cout << "Queue number: " +
		// std::to_string(queueFamily.queueCount)
		//           << std::endl;
		// std::cout << "Queue flags: " +
		// string_VkQueueFlags(queueFamily.queueFlags)
		//           << std::endl;

		bool hasGraphicsBit = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		if (hasGraphicsBit)
		{
			indices.graphicsFamily = i;
		}

		// bool hasTransferBit = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
		// if (!hasGraphicsBit && hasTransferBit)
		// {
		// 	indices.transferFamily = i;
		// }

		VkBool32 presentCapabilitiesSupported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(
		    physicalDevice, i, vulkanSurface, &presentCapabilitiesSupported);

		if (presentCapabilitiesSupported)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete()) break;

		i++;
	}
	// std::cout << "------------------------------------\n";

	// std::cout << "indices.graphicsFamily: " << indices.graphicsFamily.value() << std::endl;
	// std::cout << "indices.transferFamily: " << indices.transferFamily.value() << std::endl;
	// std::cout << "indices.presentFamily: " << indices.presentFamily.value() << std::endl;

	if (!indices.isComplete())
	{
		throw std::runtime_error(
		    "QueueFamilyIndices::findQueueFamilies - failed to find suitable queue families!");
	}

	return indices;
}

inline void
createBuffer([[maybe_unused]] VkPhysicalDevice physicalDevice,
             [[maybe_unused]] VkDevice logicalDevice,
             VmaAllocator allocator,
             DeletionQueue& deletionQueue,
             VkDeviceSize size,
             VkBufferUsageFlags usage,
             VkMemoryPropertyFlags properties,
             [[maybe_unused]] const VkMemoryAllocateFlagsInfo& additionalMemoryAllocateFlagsInfo,
             VkBuffer& buffer,
             VmaAllocation& allocation,
             VkDeviceSize alignment = 0)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = NULL;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.requiredFlags = properties;

	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	allocInfo.memoryTypeBits = 0; // no restrictions

	if (alignment == 0)
	{
		VK_CHECK_RESULT(
		    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));
	}
	else
	{
		VK_CHECK_RESULT(vmaCreateBufferWithAlignment(
		    allocator, &bufferInfo, &allocInfo, alignment, &buffer, &allocation, nullptr));
	}

	deletionQueue.push_function([=]() { vmaDestroyBuffer(allocator, buffer, allocation); });

	VmaAllocationInfo allocationInfo = {};
	vmaGetAllocationInfo(allocator, allocation, &allocationInfo);
}

inline void copyDataToBuffer(VmaAllocator vmaAllocator,
                             VmaAllocation allocation,
                             const void* data,
                             VkDeviceSize size)
{
	void* memoryBuffer;
	vmaMapMemory(vmaAllocator, allocation, &memoryBuffer);
	// VK_CHECK_RESULT(vkMapMemory(logicalDevice, bufferMemory, 0, size, 0, &memoryBuffer));

	memcpy(memoryBuffer, data, size);
	vmaUnmapMemory(vmaAllocator, allocation);
}

inline std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file: " + filename);
	}

	size_t filesize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(filesize);
	file.seekg(0);
	file.read(buffer.data(), static_cast<long>(filesize));

	file.close();

	return buffer;
}

inline void addImageMemoryBarrier(VkCommandBuffer commandBuffer,
                                  VkPipelineStageFlags2 srcStageMask,
                                  VkAccessFlags2 srcAccessMask,
                                  VkPipelineStageFlags2 dstStageMask,
                                  VkAccessFlags2 dstAccessMask,
                                  VkImageLayout oldLayout,
                                  VkImageLayout newLayout,
                                  VkImageSubresourceRange subresourceRange,
                                  VkImage image)
{
	VkImageMemoryBarrier2 imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	imageBarrier.srcStageMask = srcStageMask;
	imageBarrier.dstStageMask = dstStageMask;
	imageBarrier.srcAccessMask = srcAccessMask; // No prior access
	imageBarrier.dstAccessMask = dstAccessMask;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = image;

	imageBarrier.subresourceRange = subresourceRange;

	VkDependencyInfo dependencyInfo = {};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.dependencyFlags = 0;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

inline VmaAllocator createVMAAllocator(VkPhysicalDevice _physicalDevice,
                                       VkInstance _vulkanInstance,
                                       VkDevice _logicalDevice)
{

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.physicalDevice = _physicalDevice;
	allocatorInfo.instance = _vulkanInstance;
	allocatorInfo.device = _logicalDevice;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;

	VmaAllocator allocator;
	VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &allocator));
	return allocator;
}
} // namespace tracer
