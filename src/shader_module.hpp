#pragma once

#include "src/deletion_queue.hpp"
#include <filesystem>
#include <fstream>
#include <vulkan/vulkan_core.h>
namespace ltracer
{

namespace shader
{
inline void createShaderModule(const std::filesystem::path& filePath,
                               const VkDevice logicalDevice,
                               DeletionQueue& deletionQueue,
                               VkShaderModule& shaderModuleHandle)
{
	std::ifstream shaderFile(filePath, std::ios::binary | std::ios::ate);
	std::streamsize shaderFileSize = shaderFile.tellg();
	shaderFile.seekg(0, std::ios::beg);
	std::vector<uint32_t> shaderSource(static_cast<unsigned long>(shaderFileSize)
	                                   / sizeof(uint32_t));

	shaderFile.read(reinterpret_cast<char*>(shaderSource.data()), shaderFileSize);
	shaderFile.close();

	VkShaderModuleCreateInfo rayClosestHitShaderModuleCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .codeSize = (uint32_t)shaderSource.size() * sizeof(uint32_t),
	    .pCode = shaderSource.data(),
	};

	VkResult result = vkCreateShaderModule(
	    logicalDevice, &rayClosestHitShaderModuleCreateInfo, NULL, &shaderModuleHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateShaderModule");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyShaderModule(logicalDevice, shaderModuleHandle, NULL); });
}
}; // namespace shader
} // namespace ltracer
