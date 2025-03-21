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

	inline const size_t& getBLASInstancesCount() const
	{
		return blasInstancesCount;
	}

	MeshObject& addObjectMesh(const MeshObject& meshObject)
	{
		meshObjects.push_back(meshObject);
		return meshObjects.back();
	}

	RaytracingWorldObject<Sphere>&
	addObjectSphere(const glm::vec3 position, const float radius, const ColorIdx colorIdx)
	{
		Sphere sphere{
		    .center = position,
		    .radius = radius,
		    .colorIdx = static_cast<int>(colorIdx),
		};
		spheres.emplace_back(ObjectType::t_Sphere, AABB::fromSphere(sphere), sphere, position);
		return spheres.back();
	}

	RaytracingWorldObject<BezierTriangle2>&
	addObjectBezierTriangle(const BezierTriangle2& bezierTriangle)
	{
		AABB aabb = AABB::fromBezierTriangle2(bezierTriangle);
		bezierTriangles2.emplace_back(
		    ObjectType::t_BezierTriangle2, aabb, bezierTriangle, glm::vec3(0));
		return bezierTriangles2.back();
	}

	RaytracingWorldObject<RectangularBezierSurface2x2>&
	addObjectRectangularBezierSurface2x2(const RectangularBezierSurface2x2& surface)
	{
		AABB aabb = AABB::fromRectangularBezierSurface2x2(surface);
		rectangularBezierSurfaces2x2.emplace_back(
		    ObjectType::t_RectangularBezierSurface2x2, aabb, surface, glm::vec3(0));
		return rectangularBezierSurfaces2x2.back();
	}

	void addSlicingPlane(const SlicingPlane& slicingPlane)
	{
		slicingPlanes.push_back(slicingPlane);
	}

	void addSidesFromTetrahedronAsBezierTriangles(const Tetrahedron2& tetrahedron2,
	                                              const int subdivisions = 0)
	{
		for (int side = 1; side <= 4; side++)
		{
			const auto& bezierTriangle
			    = ltracer::extractBezierTriangleFromTetrahedron(tetrahedron2, side);

			std::vector<BezierTriangle2> subTriangles;
			subTriangles.reserve(static_cast<size_t>(std::powl(4, subdivisions)));
			subTriangles.push_back(bezierTriangle);

			for (int i = 0; i < subdivisions; i++)
			{
				for (size_t j = 0; j < std::powl(4, i); j++)
				{
					BezierTriangle2& t = subTriangles.front();
					const auto& s = ltracer::subdivideBezierTriangle2(t);

					subTriangles.push_back(s.bottomLeft);
					subTriangles.push_back(s.bottomRight);
					subTriangles.push_back(s.top);
					subTriangles.push_back(s.center);

					subTriangles.erase(subTriangles.begin());
				}
			}

			for (const auto& subTriangle : subTriangles)
			{
				addObjectBezierTriangle(subTriangle);
			}
			// visualizeTetrahedron2(raytracingScene, tetrahedron2);
		}
	}

	std::vector<RaytracingWorldObject<Sphere>>& getWorldObjectSpheres()
	{
		return spheres;
	}

	std::vector<RaytracingWorldObject<Tetrahedron2>>& getWorldObjectTetrahedrons()
	{
		return tetrahedrons2;
	}

	std::vector<RaytracingWorldObject<BezierTriangle2>>& getWorldObjectBezierTriangles()
	{
		return bezierTriangles2;
	}

	std::vector<RaytracingWorldObject<RectangularBezierSurface2x2>>&
	getWorldObjectRectangularBezierSurfaces2x2()
	{
		return rectangularBezierSurfaces2x2;
	}

	std::vector<SlicingPlane>& getSlicingPlanes()
	{
		return slicingPlanes;
	}

	inline void setTransformMatrixForInstance(const size_t instanceIndex,
	                                          const VkTransformMatrixKHR& matrix)
	{
		blasInstances[instanceIndex].transform = matrix;
	}

	void createSlicingPlanesBuffer()
	{
		if (slicingPlanes.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             deletionQueueForAccelerationStructure,
			             slicingPlanes.size() * sizeof(SlicingPlane),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.slicingPlanesBufferHandle,
			             objectBuffers.slicingPlanesDeviceMemoryHandle);
		}
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

		if (bezierTriangles2.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             deletionQueueForAccelerationStructure,
			             bezierTriangles2.size() * sizeof(BezierTriangle2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.bezierTriangles2BufferHandle,
			             objectBuffers.bezierTriangles2DeviceMemoryHandles);
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

		size_t size = spheres.size() + tetrahedrons2.size() + bezierTriangles2.size()
		              + rectangularBezierSurfaces2x2.size();
		gpuObjects.resize(size);
	}

	void recreateAccelerationStructures(RaytracingInfo& raytracingInfo, const bool fullRebuild)
	{
		// We dont want to recreate the BLAS's but only update the related transform matrix inside
		// the TLAS that links to the particular BLAS
		if (fullRebuild)
		{
			// WARNING:	USE A POOL OF BUFFERS INSTEAD!!
			// TODO: Reuse created buffers instead of throwing them away and recreating them, this
			// e.g. instead of recreating 1000 buffers for each sphere, just reuse the buffer and
			// can save a lot of time,
			// assign  them new
			// if the amount of spheres changes, only create new buffers for the new spheres
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
			createSlicingPlanesBuffer();

			auto buildData = createBLASBuildDataForScene();
			blasInstances = buildBLASInstancesFromBuildDataList(
			    buildData,
			    raytracingInfo.commandBufferBuildTopAndBottomLevel,
			    raytracingInfo.graphicsQueueHandle,
			    raytracingInfo.accelerationStructureBuildFence);
			blasInstancesCount = blasInstances.size();

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

		if (bezierTriangles2.size() > 0)
		{
			std::vector<BezierTriangle2> bezierTriangles2List;
			for (size_t i = 0; i < bezierTriangles2.size(); i++)
			{
				auto& obj = bezierTriangles2[i];
				bezierTriangles2List.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(logicalDevice,
			                 objectBuffers.bezierTriangles2DeviceMemoryHandles,
			                 bezierTriangles2List.data(),
			                 sizeof(BezierTriangle2) * bezierTriangles2List.size());
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

		if (slicingPlanes.size() > 0)
		{
			copyDataToBuffer(logicalDevice,
			                 objectBuffers.slicingPlanesDeviceMemoryHandle,
			                 slicingPlanes.data(),
			                 sizeof(SlicingPlane) * slicingPlanes.size());
		}
	}

  private:
	template <typename T>
	void addObjectsToBLASBuildDataListAndGPUObjectsList(
	    std::vector<BLASBuildData>& blasBuildDataList,
	    std::vector<RaytracingWorldObject<T>>& referenceList,
	    const ObjectType objectType,
	    const std::vector<T>& objectData,
	    const std::vector<VkBuffer>& aabbBufferHandles)
	{
		for (size_t i = 0; i < objectData.size(); i++)
		{
			size_t instanceCustomIndex = gpuObjects.size();
			auto buildData = createBottomLevelAccelerationStructureBuildDataAABB(
			    logicalDevice,
			    aabbBufferHandles[i],
			    static_cast<int>(instanceCustomIndex),
			    referenceList[i].getTransform().getTransformMatrix());

			referenceList[i].setInstanceIndex(blasBuildDataList.size());
			gpuObjects.push_back(GPUInstance(objectType, i));
			blasBuildDataList.push_back(buildData);
		}
	}

	// Extracts the data of the objects from the RaytracingWorldObject<T> vector into a vector
	// and creates and copies the AABB to the corresponding buffer
	template <typename T>
	std::vector<T> extractAndCopyAABBsToBuffer(const std::vector<RaytracingWorldObject<T>>& objects,
	                                           std::vector<VkBuffer>& aabbBufferHandles,
	                                           std::vector<VkDeviceMemory>& aabbDeviceMemoryHandles,
	                                           std::vector<AABB>& objectsAABBs)
	{
		std::vector<T> extractedObjects;
		if (objects.size() > 0)
		{
			aabbBufferHandles.resize(objects.size());
			aabbDeviceMemoryHandles.resize(objects.size());
			for (size_t i = 0; i < objects.size(); i++)
			{
				auto& obj = objects[i];
				objectsAABBs.push_back(obj.getGeometry().getAABB());
				extractedObjects.push_back(obj.getGeometry().getData());

				createBuffer(physicalDevice,
				             logicalDevice,
				             deletionQueueForAccelerationStructure,
				             sizeof(VkAabbPositionsKHR),
				             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
				                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             aabbBufferHandles[i],
				             aabbDeviceMemoryHandles[i]);

				// Note: for now, all instances only contain one AABB, so we only need to copy
				// one object into the buffer
				const auto aabbPositions = obj.getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(
				    logicalDevice, aabbDeviceMemoryHandles[i], &aabbPositions, sizeof(AABB));
			}
		}
		return extractedObjects;
	}

	[[nodiscard]] const std::vector<BLASBuildData> createBLASBuildDataForScene()
	{
		copyObjectsToBuffers();

		std::vector<ltracer::AABB> aabbsSpheres;
		std::vector<ltracer::AABB> aabbsTetrahedrons;
		std::vector<ltracer::AABB> aabbsBezierTriangles2;
		std::vector<ltracer::AABB> aabbsRectangularSurfaces2x2;

		// extract and copy AABBs to buffers
		std::vector<Sphere> spheresList
		    = extractAndCopyAABBsToBuffer(spheres,
		                                  objectBuffers.spheresAABBBufferHandles,
		                                  objectBuffers.spheresAABBDeviceMemoryHandles,
		                                  aabbsSpheres);

		std::vector<Tetrahedron2> tetrahedrons2List
		    = extractAndCopyAABBsToBuffer(tetrahedrons2,
		                                  objectBuffers.tetrahedronsAABBBufferHandles,
		                                  objectBuffers.tetrahedronsAABBDeviceMemoryHandles,
		                                  aabbsTetrahedrons);

		std::vector<BezierTriangle2> bezierTriangles2List
		    = extractAndCopyAABBsToBuffer(bezierTriangles2,
		                                  objectBuffers.bezierTriangles2AABBBufferHandles,
		                                  objectBuffers.bezierTriangles2AABBDeviceMemoryHandles,
		                                  aabbsBezierTriangles2);

		std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List
		    = extractAndCopyAABBsToBuffer(
		        rectangularBezierSurfaces2x2,
		        objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles,
		        objectBuffers.rectangularBezierSurfacesAABB2x2AABBDeviceMemoryHandles,
		        aabbsRectangularSurfaces2x2);

		VkDeviceSize instancesCount = spheresList.size() + tetrahedrons2List.size()
		                              + bezierTriangles2List.size()
		                              + rectangularSurfaces2x2List.size();

		// FIXME: it is allowed when we do a full rebuild, check if this is a full rebuild
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

		// for each object add the extracted object data to the gpuObjects vector and create the
		// buildData in blasBuildDataList
		addObjectsToBLASBuildDataListAndGPUObjectsList(blasBuildDataList,
		                                               spheres,
		                                               ObjectType::t_Sphere,
		                                               spheresList,
		                                               objectBuffers.spheresAABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(blasBuildDataList,
		                                               tetrahedrons2,
		                                               ObjectType::t_Tetrahedron2,
		                                               tetrahedrons2List,
		                                               objectBuffers.tetrahedronsAABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    bezierTriangles2,
		    ObjectType::t_BezierTriangle2,
		    bezierTriangles2List,
		    objectBuffers.bezierTriangles2AABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    rectangularBezierSurfaces2x2,
		    ObjectType::t_RectangularBezierSurface2x2,
		    rectangularSurfaces2x2List,
		    objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles);

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
	std::vector<SlicingPlane> slicingPlanes;
	std::vector<RaytracingWorldObject<BezierTriangle2>> bezierTriangles2;
	std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

	size_t blasInstancesCount = 0;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	DeletionQueue deletionQueueForAccelerationStructure;

	// the objects that are rendered using ray tracing (with an intersection shader)
	RaytracingObjectBuffers objectBuffers;
};

} // namespace rt
} // namespace ltracer
