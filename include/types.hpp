#pragma once

#include "logger.hpp"
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

#include "vk_mem_alloc.h"

namespace tracer
{

// holds the queue family indices for the graphics and present queue
// transfer queue is not used for now
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily = std::nullopt;
	std::optional<uint32_t> presentFamily = std::nullopt;
	// std::optional<uint32_t> transferFamily = std::nullopt;

	bool isComplete()
	{

		return graphicsFamily.has_value() && presentFamily.has_value();
		// && transferFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// unused
struct SharedInfo
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// struct to group together all AABB objects needed for the BLAS creation
struct RaytracingObjectAABBBuffers
{

	// TODO: convert into a list of structs that hold the buffer and the device memory (maybe look
	// at how its done in nvidia's examples)
	std::vector<VkBuffer> tetrahedronsAABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> tetrahedronsAABBBufferAllocations = std::vector<VmaAllocation>(0);

	std::vector<VkBuffer> bezierTriangles2AABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> bezierTriangles2AABBBufferAllocations
	    = std::vector<VmaAllocation>(0);

	std::vector<VkBuffer> bezierTriangles3AABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> bezierTriangles3AABBBufferAllocations
	    = std::vector<VmaAllocation>(0);

	std::vector<VkBuffer> bezierTriangles4AABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> bezierTriangles4AABBBufferAllocations
	    = std::vector<VmaAllocation>(0);

	std::vector<VkBuffer> spheresAABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> spheresAABBBufferAllocations = std::vector<VmaAllocation>(0);

	std::vector<VkBuffer> rectangularBezierSurfacesAABB2x2BufferHandles = std::vector<VkBuffer>(0);
	std::vector<VmaAllocation> rectangularBezierSurfacesAABB2x2AABBBufferAllocations
	    = std::vector<VmaAllocation>(0);

	inline void clearAllHandles()
	{
		spheresAABBBufferHandles.clear();
		spheresAABBBufferAllocations.clear();
		tetrahedronsAABBBufferHandles.clear();
		tetrahedronsAABBBufferAllocations.clear();
		bezierTriangles2AABBBufferHandles.clear();
		bezierTriangles2AABBBufferAllocations.clear();
		rectangularBezierSurfacesAABB2x2BufferHandles.clear();
		rectangularBezierSurfacesAABB2x2AABBBufferAllocations.clear();
	}
};

// TODO: split this up a bit into more sensible structs
// holds all kinds of various pointers used for raytracing
struct RaytracingInfo
{
	VkPipeline rayTracingPipelineHandle = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSetHandleList{};

	VkStridedDeviceAddressRegionKHR rchitShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR rgenShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR rmissShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};
	VkStridedDeviceAddressRegionKHR intersectionShaderBindingTable = {};

	VmaAllocation rayTraceImageDeviceMemoryHandle = VK_NULL_HANDLE;
	VkImage rayTraceImageHandle = VK_NULL_HANDLE;
	VkImageView rayTraceImageViewHandle = VK_NULL_HANDLE;

	VkFence accelerationStructureBuildFence = VK_NULL_HANDLE;

	tracer::QueueFamilyIndices queueFamilyIndices = {};

	UniformStructure uniformStructure;

	VkCommandBuffer commandBufferBuildTopAndBottomLevel = VK_NULL_HANDLE;
	// VkCommandBuffer commandBufferBuildAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureKHR topLevelAccelerationStructureHandle = VK_NULL_HANDLE;

	VkAccelerationStructureBuildGeometryInfoKHR topLevelAccelerationStructureBuildGeometryInfo;
	VkAccelerationStructureBuildSizesInfoKHR topLevelAccelerationStructureBuildSizesInfo;
	VkBuffer topLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
	VkBuffer topLevelAccelerationStructureScratchBufferHandle = VK_NULL_HANDLE;
	VkAccelerationStructureBuildRangeInfoKHR topLevelAccelerationStructureBuildRangeInfo;
	VkAccelerationStructureGeometryKHR topLevelAccelerationStructureGeometry;
	VmaAllocation blasGeometryInstancesDeviceMemoryHandle;

	VmaAllocation topLevelAccelerationStructureDeviceMemoryHandle = VK_NULL_HANDLE;
	VmaAllocation topLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;

	VkBuffer uniformBufferHandle = VK_NULL_HANDLE;
	VmaAllocation uniformBufferAllocation = VK_NULL_HANDLE;

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
	VkDeviceSize minAccelerationStructureScratchOffsetAlignment;

	// the mesh objects that are shown in the scene (loaded obj files)
	std::vector<MeshObject> meshObjects;

	// cache sicne it wont change during the lifetime of the program (only if a new device is used)
	VkQueue graphicsQueueHandle = VK_NULL_HANDLE;

	VkSampler raytraceImageSampler;
};

} // namespace tracer
