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

namespace tracer
{

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

struct SharedInfo
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct RaytracingObjectBuffers
{
	VkBuffer gpuObjectsBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory gpuObjectsDeviceMemoryHandle = VK_NULL_HANDLE;

	// TODO: convert into a list of structs that hold the buffer and the device memory (maybe look
	// at how its done in nvidia's examples)
	VkBuffer tetrahedronsBufferHandle = VkBuffer(0);
	VkDeviceMemory tetrahedronsDeviceMemoryHandles = VkDeviceMemory(0);
	std::vector<VkBuffer> tetrahedronsAABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VkDeviceMemory> tetrahedronsAABBDeviceMemoryHandles
	    = std::vector<VkDeviceMemory>(0);

	VkBuffer bezierTriangles2BufferHandle = VkBuffer(0);
	VkDeviceMemory bezierTriangles2DeviceMemoryHandles = VkDeviceMemory(0);
	std::vector<VkBuffer> bezierTriangles2AABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VkDeviceMemory> bezierTriangles2AABBDeviceMemoryHandles
	    = std::vector<VkDeviceMemory>(0);

	VkBuffer spheresBufferHandle = VkBuffer(0);
	VkDeviceMemory spheresDeviceMemoryHandles = VkDeviceMemory(0);
	std::vector<VkBuffer> spheresAABBBufferHandles = std::vector<VkBuffer>(0);
	std::vector<VkDeviceMemory> spheresAABBDeviceMemoryHandles = std::vector<VkDeviceMemory>(0);

	VkBuffer rectangularBezierSurfaces2x2BufferHandle = VkBuffer(0);
	VkDeviceMemory rectangularBezierSurfaces2x2DeviceMemoryHandles = VkDeviceMemory(0);
	std::vector<VkBuffer> rectangularBezierSurfacesAABB2x2BufferHandles = std::vector<VkBuffer>(0);
	std::vector<VkDeviceMemory> rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles
	    = std::vector<VkDeviceMemory>(0);

	VkBuffer slicingPlanesBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory slicingPlanesDeviceMemoryHandle = VK_NULL_HANDLE;

	inline void clearAllHandles()
	{
		gpuObjectsDeviceMemoryHandle = VK_NULL_HANDLE;
		gpuObjectsBufferHandle = VK_NULL_HANDLE;
		spheresAABBBufferHandles.clear();
		spheresAABBDeviceMemoryHandles.clear();
		tetrahedronsAABBBufferHandles.clear();
		tetrahedronsAABBDeviceMemoryHandles.clear();
		bezierTriangles2AABBBufferHandles.clear();
		bezierTriangles2AABBDeviceMemoryHandles.clear();
		rectangularBezierSurfacesAABB2x2BufferHandles.clear();
		rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles.clear();

		tetrahedronsBufferHandle = VK_NULL_HANDLE;
		tetrahedronsDeviceMemoryHandles = VK_NULL_HANDLE;

		spheresBufferHandle = VK_NULL_HANDLE;
		spheresDeviceMemoryHandles = VK_NULL_HANDLE;

		rectangularBezierSurfaces2x2BufferHandle = VK_NULL_HANDLE;
		rectangularBezierSurfaces2x2DeviceMemoryHandles = VK_NULL_HANDLE;

		slicingPlanesBufferHandle = VK_NULL_HANDLE;
		slicingPlanesDeviceMemoryHandle = VK_NULL_HANDLE;

		bezierTriangles2BufferHandle = VK_NULL_HANDLE;
		bezierTriangles2DeviceMemoryHandles = VK_NULL_HANDLE;
	}
};

// TODO: split this up a bit into more sensible structs
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

	VkDeviceMemory rayTraceImageDeviceMemoryHandle = VK_NULL_HANDLE;
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
	VkDeviceMemory blasGeometryInstancesDeviceMemoryHandle;

	VkDeviceMemory topLevelAccelerationStructureDeviceMemoryHandle = VK_NULL_HANDLE;
	VkDeviceMemory topLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;

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

	// cache sicne it wont change during the lifetime of the program (only if a new device is used)
	VkQueue graphicsQueueHandle = VK_NULL_HANDLE;

	VkSampler raytraceImageSampler;
};

} // namespace tracer
