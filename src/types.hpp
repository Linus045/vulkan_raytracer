#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "glm/ext/matrix_float4x4.hpp"
#include "src/deletion_queue.hpp"
#include <vulkan/vulkan_core.h>

namespace ltracer
{

static VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
    .pNext = NULL,
    .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
    .deviceMask = 0,
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily = std::nullopt;
	std::optional<uint32_t> presentFamily = std::nullopt;
	std::optional<uint32_t> transferFamily = std::nullopt;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value()
		       && transferFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 modelMatrix;
	alignas(16) std::vector<glm::vec3> modelNormals;
};

struct SharedInfo
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UniformStructure
{
	float cameraPosition[4];
	float cameraRight[4];
	float cameraUp[4];
	float cameraForward[4];

	uint32_t frameCount;
};

// TODO: split this up a bit into more sensible structs
struct RaytracingInfo
{
	VkPipeline rayTracingPipelineHandle = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSetHandleList
	    = std::vector<VkDescriptorSet>(2, VK_NULL_HANDLE);

	VkStridedDeviceAddressRegionKHR rchitShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR rgenShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR rmissShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR intersectionShaderBindingTable = {};

	VkDeviceMemory rayTraceImageDeviceMemoryHandle = VK_NULL_HANDLE;
	VkImage rayTraceImageHandle = VK_NULL_HANDLE;
	VkImageView rayTraceImageViewHandle = VK_NULL_HANDLE;

	ltracer::QueueFamilyIndices queueFamilyIndices = {};

	UniformStructure uniformStructure;

	VkCommandBuffer commandBufferBuildTopAndBottomLevel = VK_NULL_HANDLE;
	// VkCommandBuffer commandBufferBuildAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureKHR topLevelAccelerationStructureHandle = VK_NULL_HANDLE;

	VkBuffer uniformBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory uniformDeviceMemoryHandle = VK_NULL_HANDLE;

	VkShaderModule rayMissShadowShaderModuleHandle = VK_NULL_HANDLE;
	VkShaderModule rayMissShaderModuleHandle = VK_NULL_HANDLE;
	VkShaderModule rayGenerateShaderModuleHandle = VK_NULL_HANDLE;
	VkShaderModule rayClosestHitShaderModuleHandle = VK_NULL_HANDLE;
	VkShaderModule rayAABBIntersectionModuleHandle = VK_NULL_HANDLE;

	uint32_t shaderGroupCount = 0;

	uint32_t hitGroupOffset = 0;
	uint32_t rayGenGroupOffset = 0;
	uint32_t missGroupOffset = 0;

	uint32_t hitGroupSize = 0;
	uint32_t rayGenGroupSize = 0;
	uint32_t missGroupSize = 0;
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

inline void createBuffer(VkPhysicalDevice physicalDevice,
                         VkDevice logicalDevice,
                         DeletionQueue& deletionQueue,
                         VkDeviceSize size,
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         VkMemoryAllocateFlagsInfo& additionalMemoryAllocateFlagsInfo,
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

	int i = 0;
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

/// TODO: Move this to a more appropriate place
static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file: " + filename);
	}

	size_t filesize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(filesize);
	file.seekg(0);
	file.read(buffer.data(), filesize);

	file.close();

	return buffer;
}

} // namespace ltracer
