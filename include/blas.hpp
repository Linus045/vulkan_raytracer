#pragma once

#include <cstring>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "deletion_queue.hpp"
#include "device_procedures.hpp"
#include "raytracing_worldobject.hpp"
#include "vk_utils.hpp"

namespace tracer
{
namespace rt
{

// TODO: create blas.cpp and move functions  there
// also move these structs into a different header file
struct BLASBuildData
{
	// NOTE: maybe make this a vector of geometries
	const VkAccelerationStructureGeometryKHR geometry;
	const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo;
};

// Holds a collection of objects to put into one BLAS
struct SceneObject
{
	// we only store pointers to the actual data in the raytracingScene
	// NOTE: idk why normal pointer caused issues here, instead of further debugging we just
	// use shared pointers...
	std::vector<std::shared_ptr<RaytracingWorldObject<RectangularBezierSurface2x2>>>
	    rectangularBezierSurfaces2x2{};
	// std::vector<rt::RaytracingWorldObject<Tetrahedron2>::Ptr> tetrahedrons2;
	std::vector<std::shared_ptr<RaytracingWorldObject<Sphere>>> spheres{};
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle2>>> bezierTriangles2{};
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle3>>> bezierTriangles3{};
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle4>>> bezierTriangles4{};

	const std::string name;
	VkTransformMatrixKHR transformMatrix;
	const uint32_t instanceCustomIndex;

	// store the buffer offsets for the objects inside the SceneObject
	// inside the shader we need to start accessing the objects with with this offet into the
	// spheres[]/tetrahedrons2[] etc. buffers
	const size_t spheresBufferOffset;
	const size_t bezierTriangles2BufferOffset;
	const size_t bezierTriangles3BufferOffset;
	const size_t bezierTriangles4BufferOffset;
	const size_t rectangularBezierSurfaces2x2BufferOffset;

	~SceneObject() = default;
	SceneObject(const SceneObject&) = delete;
	SceneObject& operator=(const SceneObject&) = delete;

	SceneObject(SceneObject&&) noexcept = default;
	SceneObject& operator=(SceneObject&&) noexcept = delete;

	size_t totalElementsCount() const
	{
		return spheres.size() + bezierTriangles2.size() + bezierTriangles3.size()
		       + bezierTriangles4.size() + rectangularBezierSurfaces2x2.size();
	}

	void setTransformMatrix(const VkTransformMatrixKHR& newTransformMatrix)
	{
		transformMatrix = newTransformMatrix;
	}

  private:
	SceneObject(const std::string& name,
	            const VkTransformMatrixKHR& transformMatrix,
	            const uint32_t instanceCustomIndex,
	            const size_t spheresBufferOffset,
	            const size_t bezierTriangles2BufferOffset,
	            const size_t bezierTriangles3BufferOffset,
	            const size_t bezierTriangles4BufferOffset,
	            const size_t rectangularBezierSurfaces2x2BufferOffset)
	    : name(name), transformMatrix(transformMatrix), instanceCustomIndex(instanceCustomIndex),
	      spheresBufferOffset(spheresBufferOffset),
	      bezierTriangles2BufferOffset(bezierTriangles2BufferOffset),
	      bezierTriangles3BufferOffset(bezierTriangles3BufferOffset),
	      bezierTriangles4BufferOffset(bezierTriangles4BufferOffset),
	      rectangularBezierSurfaces2x2BufferOffset(rectangularBezierSurfaces2x2BufferOffset)
	{
	}
	// allow only the RaytracingScene to create a SceneObject
	friend class RaytracingScene;
};

// holds the data for the BLAS build of a scene object
struct BLASSceneObjectBuildData
{
	std::vector<BLASBuildData> blasData;
	const VkTransformMatrixKHR transformMatrix;
	const uint32_t instanceCustomIndex;
};

[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataAABB(VkDevice logicalDevice,
                                                    VkBuffer aabbBufferHandle)
{
	VkBufferDeviceAddressInfo aabbPositionsDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = aabbBufferHandle,
	};

	VkDeviceAddress aabbPositionsDeviceAddress = tracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &aabbPositionsDeviceAddressInfo);

	// create Bottom Level Acceleration Structure
	VkAccelerationStructureGeometryDataKHR bottomLevelAccelerationStructureGeometryData = {
		.aabbs = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
			.pNext = NULL,
			.data = {
				.deviceAddress = aabbPositionsDeviceAddress
			},
			.stride = sizeof(VkAabbPositionsKHR),
		},
	};

	VkAccelerationStructureGeometryKHR bottomLevelAccelerationStructureGeometry = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = NULL,
	    .geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
	    .geometry = bottomLevelAccelerationStructureGeometryData,
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR bottomLevelAccelerationStructureBuildRangeInfo = {
	    .primitiveCount = 1, // primitiveCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	return BLASBuildData{
	    .geometry = bottomLevelAccelerationStructureGeometry,
	    .buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
	};
}

[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataTriangle(uint32_t primitiveCount,
                                                        uint32_t verticesCount,
                                                        VkDeviceAddress vertexBufferDeviceAddress,
                                                        VkDeviceAddress indexBufferDeviceAddress)
{

	// create Bottom Level Acceleration Structure
	VkAccelerationStructureGeometryDataKHR bottomLevelAccelerationStructureGeometryData = {
	    .triangles = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
			.pNext = NULL,
			.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
			.vertexData = {.deviceAddress = vertexBufferDeviceAddress},
			.vertexStride = sizeof(float) * 3,
			.maxVertex = verticesCount,
			.indexType = VK_INDEX_TYPE_UINT32,
			.indexData = {.deviceAddress = indexBufferDeviceAddress},
			.transformData = {.deviceAddress = 0},
		},
	};

	VkAccelerationStructureGeometryKHR bottomLevelAccelerationStructureGeometry = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = NULL,
	    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
	    .geometry = bottomLevelAccelerationStructureGeometryData,
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
	};

	VkAccelerationStructureBuildRangeInfoKHR bottomLevelAccelerationStructureBuildRangeInfo = {
	    .primitiveCount = primitiveCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0,
	};

	return BLASBuildData{
	    .geometry = bottomLevelAccelerationStructureGeometry,
	    .buildRangeInfo = bottomLevelAccelerationStructureBuildRangeInfo,
	};
}

inline VkAccelerationStructureKHR
buildBottomLevelAccelerationStructure(VkPhysicalDevice physicalDevice,
                                      VkDevice logicalDevice,
                                      VmaAllocator vmaAllocator,
                                      DeletionQueue& deletionQueue,
                                      const VkCommandBuffer bottomLevelCommandBuffer,
                                      const VkQueue graphicsQueue,
                                      const BLASSceneObjectBuildData blasBuildDatas,
                                      const VkFence accelerationStructureBuildFence)
{
	VkAccelerationStructureKHR bottomLevelAccelerationStructureHandle = VK_NULL_HANDLE;

	std::vector<uint32_t> bottomLevelMaxPrimitiveCountList;
	std::vector<VkAccelerationStructureGeometryKHR> geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>
	    bottomLevelAccelerationStructureBuildRangeInfos;

	bottomLevelMaxPrimitiveCountList.resize(blasBuildDatas.blasData.size());
	geometries.resize(blasBuildDatas.blasData.size());
	bottomLevelAccelerationStructureBuildRangeInfos.resize(blasBuildDatas.blasData.size());

	for (size_t i = 0; i < blasBuildDatas.blasData.size(); i++)
	{
		auto& buildData = blasBuildDatas.blasData[i];
		geometries[i] = buildData.geometry;
		bottomLevelMaxPrimitiveCountList[i] = buildData.buildRangeInfo.primitiveCount;
		bottomLevelAccelerationStructureBuildRangeInfos[i] = buildData.buildRangeInfo;
	}

	// Create buffer for the bottom level acceleration structure
	VkAccelerationStructureBuildGeometryInfoKHR bottomLevelAccelerationStructureBuildGeometryInfo
	    = {
	        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	        .pNext = NULL,
	        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	        .srcAccelerationStructure = VK_NULL_HANDLE,
	        .dstAccelerationStructure = VK_NULL_HANDLE,
	        .geometryCount = static_cast<uint32_t>(geometries.size()),
	        .pGeometries = geometries.data(),
	        .ppGeometries = NULL,
	        .scratchData = {.deviceAddress = 0},
	    };

	VkAccelerationStructureBuildSizesInfoKHR bottomLevelAccelerationStructureBuildSizesInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	    .pNext = NULL,
	    .accelerationStructureSize = 0,
	    .updateScratchSize = 0,
	    .buildScratchSize = 0,
	};

	tracer::procedures::pvkGetAccelerationStructureBuildSizesKHR(
	    logicalDevice,
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &bottomLevelAccelerationStructureBuildGeometryInfo,
	    bottomLevelMaxPrimitiveCountList.data(),
	    &bottomLevelAccelerationStructureBuildSizesInfo);

	VkBuffer bottomLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
	VmaAllocation bottomLevelAccelerationStructureBufferAllocation = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             vmaAllocator,
	             deletionQueue,
	             bottomLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             bottomLevelAccelerationStructureBufferHandle,
	             bottomLevelAccelerationStructureBufferAllocation);

	VkAccelerationStructureCreateInfoKHR bottomLevelAccelerationStructureCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .createFlags = 0,
	    .buffer = bottomLevelAccelerationStructureBufferHandle,
	    .offset = 0,
	    .size = bottomLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	    .deviceAddress = 0,
	};

	VK_CHECK_RESULT(tracer::procedures::pvkCreateAccelerationStructureKHR(
	    logicalDevice,
	    &bottomLevelAccelerationStructureCreateInfo,
	    NULL,
	    &bottomLevelAccelerationStructureHandle));

	deletionQueue.push_function(
	    [=]()
	    {
		    tracer::procedures::pvkDestroyAccelerationStructureKHR(
		        logicalDevice, bottomLevelAccelerationStructureHandle, NULL);
	    });

	// Build Bottom Level Acceleration Structure
	VkBuffer bottomLevelAccelerationStructureScratchBufferHandle = VK_NULL_HANDLE;
	VmaAllocation bottomLevelAccelerationStructureScratchBufferAllocation = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             vmaAllocator,
	             deletionQueue,
	             bottomLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             memoryAllocateFlagsInfo,
	             bottomLevelAccelerationStructureScratchBufferHandle,
	             bottomLevelAccelerationStructureScratchBufferAllocation);

	VkBufferDeviceAddressInfo bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = bottomLevelAccelerationStructureScratchBufferHandle,
	};

	VkDeviceAddress bottomLevelAccelerationStructureScratchBufferDeviceAddress
	    = tracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo);

	bottomLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure
	    = bottomLevelAccelerationStructureHandle;

	bottomLevelAccelerationStructureBuildGeometryInfo.scratchData = {
	    .deviceAddress = bottomLevelAccelerationStructureScratchBufferDeviceAddress,
	};

	VkCommandBufferBeginInfo bottomLevelCommandBufferBeginInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	    .pInheritanceInfo = NULL,
	};

	VK_CHECK_RESULT(
	    vkBeginCommandBuffer(bottomLevelCommandBuffer, &bottomLevelCommandBufferBeginInfo));

	// TODO: this doesn't seem to be strictly necessary? Keeping it here just to be sure
	VkMemoryBarrier2 waitForAccelerationStructureBarrier = {};
	waitForAccelerationStructureBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
	waitForAccelerationStructureBarrier.srcAccessMask
	    = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	waitForAccelerationStructureBarrier.dstAccessMask
	    = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	waitForAccelerationStructureBarrier.srcStageMask
	    = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
	waitForAccelerationStructureBarrier.dstStageMask
	    = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;

	VkDependencyInfo dependencyInfoWaitforAccelerationStructure = {};
	dependencyInfoWaitforAccelerationStructure.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfoWaitforAccelerationStructure.dependencyFlags = 0;
	dependencyInfoWaitforAccelerationStructure.memoryBarrierCount = 1;
	dependencyInfoWaitforAccelerationStructure.pMemoryBarriers
	    = &waitForAccelerationStructureBarrier;

	vkCmdPipelineBarrier2(bottomLevelCommandBuffer, &dependencyInfoWaitforAccelerationStructure);

	const VkAccelerationStructureBuildRangeInfoKHR* const buildRangeInfo[1]
	    = {bottomLevelAccelerationStructureBuildRangeInfos.data()};

	tracer::procedures::pvkCmdBuildAccelerationStructuresKHR(
	    bottomLevelCommandBuffer,
	    1,
	    &bottomLevelAccelerationStructureBuildGeometryInfo,
	    buildRangeInfo);

	VK_CHECK_RESULT(vkEndCommandBuffer(bottomLevelCommandBuffer));

	VkSubmitInfo bottomLevelAccelerationStructureBuildSubmitInfo = {
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .pNext = NULL,
	    .waitSemaphoreCount = 0,
	    .pWaitSemaphores = NULL,
	    .pWaitDstStageMask = NULL,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &bottomLevelCommandBuffer,
	    .signalSemaphoreCount = 0,
	    .pSignalSemaphores = NULL,
	};

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue,
	                              1,
	                              &bottomLevelAccelerationStructureBuildSubmitInfo,
	                              accelerationStructureBuildFence));

	VK_CHECK_RESULT(
	    vkWaitForFences(logicalDevice, 1, &accelerationStructureBuildFence, true, UINT32_MAX));

	VK_CHECK_RESULT(vkResetFences(logicalDevice, 1, &accelerationStructureBuildFence));

	return bottomLevelAccelerationStructureHandle;
}

template <typename T>
inline std::pair<VkBuffer, VmaAllocation> createObjectBuffer(VkPhysicalDevice physicalDevice,
                                                             VkDevice logicalDevice,
                                                             VmaAllocator vmaAllocator,
                                                             DeletionQueue& deletionQueue,
                                                             const T& object)
{
	VkBuffer bufferHandle = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             vmaAllocator,
	             deletionQueue,
	             sizeof(T),
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             bufferHandle,
	             allocation);

	copyDataToBuffer(vmaAllocator, allocation, &object, sizeof(T));

	return std::make_pair(bufferHandle, allocation);
}

/**
 * @brief Creates a buffer for the VkAabbPositionsKHR objects from a vector of AABB objects
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue The buffer will be added to the deletion queue
 * @param aabbs Vector of AABB objects to copy to the buffer
 * @return VkBuffer The buffer handle
 */
// inline VkBuffer createAABBBuffer(VkPhysicalDevice physicalDevice,
//                                  VkDevice logicalDevice,
//                                  DeletionQueue& deletionQueue,
//                                  const std::vector<tracer::AABB>& aabbs)
// {
// 	std::vector<VkAabbPositionsKHR> aabbPositions = std::vector<VkAabbPositionsKHR>(0);
// 	aabbPositions.reserve(aabbs.size());
// 	for (const auto& aabb : aabbs)
// 	{
// 		aabbPositions.push_back(aabb.getAabbPositions());
// 	}

// 	// we need an alignment of 8 bytes, VkAabbPositionsKHR is 24 bytes
// 	uint32_t blockSize = sizeof(VkAabbPositionsKHR);

// 	VkBuffer bufferHandle = VK_NULL_HANDLE;
// 	VkDeviceMemory aabbDeviceMemoryHandle = VK_NULL_HANDLE;
// 	createBuffer(physicalDevice,
// 	             logicalDevice,
// 	             deletionQueue,
// 	             blockSize,
// 	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
// 	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
// 	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
// 	             memoryAllocateFlagsInfo,
// 	             bufferHandle,
// 	             aabbDeviceMemoryHandle);

// 	copyDataToBuffer(logicalDevice, aabbDeviceMemoryHandle, &aabbPositions, blockSize);

// 	return bufferHandle;
// }

/**
 * @brief creates the bottom level acceleration structure for the model
 *
 * @tparam T the type of the objects, e.g. Sphere, Tetrahedron, etc.
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue the acceleration structure is added to the deletion queue
 * @param objects the list of objects that will be stored in the acceleration structure
 * @param objectType the type of the objects, this is passed to the shader to determine what
 * intersection calculation to use
 * @param aabbs the list of Axis Aligned Bounding Boxes for the objects, see AABB::from* method,
 * it extracts the aabb from the object
 * @param blasInstancesData the vector that holds all BLAS (Bottom Level Acceleration Structure)
 * instances, to which the acceleration structure is added
 * @param objectsBufferHandle the buffer handle of the objects
 * @param aabbObjectsBufferHandle the buffer handle of the AABBs for the objects
 */
template <typename T>
[[nodiscard]] inline BLASBuildData
createBottomLevelAccelerationStructureBuildDataForObject(VkDevice logicalDevice,
                                                         VkBuffer& aabbObjectBufferHandle)
{
	// TODO: Make it possible to have multiple BLAS with their individual AABBs
	return createBottomLevelAccelerationStructureBuildDataAABB(logicalDevice,
	                                                           aabbObjectBufferHandle);
}

} // namespace rt
} // namespace tracer
