#pragma once

#include <vector>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan_core.h>

#include "blas.hpp"
#include "common_types.h"
#include "deletion_queue.hpp"
#include "model.hpp"
#include "raytracing_worldobject.hpp"
#include "tlas.hpp"
#include "vk_utils.hpp"

namespace ltracer
{
namespace rt
{

class RaytracingScene
{
  public:
	RaytracingScene(const VkPhysicalDevice& physicalDevice, const VkDevice logicalDevice)
	    : physicalDevice(physicalDevice), logicalDevice(logicalDevice) {};

	~RaytracingScene() = default;

	RaytracingScene(const RaytracingScene&) = delete;
	RaytracingScene& operator=(const RaytracingScene&) = delete;

	RaytracingScene(RaytracingScene&&) noexcept = delete;
	RaytracingScene& operator=(RaytracingScene&&) noexcept = delete;

	const RaytracingObjectBuffers& getObjectBuffers() const
	{
		return objectBuffers;
	}

	void cleanup(VkQueue graphicsQueneHandle)
	{
		vkQueueWaitIdle(graphicsQueneHandle);
		deletionQueueForAccelerationStructure.flush();
	}

	void addWorldObject(const MeshObject& meshObject)
	{
		meshObjects.push_back(meshObject);
	}

	void addSphere(const glm::vec3 position, const float radius, const ColorIdx colorIdx)
	{
		Sphere sphere{
		    .center = position,
		    .radius = radius,
		    .colorIdx = static_cast<int>(colorIdx),
		};
		spheres.push_back(RaytracingWorldObject(
		    ObjectType::t_Sphere, AABB::fromSphere(sphere), sphere, position));
	}

	void addWorldObject(const RaytracingWorldObject<Sphere>& object)
	{
		spheres.push_back(object);
	}

	void addWorldObject(const Tetrahedron2& tetrahedron2, const glm::vec3 position = glm::vec3(0))
	{
		tetrahedrons2.push_back(RaytracingWorldObject(ObjectType::t_Tetrahedron2,
		                                              AABB::fromTetrahedron2(tetrahedron2),
		                                              tetrahedron2,
		                                              position));
	}

	void addWorldObject(const RectangularBezierSurface2x2& rectangularBezierSurface2x2,
	                    const glm::vec3 position = glm::vec3(0))
	{
		rectangularBezierSurfaces2x2.push_back(RaytracingWorldObject(
		    ObjectType::t_RectangularBezierSurface2x2,
		    AABB::fromRectangularBezierSurface2x2(rectangularBezierSurface2x2),
		    rectangularBezierSurface2x2,
		    position));
	}

	void addWorldObject(const RaytracingWorldObject<Tetrahedron2>& object)
	{
		tetrahedrons2.push_back(object);
	}

	void addWorldObject(const RaytracingWorldObject<RectangularBezierSurface2x2>& object)
	{
		rectangularBezierSurfaces2x2.push_back(object);
	}

	std::vector<RaytracingWorldObject<Sphere>>& getWorldObjectSpheres()
	{
		return spheres;
	}

	std::vector<RaytracingWorldObject<Tetrahedron2>>& getWorldObjectTetrahedrons()
	{
		return tetrahedrons2;
	}

	std::vector<RaytracingWorldObject<RectangularBezierSurface2x2>>&
	getWorldObjectRectangularBezierSurfaces2x2()
	{
		return rectangularBezierSurfaces2x2;
	}

	inline void setTransformMatrixForInstance(const size_t instanceIndex,
	                                          const VkTransformMatrixKHR& matrix)
	{
		blasInstances[instanceIndex].transform = matrix;
	}

	void createBuffers()
	{
		if (spheres.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             deletionQueueForAccelerationStructure,
			             spheres.size() * sizeof(Sphere),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.spheresBufferHandle,
			             objectBuffers.spheresDeviceMemoryHandles);
		}

		if (tetrahedrons2.size() > 0)
		{
			createBuffer(physicalDevice,

			             logicalDevice,
			             deletionQueueForAccelerationStructure,
			             tetrahedrons2.size() * sizeof(Tetrahedron2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.tetrahedronsBufferHandle,
			             objectBuffers.tetrahedronsDeviceMemoryHandles);
		}

		if (rectangularBezierSurfaces2x2.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             deletionQueueForAccelerationStructure,
			             rectangularBezierSurfaces2x2.size() * sizeof(RectangularBezierSurface2x2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.rectangularBezierSurfaces2x2BufferHandle,
			             objectBuffers.rectangularBezierSurfaces2x2DeviceMemoryHandles);
		}

		size_t size = spheres.size() + tetrahedrons2.size() + rectangularBezierSurfaces2x2.size();
		gpuObjects.resize(size);
	}

	void recreateAccelerationStructures(RaytracingInfo& raytracingInfo, const bool fullRebuild)
	{
		// We dont want to recreate the BLAS's but only update the related transform matrix inside
		// the TLAS that links to the particular BLAS
		if (fullRebuild)
		{
			deletionQueueForAccelerationStructure.flush();

			objectBuffers.gpuObjectsDeviceMemoryHandle = VK_NULL_HANDLE;
			objectBuffers.gpuObjectsBufferHandle = VK_NULL_HANDLE;
			objectBuffers.spheresAABBBufferHandles.clear();
			objectBuffers.spheresAABBDeviceMemoryHandles.clear();
			objectBuffers.tetrahedronsAABBBufferHandles.clear();
			objectBuffers.tetrahedronsAABBDeviceMemoryHandles.clear();

			objectBuffers.tetrahedronsBufferHandle = VK_NULL_HANDLE;
			objectBuffers.tetrahedronsDeviceMemoryHandles = VK_NULL_HANDLE;

			objectBuffers.spheresBufferHandle = VK_NULL_HANDLE;
			objectBuffers.spheresDeviceMemoryHandles = VK_NULL_HANDLE;

			objectBuffers.rectangularBezierSurfaces2x2BufferHandle = VK_NULL_HANDLE;
			objectBuffers.rectangularBezierSurfaces2x2DeviceMemoryHandles = VK_NULL_HANDLE;

			createBuffers();

			auto buildData = createBLASBuildDataForScene();
			blasInstances = buildBLASInstancesFromBuildDataList(
			    buildData,
			    raytracingInfo.commandBufferBuildTopAndBottomLevel,
			    raytracingInfo.graphicsQueueHandle,
			    raytracingInfo.accelerationStructureBuildFence);

			// TODO: create a dedicated struct that holds all the information for the acceleration
			// structure that is actually needed
			createTLAS(false,
			           raytracingInfo.blasGeometryInstancesDeviceMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureGeometry,
			           raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo,
			           raytracingInfo.topLevelAccelerationStructureHandle,
			           raytracingInfo.topLevelAccelerationStructureBuildSizesInfo,
			           raytracingInfo.topLevelAccelerationStructureBufferHandle,
			           raytracingInfo.topLevelAccelerationStructureDeviceMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureScratchBufferHandle,
			           raytracingInfo.topLevelAccelerationStructureDeviceScratchMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureBuildRangeInfo,
			           raytracingInfo.commandBufferBuildTopAndBottomLevel,
			           raytracingInfo.graphicsQueueHandle,
			           raytracingInfo.uniformBufferHandle,
			           raytracingInfo.uniformDeviceMemoryHandle,
			           raytracingInfo.uniformStructure);
		}
		else
		{

			createTLAS(true,
			           raytracingInfo.blasGeometryInstancesDeviceMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureGeometry,
			           raytracingInfo.topLevelAccelerationStructureBuildGeometryInfo,
			           raytracingInfo.topLevelAccelerationStructureHandle,
			           raytracingInfo.topLevelAccelerationStructureBuildSizesInfo,
			           raytracingInfo.topLevelAccelerationStructureBufferHandle,
			           raytracingInfo.topLevelAccelerationStructureDeviceMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureScratchBufferHandle,
			           raytracingInfo.topLevelAccelerationStructureDeviceScratchMemoryHandle,
			           raytracingInfo.topLevelAccelerationStructureBuildRangeInfo,
			           raytracingInfo.commandBufferBuildTopAndBottomLevel,
			           raytracingInfo.graphicsQueueHandle,
			           raytracingInfo.uniformBufferHandle,
			           raytracingInfo.uniformDeviceMemoryHandle,
			           raytracingInfo.uniformStructure);
		}
	}

	/// Copies the spheres, tetrhedrons and rectangular bezier surfaces to the corresponding
	/// buffers on the GPU
	void copyObjectsToBuffers()
	{
		if (spheres.size() > 0)
		{
			std::vector<Sphere> spheresList;
			for (size_t i = 0; i < spheres.size(); i++)
			{
				auto& obj = spheres[i];
				spheresList.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(logicalDevice,
			                 objectBuffers.spheresDeviceMemoryHandles,
			                 spheresList.data(),
			                 sizeof(Sphere) * spheresList.size());
		}

		if (tetrahedrons2.size() > 0)
		{
			std::vector<Tetrahedron2> tetrahedrons2List;
			for (size_t i = 0; i < tetrahedrons2.size(); i++)
			{
				auto& obj = tetrahedrons2[i];
				tetrahedrons2List.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(logicalDevice,
			                 objectBuffers.tetrahedronsDeviceMemoryHandles,
			                 tetrahedrons2List.data(),
			                 sizeof(Tetrahedron2) * tetrahedrons2List.size());
		}

		if (rectangularBezierSurfaces2x2.size() > 0)
		{
			std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List;
			for (size_t i = 0; i < rectangularBezierSurfaces2x2.size(); i++)
			{
				auto& obj = rectangularBezierSurfaces2x2[i];
				rectangularSurfaces2x2List.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(logicalDevice,
			                 objectBuffers.rectangularBezierSurfaces2x2DeviceMemoryHandles,
			                 rectangularSurfaces2x2List.data(),
			                 sizeof(RectangularBezierSurface2x2)
			                     * rectangularSurfaces2x2List.size());
		}
	}

  private:
	[[nodiscard]] const std::vector<BLASBuildData> createBLASBuildDataForScene()
	{
		std::vector<ltracer::AABB> aabbsSpheres;
		std::vector<ltracer::AABB> aabbsTetrahedrons;
		std::vector<ltracer::AABB> aabbsRectangularSurfaces2x2;

		std::vector<Sphere> spheresList;
		std::vector<Tetrahedron2> tetrahedrons2List;
		std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List;

		if (spheres.size() > 0)
		{
			objectBuffers.spheresAABBBufferHandles.resize(spheres.size());
			objectBuffers.spheresAABBDeviceMemoryHandles.resize(spheres.size());
			for (size_t i = 0; i < spheres.size(); i++)
			{
				auto& obj = spheres[i];
				aabbsSpheres.push_back(obj.getGeometry().getAABB());
				spheresList.push_back(obj.getGeometry().getData());

				createBuffer(physicalDevice,
				             logicalDevice,
				             deletionQueueForAccelerationStructure,
				             sizeof(VkAabbPositionsKHR),
				             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
				                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             objectBuffers.spheresAABBBufferHandles[i],
				             objectBuffers.spheresAABBDeviceMemoryHandles[i]);

				// Note: for now, all instances only contain one AABB, so we only need to copy
				// one object into the buffer
				const auto aabbPositions = obj.getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(logicalDevice,
				                 objectBuffers.spheresAABBDeviceMemoryHandles[i],
				                 &aabbPositions,
				                 sizeof(AABB));
			}
		}

		if (tetrahedrons2.size() > 0)
		{
			objectBuffers.tetrahedronsAABBBufferHandles.resize(tetrahedrons2.size());
			objectBuffers.tetrahedronsAABBDeviceMemoryHandles.resize(tetrahedrons2.size());
			for (size_t i = 0; i < tetrahedrons2.size(); i++)
			{
				auto& obj = tetrahedrons2[i];
				aabbsTetrahedrons.push_back(obj.getGeometry().getAABB());
				tetrahedrons2List.push_back(obj.getGeometry().getData());

				createBuffer(physicalDevice,
				             logicalDevice,
				             deletionQueueForAccelerationStructure,
				             sizeof(VkAabbPositionsKHR),
				             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
				                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             objectBuffers.tetrahedronsAABBBufferHandles[i],
				             objectBuffers.tetrahedronsAABBDeviceMemoryHandles[i]);

				const auto aabbPositions = obj.getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(logicalDevice,
				                 objectBuffers.tetrahedronsAABBDeviceMemoryHandles[i],
				                 &aabbPositions,
				                 sizeof(AABB));
			}
		}

		if (rectangularBezierSurfaces2x2.size() > 0)
		{
			objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles.resize(
			    rectangularBezierSurfaces2x2.size());
			objectBuffers.rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles.resize(
			    rectangularBezierSurfaces2x2.size());
			for (size_t i = 0; i < rectangularBezierSurfaces2x2.size(); i++)
			{
				auto& obj = rectangularBezierSurfaces2x2[i];
				aabbsRectangularSurfaces2x2.push_back(obj.getGeometry().getAABB());
				rectangularSurfaces2x2List.push_back(obj.getGeometry().getData());

				createBuffer(
				    physicalDevice,
				    logicalDevice,
				    deletionQueueForAccelerationStructure,
				    sizeof(VkAabbPositionsKHR),
				    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
				        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				    memoryAllocateFlagsInfo,
				    objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles[i],
				    objectBuffers.rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles[i]);

				const auto aabbPositions = obj.getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(
				    logicalDevice,
				    objectBuffers.rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles[i],
				    &aabbPositions,
				    sizeof(AABB));
			}
		}

		copyObjectsToBuffers();

		// TODO: create TLAS for each object in the scene
		VkDeviceSize instancesCount
		    = spheresList.size() + tetrahedrons2List.size() + rectangularSurfaces2x2List.size();

		assert(
		    instancesCount == gpuObjects.size()
		    && "Amount of objects in the scene does not match the "
		       "previous amount of objects in the GPU buffer. Changing the object count is not yet "
		       "supported!");

		gpuObjects.clear();

		// TODO: the size of the elements does not change as of now, we need to  implement
		// recreation/resizing of the buffer otherwise
		assert(objectBuffers.gpuObjectsBufferHandle == VK_NULL_HANDLE
		       && "GPU buffer already created");

		createBuffer(physicalDevice,
		             logicalDevice,
		             deletionQueueForAccelerationStructure,
		             sizeof(GPUInstance) * instancesCount,
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		             memoryAllocateFlagsInfo,
		             objectBuffers.gpuObjectsBufferHandle,
		             objectBuffers.gpuObjectsDeviceMemoryHandle);

		std::vector<BLASBuildData> blasBuildDataList;
		if (spheresList.size() > 0)
		{
			// create vector to hold all BLAS instances
			for (size_t i = 0; i < spheresList.size(); i++)
			{
				size_t instanceCustomIndex = gpuObjects.size();
				auto buildData = createBottomLevelAccelerationStructureBuildDataAABB(
				    logicalDevice,
				    objectBuffers.spheresAABBBufferHandles[i],
				    static_cast<int>(instanceCustomIndex),
				    spheres[i].getTransform().getTransformMatrix());

				spheres[i].setInstanceIndex(blasBuildDataList.size());
				gpuObjects.push_back(GPUInstance(ObjectType::t_Sphere, i));
				blasBuildDataList.push_back(buildData);
			}
		}

		if (tetrahedrons2List.size() > 0)
		{
			for (size_t i = 0; i < tetrahedrons2List.size(); i++)
			{
				size_t instanceCustomIndex = gpuObjects.size();
				auto buildData = createBottomLevelAccelerationStructureBuildDataAABB(
				    logicalDevice,
				    objectBuffers.tetrahedronsAABBBufferHandles[i],
				    static_cast<int>(instanceCustomIndex),
				    tetrahedrons2[i].getTransform().getTransformMatrix());

				tetrahedrons2[i].setInstanceIndex(blasBuildDataList.size());
				blasBuildDataList.push_back(buildData);
				gpuObjects.push_back(GPUInstance(ObjectType::t_Tetrahedron2, i));
			}
		}

		if (rectangularSurfaces2x2List.size() > 0)
		{
			for (size_t i = 0; i < rectangularSurfaces2x2List.size(); i++)
			{
				size_t instanceCustomIndex = gpuObjects.size();
				auto buildData = createBottomLevelAccelerationStructureBuildDataAABB(
				    logicalDevice,
				    objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles[i],
				    static_cast<int>(instanceCustomIndex),
				    rectangularBezierSurfaces2x2[i].getTransform().getTransformMatrix());

				rectangularBezierSurfaces2x2[i].setInstanceIndex(blasBuildDataList.size());
				gpuObjects.push_back(GPUInstance(ObjectType::t_RectangularBezierSurface2x2, i));
				blasBuildDataList.push_back(buildData);
			}
		}

		// for (auto&& obj : meshObjects)
		// {
		// 	loadAndCreateVertexAndIndexBufferForModel(
		// 	    logicalDevice, physicalDevice, deletionQueue, obj);

		// 	// =========================================================================
		// 	// Bottom Level Acceleration Structure
		// 	BLASInstance triangleBLAS = createBottomLevelAccelerationStructureTriangle(
		// 	    obj.primitiveCount,
		// 	    static_cast<uint32_t>(obj.vertices.size()),
		// 	    obj.vertexBufferDeviceAddress,
		// 	    obj.indexBufferDeviceAddress,
		// 	    obj.getTransform().getTransformMatrix());
		// 	blasInstancesData.push_back(triangleBLAS);
		// }

		// update buffer data
		copyDataToBuffer(logicalDevice,
		                 objectBuffers.gpuObjectsDeviceMemoryHandle,
		                 gpuObjects.data(),
		                 sizeof(GPUInstance) * instancesCount);

		return blasBuildDataList;
	}

	// const std::vector<TLASInstance> collectAllTLASInstances()
	// {
	// 	// create vector to hold all BLAS instances
	// 	std::vector<TLASInstance> instances;

	// 	for (auto& obj : spheres)
	// 	{
	// 		auto& blasInstancesData = obj.getTLASInstance();
	// 		instances.push_back(blasInstancesData);
	// 	}

	// 	for (auto& obj : tetrahedrons2)
	// 	{
	// 		auto& blasInstancesData = obj.getTLASInstance();
	// 		instances.push_back(blasInstancesData);
	// 	}

	// 	for (auto& obj : rectangularBezierSurfaces2x2)
	// 	{
	// 		auto& blasInstancesData = obj.getTLASInstance();
	// 		instances.push_back(blasInstancesData);
	// 	}
	// 	return instances;
	// }

	[[nodiscard]]
	const std::vector<VkAccelerationStructureInstanceKHR>
	buildBLASInstancesFromBuildDataList(const std::vector<BLASBuildData>& BLASBuildDataList,
	                                    const VkCommandBuffer bottomLevelCommandBuffer,
	                                    const VkQueue graphicsQueue,
	                                    VkFence accelerationStructureBuildFence)
	{
		std::vector<VkAccelerationStructureInstanceKHR> instances;
		for (const auto& buildData : BLASBuildDataList)
		{
			// build the BLAS on GPI
			VkAccelerationStructureKHR bottomLevelAccelerationStructure
			    = buildBottomLevelAccelerationStructure(physicalDevice,
			                                            logicalDevice,
			                                            deletionQueueForAccelerationStructure,
			                                            bottomLevelCommandBuffer,
			                                            graphicsQueue,
			                                            buildData,
			                                            accelerationStructureBuildFence);

			// retrieve the device address of the built acceleration structure
			VkAccelerationStructureDeviceAddressInfoKHR
			    bottomLevelAccelerationStructureDeviceAddressInfo
			    = {
			        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
			        .pNext = NULL,
			        .accelerationStructure = bottomLevelAccelerationStructure,
			    };

			VkDeviceAddress bottomLevelAccelerationStructureDeviceAddress
			    = ltracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
			        logicalDevice, &bottomLevelAccelerationStructureDeviceAddressInfo);

			// create the blas instance
			auto bottomLevelGeometryInstance = VkAccelerationStructureInstanceKHR{
			    .transform = buildData.transformMatrix,
			    // TODO: maybe add a method that makes sure objectType does not exceed 24 bits
			    // see:
			    // https://registry.khronos.org/vulkan/specs/latest/man/html/InstanceCustomIndexKHR.html
			    // only grab 24 bits
			    .instanceCustomIndex
			    = static_cast<uint32_t>(buildData.instanceCustomIndex) & 0xFFFFFF,
			    .mask = 0xFF,
			    .instanceShaderBindingTableRecordOffset = 0,
			    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			    .accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress,
			};

			instances.push_back(std::move(bottomLevelGeometryInstance));
		}
		return instances;
	}

	void createTLAS(
	    const bool onlyUpdate,
	    VkDeviceMemory& blasGeometryInstancesDeviceMemoryHandle,
	    VkAccelerationStructureGeometryKHR& topLevelAccelerationStructureGeometry,
	    VkAccelerationStructureBuildGeometryInfoKHR& topLevelAccelerationStructureBuildGeometryInfo,
	    VkAccelerationStructureKHR& topLevelAccelerationStructureHandle,
	    VkAccelerationStructureBuildSizesInfoKHR& topLevelAccelerationStructureBuildSizesInfo,
	    VkBuffer& topLevelAccelerationStructureBufferHandle,
	    VkDeviceMemory& topLevelAccelerationStructureDeviceMemoryHandle,
	    VkBuffer& topLevelAccelerationStructureScratchBufferHandle,
	    VkDeviceMemory& topLevelAccelerationStructureDeviceScratchMemoryHandle,
	    VkAccelerationStructureBuildRangeInfoKHR& topLevelAccelerationStructureBuildRangeInfo,
	    VkCommandBuffer commandBufferBuildTopAndBottomLevel,
	    VkQueue graphicsQueueHandle,
	    VkBuffer& uniformBufferHandle,
	    VkDeviceMemory& uniformDeviceMemoryHandle,
	    UniformStructure& uniformStructure)
	{
		if (blasInstances.size() > 0)
		{
			createAndBuildTopLevelAccelerationStructure(
			    blasInstances,
			    deletionQueueForAccelerationStructure,
			    logicalDevice,
			    physicalDevice,
			    onlyUpdate,
			    blasGeometryInstancesDeviceMemoryHandle,
			    topLevelAccelerationStructureGeometry,
			    topLevelAccelerationStructureBuildGeometryInfo,
			    topLevelAccelerationStructureHandle,
			    topLevelAccelerationStructureBuildSizesInfo,
			    topLevelAccelerationStructureBufferHandle,
			    topLevelAccelerationStructureDeviceMemoryHandle,
			    topLevelAccelerationStructureScratchBufferHandle,
			    topLevelAccelerationStructureDeviceScratchMemoryHandle,
			    topLevelAccelerationStructureBuildRangeInfo,
			    commandBufferBuildTopAndBottomLevel,
			    graphicsQueueHandle);
			// =========================================================================
			// Uniform Buffer
			if (!onlyUpdate)
			{
				createBuffer(physicalDevice,
				             logicalDevice,
				             deletionQueueForAccelerationStructure,
				             sizeof(UniformStructure),
				             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             uniformBufferHandle,
				             uniformDeviceMemoryHandle);

				copyDataToBuffer(logicalDevice,
				                 uniformDeviceMemoryHandle,
				                 &uniformStructure,
				                 sizeof(UniformStructure));
			}
		}
	}

  private:
	std::vector<GPUInstance> gpuObjects;
	std::vector<RaytracingWorldObject<Sphere>> spheres;
	std::vector<RaytracingWorldObject<Tetrahedron2>> tetrahedrons2;
	std::vector<RaytracingWorldObject<RectangularBezierSurface2x2>> rectangularBezierSurfaces2x2;
	std::vector<MeshObject> meshObjects;
	std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	DeletionQueue deletionQueueForAccelerationStructure;

	// the objects that are rendered using ray tracing (with an intersection shader)
	RaytracingObjectBuffers objectBuffers;
};

} // namespace rt
} // namespace ltracer
