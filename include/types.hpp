#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"

#include "common_types.h"
#include "model.hpp"

namespace ltracer
{

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

struct RaytracingObjectBuffers
{
	// TODO: convert into a list for the aabbs per blas
	VkBuffer tetrahedronsBufferHandle = VK_NULL_HANDLE;
	VkBuffer tetrahedronsAABBBufferHandle = VK_NULL_HANDLE;

	VkBuffer spheresBufferHandle = VK_NULL_HANDLE;
	VkBuffer spheresAABBBufferHandle = VK_NULL_HANDLE;

	VkBuffer rectangularBezierSurfaces2x2BufferHandle = VK_NULL_HANDLE;
	VkBuffer rectangularBezierSurfacesAABB2x2BufferHandle = VK_NULL_HANDLE;

	VkBuffer slicingPlanesBufferHandle = VK_NULL_HANDLE;
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

	RaytracingDataConstants raytracingConstants;

	// the mesh objects that are shown in the scene (loaded obj files)
	std::vector<MeshObject> meshObjects;

	// the objects that are rendered using ray tracing (with an intersection shader)
	RaytracingObjectBuffers objectBuffers;
};

} // namespace ltracer
