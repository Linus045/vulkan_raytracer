#pragma once

#include <vector>
#include <cmath>

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

// TODO: move functions into raytracing_scene.cpp file

namespace tracer
{
// forward declaration
class Renderer;

namespace ui
{
struct UIData;
}

namespace rt
{

class RaytracingScene
{
  public:
	static const int SCENE_COUNT = 6;
	static const int INITIAL_SCENE = 1;

	inline static const std::vector<std::string> sceneNames = {
	    "Tetrahedron degree 2 deformed slightly",
	    "Tetrahedron degree 2 deformed strongly",
	    "Two Tetrahedrons degree 2 stuck together",
	    "Tetrahedron degree 3 deformed slightly",
	    "Tetrahedron degree 2 random control points",
	    "A bunch of random tetrahedrons",
	};

	RaytracingScene(const VkPhysicalDevice& physicalDevice,
	                const VkDevice logicalDevice,
	                const VmaAllocator vmaAllocator)
	    : physicalDevice(physicalDevice), logicalDevice(logicalDevice),
	      vmaAllocator(vmaAllocator) {};

	~RaytracingScene() = default;

	RaytracingScene(const RaytracingScene&) = delete;
	RaytracingScene& operator=(const RaytracingScene&) = delete;

	RaytracingScene(RaytracingScene&&) noexcept = delete;
	RaytracingScene& operator=(RaytracingScene&&) noexcept = delete;

	static void
	loadScene(const Renderer& renderer, RaytracingScene& raytracingScene, const int index);

	inline static int getSceneCount()
	{
		return SCENE_COUNT;
	}

	static std::string getSceneName(const int sceneNr)
	{
		return sceneNames[static_cast<size_t>(sceneNr - 1)];
	}

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

	RaytracingWorldObject<BezierTriangle3>&
	addObjectBezierTriangle(const BezierTriangle3& bezierTriangle)
	{
		AABB aabb = AABB::fromBezierTriangle3(bezierTriangle);
		bezierTriangles3.emplace_back(
		    ObjectType::t_BezierTriangle3, aabb, bezierTriangle, glm::vec3(0));
		return bezierTriangles3.back();
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
	                                              const std::array<bool, 4>& sides
	                                              = {true, true, true, true},
	                                              const int subdivisions = 0)
	{
		for (int side = 1; side <= 4; side++)
		{
			if (!sides[static_cast<size_t>(side - 1)])
			{
				continue;
			}

			const auto& bezierTriangle
			    = tracer::extractBezierTriangleFromTetrahedron(tetrahedron2, side);

			std::vector<BezierTriangle2> subTriangles;
			subTriangles.reserve(
			    static_cast<size_t>(std::pow(4.0L, static_cast<long double>(subdivisions))));
			subTriangles.push_back(bezierTriangle);

			assert(subdivisions == 0 && "Subdivisions > 0 not implemented");
			// for (int i = 0; i < subdivisions; i++)
			// {
			// 	for (size_t j = 0; j < std::pow(4.0L, static_cast<long double>(i)); j++)
			// 	{
			// 		BezierTriangle2& t = subTriangles.front();
			// 		const auto& s = tracer::subdivideBezierTriangle2(t);

			// 		subTriangles.push_back(s.bottomLeft);
			// 		subTriangles.push_back(s.bottomRight);
			// 		subTriangles.push_back(s.top);
			// 		subTriangles.push_back(s.center);
			// 		subTriangles.erase(subTriangles.begin());

			// 		// visualize center points
			// 		// for (int li = 0; li < 6; li++)
			// 		// {
			// 		// 	if (li <= 2)
			// 		// 	{
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.06f, ColorIdx::t_white);
			// 		// 	}
			// 		// 	else
			// 		// 	{
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.03f, ColorIdx::t_yellow);
			// 		// 	}
			// 		// }
			// 	}
			// }

			for (const auto& subTriangle : subTriangles)
			{
				addObjectBezierTriangle(subTriangle);
			}
			// visualizeTetrahedron2(raytracingScene, tetrahedron2);
		}
	}

	void addSidesFromTetrahedronAsBezierTriangles(const Tetrahedron3& tetrahedron3,
	                                              const std::array<bool, 4>& sides
	                                              = {true, true, true, true},
	                                              const int subdivisions = 0)
	{
		for (int side = 1; side <= 4; side++)
		{
			if (!sides[static_cast<size_t>(side - 1)])
			{
				continue;
			}

			const auto& bezierTriangle
			    = tracer::extractBezierTriangleFromTetrahedron(tetrahedron3, side);

			std::vector<BezierTriangle3> subTriangles;
			subTriangles.reserve(
			    static_cast<size_t>(std::pow(4.0L, static_cast<long double>(subdivisions))));
			subTriangles.push_back(bezierTriangle);

			assert(subdivisions == 0 && "Subdivisions > 0 not implemented");
			// for (int i = 0; i < subdivisions; i++)
			// {
			// 	for (size_t j = 0; j < std::pow(4.0L, static_cast<long double>(i)); j++)
			// 	{
			// 		BezierTriangle2& t = subTriangles.front();
			// 		const auto& s = tracer::subdivideBezierTriangle2(t);

			// 		subTriangles.push_back(s.bottomLeft);
			// 		subTriangles.push_back(s.bottomRight);
			// 		subTriangles.push_back(s.top);
			// 		subTriangles.push_back(s.center);
			// 		subTriangles.erase(subTriangles.begin());

			// 		// visualize center points
			// 		// for (int li = 0; li < 6; li++)
			// 		// {
			// 		// 	if (li <= 2)
			// 		// 	{
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.06f, ColorIdx::t_white);
			// 		// 	}
			// 		// 	else
			// 		// 	{
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.03f, ColorIdx::t_yellow);
			// 		// 	}
			// 		// }
			// 	}
			// }

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

	std::vector<RaytracingWorldObject<BezierTriangle2>>& getWorldObjectBezierTriangles2()
	{
		return bezierTriangles2;
	}

	std::vector<RaytracingWorldObject<BezierTriangle3>>& getWorldObjectBezierTriangles3()
	{
		return bezierTriangles3;
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
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             slicingPlanes.size() * sizeof(SlicingPlane),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.slicingPlanesBufferHandle,
			             objectBuffers.slicingPlanesBufferAllocation);
		}
	}

	void createBuffers()
	{
		if (spheres.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             spheres.size() * sizeof(Sphere),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.spheresBufferHandle,
			             objectBuffers.spheresBufferAllocation);
		}

		if (tetrahedrons2.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             tetrahedrons2.size() * sizeof(Tetrahedron2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.tetrahedronsBufferHandle,
			             objectBuffers.tetrahedronsBufferAllocation);
		}

		if (bezierTriangles2.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             bezierTriangles2.size() * sizeof(BezierTriangle2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.bezierTriangles2BufferHandle,
			             objectBuffers.bezierTriangles2BufferAllocation);
		}

		if (bezierTriangles3.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             bezierTriangles3.size() * sizeof(BezierTriangle3),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.bezierTriangles3BufferHandle,
			             objectBuffers.bezierTriangles3BuffersAllocation);
		}

		if (rectangularBezierSurfaces2x2.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             rectangularBezierSurfaces2x2.size() * sizeof(RectangularBezierSurface2x2),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             objectBuffers.rectangularBezierSurfaces2x2BufferHandle,
			             objectBuffers.rectangularBezierSurfaces2x2BufferAllocation);
		}

		size_t size = spheres.size() + tetrahedrons2.size() + bezierTriangles2.size()
		              + rectangularBezierSurfaces2x2.size() + bezierTriangles3.size();
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

			objectBuffers.clearAllHandles();

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
			           raytracingInfo.uniformBufferAllocation,
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
			           raytracingInfo.uniformBufferAllocation,
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
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.spheresBufferAllocation,
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
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.tetrahedronsBufferAllocation,
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
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.bezierTriangles2BufferAllocation,
			                 bezierTriangles2List.data(),
			                 sizeof(BezierTriangle2) * bezierTriangles2List.size());
		}

		if (bezierTriangles3.size() > 0)
		{
			std::vector<BezierTriangle3> bezierTriangles3List;
			for (size_t i = 0; i < bezierTriangles3.size(); i++)
			{
				auto& obj = bezierTriangles3[i];
				bezierTriangles3List.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.bezierTriangles3BuffersAllocation,
			                 bezierTriangles3List.data(),
			                 sizeof(BezierTriangle3) * bezierTriangles3List.size());
		}

		if (rectangularBezierSurfaces2x2.size() > 0)
		{
			std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List;
			for (size_t i = 0; i < rectangularBezierSurfaces2x2.size(); i++)
			{
				auto& obj = rectangularBezierSurfaces2x2[i];
				rectangularSurfaces2x2List.push_back(obj.getGeometry().getData());
			}
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.rectangularBezierSurfaces2x2BufferAllocation,
			                 rectangularSurfaces2x2List.data(),
			                 sizeof(RectangularBezierSurface2x2)
			                     * rectangularSurfaces2x2List.size());
		}

		if (slicingPlanes.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 objectBuffers.slicingPlanesBufferAllocation,
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
	                                           std::vector<VmaAllocation>& aabbBufferAllocations,
	                                           std::vector<AABB>& objectsAABBs)
	{
		std::vector<T> extractedObjects;
		if (objects.size() > 0)
		{
			aabbBufferHandles.resize(objects.size());
			aabbBufferAllocations.resize(objects.size());
			for (size_t i = 0; i < objects.size(); i++)
			{
				auto& obj = objects[i];
				objectsAABBs.push_back(obj.getGeometry().getAABB());
				extractedObjects.push_back(obj.getGeometry().getData());

				createBuffer(physicalDevice,
				             logicalDevice,
				             vmaAllocator,
				             deletionQueueForAccelerationStructure,
				             sizeof(VkAabbPositionsKHR),
				             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
				                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             aabbBufferHandles[i],
				             aabbBufferAllocations[i]);

				// Note: for now, all instances only contain one AABB, so we only need to copy
				// one object into the buffer
				const auto aabbPositions = obj.getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(
				    vmaAllocator, aabbBufferAllocations[i], &aabbPositions, sizeof(AABB));
			}
		}
		return extractedObjects;
	}

	[[nodiscard]] const std::vector<BLASBuildData> createBLASBuildDataForScene()
	{
		copyObjectsToBuffers();

		std::vector<tracer::AABB> aabbsSpheres;
		std::vector<tracer::AABB> aabbsTetrahedrons;
		std::vector<tracer::AABB> aabbsBezierTriangles2;
		std::vector<tracer::AABB> aabbsBezierTriangles3;
		std::vector<tracer::AABB> aabbsRectangularSurfaces2x2;

		// extract and copy AABBs to buffers
		std::vector<Sphere> spheresList
		    = extractAndCopyAABBsToBuffer(spheres,
		                                  objectBuffers.spheresAABBBufferHandles,
		                                  objectBuffers.spheresAABBBufferAllocations,
		                                  aabbsSpheres);

		std::vector<Tetrahedron2> tetrahedrons2List
		    = extractAndCopyAABBsToBuffer(tetrahedrons2,
		                                  objectBuffers.tetrahedronsAABBBufferHandles,
		                                  objectBuffers.tetrahedronsAABBBufferAllocations,
		                                  aabbsTetrahedrons);

		std::vector<BezierTriangle2> bezierTriangles2List
		    = extractAndCopyAABBsToBuffer(bezierTriangles2,
		                                  objectBuffers.bezierTriangles2AABBBufferHandles,
		                                  objectBuffers.bezierTriangles2AABBBufferAllocations,
		                                  aabbsBezierTriangles2);

		std::vector<BezierTriangle3> bezierTriangles3List
		    = extractAndCopyAABBsToBuffer(bezierTriangles3,
		                                  objectBuffers.bezierTriangles3AABBBufferHandles,
		                                  objectBuffers.bezierTriangles3AABBBufferAllocations,
		                                  aabbsBezierTriangles3);

		std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List
		    = extractAndCopyAABBsToBuffer(
		        rectangularBezierSurfaces2x2,
		        objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandles,
		        objectBuffers.rectangularBezierSurfacesAABB2x2AABBBufferAllocations,
		        aabbsRectangularSurfaces2x2);

		VkDeviceSize instancesCount = spheresList.size() + tetrahedrons2List.size()
		                              + bezierTriangles2List.size() + bezierTriangles3List.size()
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
		             vmaAllocator,
		             deletionQueueForAccelerationStructure,
		             sizeof(GPUInstance) * instancesCount,
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		             memoryAllocateFlagsInfo,
		             objectBuffers.gpuObjectsBufferHandle,
		             objectBuffers.gpuObjectsBufferAllocation);

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
		    bezierTriangles3,
		    ObjectType::t_BezierTriangle3,
		    bezierTriangles3List,
		    objectBuffers.bezierTriangles3AABBBufferHandles);

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
		copyDataToBuffer(vmaAllocator,
		                 objectBuffers.gpuObjectsBufferAllocation,
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

	[[nodiscard]] const std::vector<VkAccelerationStructureInstanceKHR>
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
			                                            vmaAllocator,
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
			    = tracer::procedures::pvkGetAccelerationStructureDeviceAddressKHR(
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
	    VmaAllocation& blasGeometryInstancesDeviceMemoryHandle,
	    VkAccelerationStructureGeometryKHR& topLevelAccelerationStructureGeometry,
	    VkAccelerationStructureBuildGeometryInfoKHR& topLevelAccelerationStructureBuildGeometryInfo,
	    VkAccelerationStructureKHR& topLevelAccelerationStructureHandle,
	    VkAccelerationStructureBuildSizesInfoKHR& topLevelAccelerationStructureBuildSizesInfo,
	    VkBuffer& topLevelAccelerationStructureBufferHandle,
	    VmaAllocation& topLevelAccelerationStructureDeviceMemoryHandle,
	    VkBuffer& topLevelAccelerationStructureScratchBufferHandle,
	    VmaAllocation& topLevelAccelerationStructureDeviceScratchMemoryHandle,
	    VkAccelerationStructureBuildRangeInfoKHR& topLevelAccelerationStructureBuildRangeInfo,
	    VkCommandBuffer commandBufferBuildTopAndBottomLevel,
	    VkQueue graphicsQueueHandle,
	    VkBuffer& uniformBufferHandle,
	    VmaAllocation& uniformBufferAllocation,
	    UniformStructure& uniformStructure)
	{
		if (blasInstances.size() > 0)
		{
			createAndBuildTopLevelAccelerationStructure(
			    blasInstances,
			    deletionQueueForAccelerationStructure,
			    logicalDevice,
			    physicalDevice,
			    vmaAllocator,
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
				             vmaAllocator,
				             deletionQueueForAccelerationStructure,
				             sizeof(UniformStructure),
				             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             uniformBufferHandle,
				             uniformBufferAllocation);

				copyDataToBuffer(vmaAllocator,
				                 uniformBufferAllocation,
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
	std::vector<RaytracingWorldObject<BezierTriangle3>> bezierTriangles3;
	std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

	size_t blasInstancesCount = 0;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VmaAllocator vmaAllocator;
	DeletionQueue deletionQueueForAccelerationStructure;

	// the objects that are rendered using ray tracing (with an intersection shader)
	RaytracingObjectBuffers objectBuffers;
};

} // namespace rt
} // namespace tracer
