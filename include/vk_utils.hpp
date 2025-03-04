#pragma once

#include <cstring>
#include <stdexcept>
#include <fstream>

#include <vulkan/vulkan_core.h>

#include "deletion_queue.hpp"
#include "types.hpp"

namespace ltracer
{

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
inline ltracer::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
                                                     VkSurfaceKHR vulkanSurface)
{
	ltracer::QueueFamilyIndices indices;

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

		bool hasTransferBit = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
		if (!hasGraphicsBit && hasTransferBit)
		{
			indices.transferFamily = i;
		}

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

	return indices;
}

inline void createBuffer(VkPhysicalDevice physicalDevice,
                         VkDevice logicalDevice,
                         DeletionQueue& deletionQueue,
                         VkDeviceSize size,
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         const VkMemoryAllocateFlagsInfo& additionalMemoryAllocateFlagsInfo,
                         VkBuffer& buffer,
                         VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = NULL;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vertex buffer");
	}

	deletionQueue.push_function([=]() { vkDestroyBuffer(logicalDevice, buffer, NULL); });

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.pNext = &additionalMemoryAllocateFlagsInfo,
	allocInfo.memoryTypeIndex
	    = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);
	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate vertex buffer memory");
	}

	deletionQueue.push_function([=]() { vkFreeMemory(logicalDevice, bufferMemory, NULL); });
	vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
}

inline void copyDataToBuffer(VkDevice logicalDevice,
                             VkDeviceMemory bufferMemory,
                             const void* data,
                             VkDeviceSize size)
{
	void* memoryBuffer;
	VkResult result = vkMapMemory(logicalDevice, bufferMemory, 0, size, 0, &memoryBuffer);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("createBLASBuildDataForScene - vkMapMemory");
	}

	memcpy(memoryBuffer, data, size);

	vkUnmapMemory(logicalDevice, bufferMemory);
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

} // namespace ltracer
