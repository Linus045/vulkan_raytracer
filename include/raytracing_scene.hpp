#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
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
#include "ui.hpp"
#include "vk_utils.hpp"

// TODO: move functions into raytracing_scene.cpp file

namespace tracer
{
// forward declaration
class Renderer;

struct SceneConfig
{
	bool visualizeControlPoints;
	bool visualizeSampledSurface;
	bool visualizeSampledVolume;

	static const SceneConfig fromUIData(const ui::UIData& uiData)
	{
		return SceneConfig{
		    .visualizeControlPoints
		    = uiData.raytracingDataConstants.debugVisualizeControlPoints > 0.0f,
		    .visualizeSampledSurface
		    = uiData.raytracingDataConstants.debugVisualizeSampledSurface > 0.0f,
		    .visualizeSampledVolume
		    = uiData.raytracingDataConstants.debugVisualizeSampledVolume > 0.0f,
		};
	}
};

namespace rt
{

// TODO: give this a more fitting name, since its not really a singular scene but actually
// controlling the current scene and its objects
class RaytracingScene
{
  public:
	static const int SCENE_COUNT = 8;
	static const int INITIAL_SCENE = 1;

	VkBuffer gpuObjectsBufferHandle = VK_NULL_HANDLE;
	VkBuffer slicingPlanesBufferHandle = VK_NULL_HANDLE;

	VkBuffer spheresBufferHandle = VkBuffer(0);
	// VkBuffer tetrahedronsBufferHandle = VkBuffer(0);
	VkBuffer bezierTriangles2BufferHandle = VkBuffer(0);
	VkBuffer bezierTriangles3BufferHandle = VkBuffer(0);
	VkBuffer bezierTriangles4BufferHandle = VkBuffer(0);
	VkBuffer rectangularBezierSurfaces2x2BufferHandle = VkBuffer(0);
	inline static const std::vector<std::string> sceneNames = {
	    "Tetrahedron degree 2 deformed slightly",
	    "Tetrahedron degree 2 deformed strongly",
	    "Two Tetrahedrons degree 2 stuck together",
	    "Tetrahedron degree 3 deformed slightly",
	    "Tetrahedron degree 2 random control points",
	    "A bunch of random tetrahedrons",
	    "Tetrahedron degree 4",
	    "Tetrahedron degree 4 - inside volume pulled out",
	};

	RaytracingScene(const VkPhysicalDevice& physicalDevice,
	                const VkDevice logicalDevice,
	                const VmaAllocator vmaAllocator)
	    : physicalDevice(physicalDevice), logicalDevice(logicalDevice), vmaAllocator(vmaAllocator)
	{
		// TODO: allow multiple slicing planes
		// we always wanna create one slicing plane
		addSlicingPlane(SlicingPlane{
		    glm::vec3(0.7, 0, 0),
		    glm::vec3(-1, 0, 0),
		});
	};

	~RaytracingScene() = default;

	RaytracingScene(const RaytracingScene&) = delete;
	RaytracingScene& operator=(const RaytracingScene&) = delete;

	RaytracingScene(RaytracingScene&&) noexcept = delete;
	RaytracingScene& operator=(RaytracingScene&&) noexcept = delete;

	static void loadScene(const Renderer& renderer,
	                      RaytracingScene& raytracingScene,
	                      const SceneConfig sceneConfig,
	                      const int index);

	inline static int getSceneCount()
	{
		return SCENE_COUNT;
	}

	inline int getCurrentSceneNr() const
	{
		return currentSceneNr;
	}

	static std::string getSceneName(const int sceneNr)
	{
		return sceneNames[static_cast<size_t>(sceneNr - 1)];
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

	// MeshObject& addObjectMesh(const MeshObject& meshObject)
	// {
	// 	meshObjects.push_back(meshObject);
	// 	return meshObjects.back();
	// }

	std::shared_ptr<RaytracingWorldObject<Sphere>> addObjectSphere(SceneObject& sceneObject,
	                                                               const glm::vec3 position,
	                                                               const float radius,
	                                                               const ColorIdx colorIdx)
	{
		Sphere sphere{
		    .center = position,
		    .radius = radius,
		    .colorIdx = static_cast<int>(colorIdx),
		};

		spheres.emplace_back(std::make_shared<RaytracingWorldObject<Sphere>>(
		    ObjectType::t_Sphere, AABB::fromSphere(sphere), sphere, position));
		auto& obj = spheres.back();
		sceneObject.spheres.push_back(obj);
		return obj;
	}

	template <typename S>
	std::vector<std::shared_ptr<RaytracingWorldObject<S>>>&
	getTriangleListForSceneObject(SceneObject& sceneObject);

	template <typename S>
	std::vector<std::shared_ptr<RaytracingWorldObject<S>>>& getTriangleList();

	template <typename S>
	std::shared_ptr<RaytracingWorldObject<S>> addObjectBezierTriangle(SceneObject& sceneObject,
	                                                                  const S& bezierTriangle,
	                                                                  const bool markAsInside)
	{
		AABB aabb = AABB::fromBezierTriangle(bezierTriangle);
		constexpr auto objectType = getObjectType<S>();

		ObjectType type = objectType;

		// mark as inside for the slicing plane calculations
		if (markAsInside)
		{
			switch (objectType)
			{
			case ObjectType::t_BezierTriangle1:
				type = ObjectType::t_BezierTriangleInside1;
				break;
			case ObjectType::t_BezierTriangle2:
				type = ObjectType::t_BezierTriangleInside2;
				break;
			case ObjectType::t_BezierTriangle3:
				type = ObjectType::t_BezierTriangleInside3;
				break;
			case ObjectType::t_BezierTriangle4:
				type = ObjectType::t_BezierTriangleInside4;
				break;
			default:
				throw std::runtime_error(
				    "addObjectBezierTriangle - Object type has no inside variant");
			}
		}

		auto& trianglesList = getTriangleList<S>();
		trianglesList.emplace_back(
		    std::make_shared<RaytracingWorldObject<S>>(type, aabb, bezierTriangle, glm::vec3(0)));
		auto& obj = trianglesList.back();

		auto& trianglesListForSceneObject = getTriangleListForSceneObject<S>(sceneObject);
		trianglesListForSceneObject.push_back(obj);
		return obj;
	}

	std::shared_ptr<RaytracingWorldObject<RectangularBezierSurface2x2>>
	addObjectRectangularBezierSurface2x2(SceneObject& sceneObject,
	                                     const RectangularBezierSurface2x2& surface)
	{
		AABB aabb = AABB::fromRectangularBezierSurface2x2(surface);
		rectangularBezierSurfaces2x2.emplace_back(
		    std::make_shared<RaytracingWorldObject<RectangularBezierSurface2x2>>(
		        ObjectType::t_RectangularBezierSurface2x2, aabb, surface, glm::vec3(0)));
		auto& obj = rectangularBezierSurfaces2x2.back();
		sceneObject.rectangularBezierSurfaces2x2.emplace_back(obj);
		return obj;
	}

	void addSlicingPlane(const SlicingPlane& slicingPlane)
	{
		slicingPlanes.push_back(slicingPlane);
	}

	void clearScene();

	/**
	 * @brief extracts the 4 sides from the tetehedron and adds them to the scene
	 *
	 * @param tetrahedron the tetrahedron to extract the sides from
	 * @param extractSide indicates if the side should be added
	 * @param markTriangleAsInside indicates if the triangle should be marked as inside (used
	 * for slicing plane calculations)
	 * @param subdivisions NOT IMPLEMENTED - sets the amount of subdivisions
	 *
	 * sids are as follows using a simple tetrahedron as reference:
	 * 0: front face (negative x)
	 * 1: bottom face (negative y)
	 * 2: left face (negative z)
	 * 3: right face (positive x/z)
	 */
	template <typename T>
	void addSidesFromTetrahedronAsBezierTriangles(SceneObject& sceneObject,
	                                              const T& tetrahedron,
	                                              const std::array<bool, 4>& extractSide
	                                              = {true, true, true, true},
	                                              const std::array<bool, 4>& markTriangleAsInside
	                                              = {false, false, false, false},
	                                              const int subdivisions = 0)
	{
		using S = typename BezierTriangleFromTetrahedron<T>::type;

		for (int side = 1; side <= 4; side++)
		{
			if (!extractSide[static_cast<size_t>(side - 1)])
			{
				continue;
			}
			const auto& bezierTriangle
			    = tracer::extractBezierTriangleFromTetrahedron<T, S>(tetrahedron, side);
			addObjectBezierTriangle(
			    sceneObject, bezierTriangle, markTriangleAsInside[static_cast<size_t>(side - 1)]);

			// std::vector<S> subTriangles;
			// subTriangles.reserve(
			//     static_cast<size_t>(std::pow(4.0L, static_cast<long double>(subdivisions))));
			// subTriangles.push_back(bezierTriangle);

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
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.06f,
			// ColorIdx::t_white);
			// 		// 	}
			// 		// 	else
			// 		// 	{
			// 		// 		addObjectSphere(s.center.controlPoints[li], 0.03f,
			// ColorIdx::t_yellow);
			// 		// 	}
			// 		// }
			// 	}
			// }

			// for (const auto& subTriangle : subTriangles)
			// {
			// addObjectBezierTriangle(
			//     sceneObject, subTriangle, markTriangleAsInside[static_cast<size_t>(side - 1)]);
			// }
			// visualizeTetrahedron2(raytracingScene, tetrahedron2);
		}
	}

	std::vector<std::shared_ptr<RaytracingWorldObject<Sphere>>>&
	getWorldObjectSpheres(SceneObject& sceneObject)
	{
		return sceneObject.spheres;
	}

	template <typename S>
	std::vector<std::shared_ptr<RaytracingWorldObject<S>>>&
	getWorldObjectBezierTriangles(SceneObject& sceneObject)
	{
		return getTriangleListForSceneObject<S>(sceneObject);
	}

	std::vector<std::shared_ptr<RaytracingWorldObject<RectangularBezierSurface2x2>>>&
	getWorldObjectRectangularBezierSurfaces2x2(SceneObject& sceneObject)
	{
		return sceneObject.rectangularBezierSurfaces2x2;
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
			             slicingPlanesBufferHandle,
			             slicingPlanesBufferAllocation);
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
			             spheresBufferHandle,
			             spheresBufferAllocation);
		}

		// if (tetrahedrons2.size() > 0)
		// {
		// 	createBuffer(physicalDevice,
		// 	             logicalDevice,
		// 	             vmaAllocator,
		// 	             deletionQueueForAccelerationStructure,
		// 	             tetrahedrons2.size() * sizeof(Tetrahedron2),
		// 	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		// 	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 	             memoryAllocateFlagsInfo,
		// 	             tetrahedronsBufferHandle,
		// 	             tetrahedronsBufferAllocation);
		// }

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
			             bezierTriangles2BufferHandle,
			             bezierTriangles2BufferAllocation);
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
			             bezierTriangles3BufferHandle,
			             bezierTriangles3BuffersAllocation);
		}

		if (bezierTriangles4.size() > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             bezierTriangles4.size() * sizeof(BezierTriangle4),
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             bezierTriangles4BufferHandle,
			             bezierTriangles4BuffersAllocation);
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
			             rectangularBezierSurfaces2x2BufferHandle,
			             rectangularBezierSurfaces2x2BufferAllocation);
		}
	}

	SceneObject& createSceneObject(const glm::vec3 pos = glm::vec3(0),
	                               const glm::quat rotation = glm::quat(),
	                               const glm::vec3 scale = glm::vec3(1))
	{
		return createNamedSceneObject("", pos, rotation, scale);
	}

	SceneObject& createNamedSceneObject(const std::string& name = "",
	                                    const glm::vec3 pos = glm::vec3(0),
	                                    const glm::quat rotation = glm::quat(),
	                                    const glm::vec3 scale = glm::vec3(1))
	{
		Transform identityTransform(pos, rotation, scale);

		const size_t instanceCustomIndex = sceneObjects.size();

		// only the first 24 bits are used for the instance custom index in vulkan
		// therefore this ensures that the index is not larger than 24 bits
		[[maybe_unused]] constexpr auto indexLimit24Bit = (1u << 24) - 1;
		assert(instanceCustomIndex <= indexLimit24Bit
		       && "instanceCustomIndex exceeds 24 bit limit");

		if (!name.empty() && getSceneObject(name))
		{
			throw std::runtime_error("createSceneObject - Scene object with name already exists");
		}

		const auto instanceIndex = static_cast<uint32_t>(instanceCustomIndex);
		const auto transformMatrix = identityTransform.getTransformMatrix();
		sceneObjects.push_back(SceneObject(name, transformMatrix, instanceIndex));

		auto& obj = sceneObjects.back();
		objectNameToSceneObjectMap.emplace(name, obj);
		return obj;
	}

	void recreateAccelerationStructures(RaytracingInfo& raytracingInfo, const bool fullRebuild)
	{
		// update the lists and copy then to the GPU buffer
		spheresList.clear();
		bezierTriangles2List.clear();
		bezierTriangles3List.clear();
		bezierTriangles4List.clear();
		rectangularSurfaces2x2List.clear();

		// We dont want to recreate the BLAS's but only update the related transform matrix
		// inside the TLAS that links to the particular BLAS
		if (fullRebuild)
		{
			// WARNING:	USE A POOL OF BUFFERS INSTEAD!!
			// TODO: Reuse created buffers instead of throwing them away and recreating them,
			// this e.g. instead of recreating 1000 buffers for each sphere, just reuse the
			// buffer and can save a lot of time, assign  them new if the amount of spheres
			// changes, only create new buffers for the new spheres
			deletionQueueForAccelerationStructure.flush();

			gpuObjectsBufferAllocation = VK_NULL_HANDLE;
			gpuObjectsBufferHandle = VK_NULL_HANDLE;

			// TODO: do we need to recreate the slicing planes buffer?
			slicingPlanesBufferHandle = VK_NULL_HANDLE;
			slicingPlanesBufferAllocation = VK_NULL_HANDLE;
			createSlicingPlanesBuffer();

			// tetrahedronsBufferHandle = VK_NULL_HANDLE;
			// tetrahedronsBufferAllocation = VK_NULL_HANDLE;

			spheresBufferHandle = VK_NULL_HANDLE;
			spheresBufferAllocation = VK_NULL_HANDLE;

			rectangularBezierSurfaces2x2BufferHandle = VK_NULL_HANDLE;
			rectangularBezierSurfaces2x2BufferAllocation = VK_NULL_HANDLE;

			bezierTriangles2BufferHandle = VK_NULL_HANDLE;
			bezierTriangles2BufferAllocation = VK_NULL_HANDLE;

			bezierTriangles3BufferHandle = VK_NULL_HANDLE;
			bezierTriangles3BuffersAllocation = VK_NULL_HANDLE;

			bezierTriangles4BufferHandle = VK_NULL_HANDLE;
			bezierTriangles4BuffersAllocation = VK_NULL_HANDLE;

			spheresList.clear();
			bezierTriangles2List.clear();
			bezierTriangles3List.clear();
			bezierTriangles4List.clear();
			rectangularSurfaces2x2List.clear();

			gpuObjects.clear();
			blasInstances.clear();

			// the objects that are rendered using ray tracing (with an intersection shader)
			RaytracingObjectAABBBuffers aabbBuffers{};

			for (const auto& sceneObject : sceneObjects)
			{
				vkQueueWaitIdle(raytracingInfo.graphicsQueueHandle);
				aabbBuffers.clearAllHandles();

				copyAABBsToBuffer(aabbBuffers, sceneObject);
				addSceneObjectToGpuObjects(sceneObject);

				const std::vector<BLASBuildData> buildData
				    = createBLASBuildDataForSceneObject(aabbBuffers, sceneObject);
				auto blasBuildData = BLASSceneObjectBuildData{
				    .blasData = buildData,
				    .transformMatrix = sceneObject.transformMatrix,
				    .instanceCustomIndex = sceneObject.instanceCustomIndex,
				};

				vkQueueWaitIdle(raytracingInfo.graphicsQueueHandle);
				auto blasInstance = buildBLASInstancesFromBuildDataList(
				    blasBuildData,
				    raytracingInfo.commandBufferBuildTopAndBottomLevel,
				    raytracingInfo.graphicsQueueHandle,
				    raytracingInfo.accelerationStructureBuildFence);

				blasInstances.push_back(blasInstance);
			}

			createBuffers();
			copyGPUObjectsToBuffers();
			copySlicingPlaneToBuffers();
			copyGPUInstancesToBuffer(fullRebuild);

			blasInstancesCount = blasInstances.size();

			// TODO: create a dedicated struct that holds all the information for the
			// acceleration structure that is actually needed
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
			vkQueueWaitIdle(raytracingInfo.graphicsQueueHandle);
			for (auto& sceneObject : sceneObjects)
			{
				addSceneObjectToGpuObjects(sceneObject);
			}
			copyGPUObjectsToBuffers();

			copySlicingPlaneToBuffers();

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
	void copyGPUObjectsToBuffers()
	{
		if (spheresList.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 spheresBufferAllocation,
			                 spheresList.data(),
			                 sizeof(Sphere) * spheresList.size());
		}

		// if (tetrahedrons2.size() > 0)
		// {
		// 	std::vector<Tetrahedron2> tetrahedrons2List;
		// 	for (size_t i = 0; i < tetrahedrons2.size(); i++)
		// 	{
		// 		auto& obj = tetrahedrons2[i];
		// 		tetrahedrons2List.push_back(obj.getGeometry().getData());
		// 	}
		// 	copyDataToBuffer(vmaAllocator,
		// 	                 aabbBuffers.tetrahedronsBufferAllocation,
		// 	                 tetrahedrons2List.data(),
		// 	                 sizeof(Tetrahedron2) * tetrahedrons2List.size());
		// }

		if (bezierTriangles2List.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 bezierTriangles2BufferAllocation,
			                 bezierTriangles2List.data(),
			                 sizeof(BezierTriangle2) * bezierTriangles2List.size());
		}

		if (bezierTriangles3List.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 bezierTriangles3BuffersAllocation,
			                 bezierTriangles3List.data(),
			                 sizeof(BezierTriangle3) * bezierTriangles3List.size());
		}

		if (bezierTriangles4List.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 bezierTriangles4BuffersAllocation,
			                 bezierTriangles4List.data(),
			                 sizeof(BezierTriangle4) * bezierTriangles4List.size());
		}

		if (rectangularSurfaces2x2List.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 rectangularBezierSurfaces2x2BufferAllocation,
			                 rectangularSurfaces2x2List.data(),
			                 sizeof(RectangularBezierSurface2x2)
			                     * rectangularSurfaces2x2List.size());
		}
	}

	void copySlicingPlaneToBuffers()
	{
		if (slicingPlanes.size() > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 slicingPlanesBufferAllocation,
			                 slicingPlanes.data(),
			                 sizeof(SlicingPlane) * slicingPlanes.size());
		}
	}

	std::optional<SceneObject*> getSceneObject(const std::string& name)
	{
		auto sceneObjectItr = objectNameToSceneObjectMap.find(name);
		if (sceneObjectItr != objectNameToSceneObjectMap.end())
		{
			return &sceneObjectItr->second;
		}
		else
		{
			return std::nullopt;
		}
	}

  private:
	template <typename T>
	void addObjectsToBLASBuildDataListAndGPUObjectsList(
	    std::vector<BLASBuildData>& blasBuildDataList,
	    const std::vector<std::shared_ptr<RaytracingWorldObject<T>>>& sceneObjectObjects,
	    std::vector<VkBuffer>& aabbBufferHandles)
	{
		// for each object in the SceneObject, create an entry in the gpuObjects vector to
		// reference later inside the shader also create the BLAS build data for the entry and
		// add it to the list
		for (size_t i = 0; i < sceneObjectObjects.size(); i++)
		{
			auto buildData = createBottomLevelAccelerationStructureBuildDataAABB(
			    logicalDevice, aabbBufferHandles[i]);

			auto objectType = sceneObjectObjects[i]->getType();
			gpuObjects.push_back(GPUInstance(objectType, i));
			blasBuildDataList.push_back(buildData);
		}
	}

	// Extracts the data of the objects from the std::shared_ptr<RaytracingWorldObject<T>> vector
	// into a vector and creates and copies the AABB to the corresponding buffer
	template <typename T>
	void extractAndCopyAABBsToBuffer(
	    const std::vector<std::shared_ptr<RaytracingWorldObject<T>>>& objects,
	    std::vector<VkBuffer>& aabbBufferHandles,
	    std::vector<VmaAllocation>& aabbBufferAllocations,
	    std::vector<AABB>& objectsAABBs)
	{
		if (objects.size() > 0)
		{
			aabbBufferHandles.resize(objects.size());
			aabbBufferAllocations.resize(objects.size());
			for (size_t i = 0; i < objects.size(); i++)
			{
				auto& obj = objects[i];
				objectsAABBs.push_back(obj->getGeometry().getAABB());

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
				const auto aabbPositions = obj->getGeometry().getAABB().getAabbPositions();
				copyDataToBuffer(
				    vmaAllocator, aabbBufferAllocations[i], &aabbPositions, sizeof(AABB));
			}
		}
	}

	void addSceneObjectToGpuObjects(const SceneObject& sceneObject)
	{
		for (size_t i = 0; i < sceneObject.spheres.size(); i++)
		{
			auto& obj = sceneObject.spheres[i];
			spheresList.push_back(obj->getGeometry().getData());
		}

		for (size_t i = 0; i < sceneObject.bezierTriangles2.size(); i++)
		{
			auto& obj = sceneObject.bezierTriangles2[i];
			bezierTriangles2List.push_back(obj->getGeometry().getData());
		}
		for (size_t i = 0; i < sceneObject.bezierTriangles3.size(); i++)
		{
			auto& obj = sceneObject.bezierTriangles3[i];
			bezierTriangles3List.push_back(obj->getGeometry().getData());
		}
		for (size_t i = 0; i < sceneObject.bezierTriangles4.size(); i++)
		{
			auto& obj = sceneObject.bezierTriangles4[i];
			bezierTriangles4List.push_back(obj->getGeometry().getData());
		}

		for (size_t i = 0; i < sceneObject.rectangularBezierSurfaces2x2.size(); i++)
		{
			auto& obj = sceneObject.rectangularBezierSurfaces2x2[i];
			rectangularSurfaces2x2List.push_back(obj->getGeometry().getData());
		}
	}

	void copyAABBsToBuffer(RaytracingObjectAABBBuffers& aabbBuffers, const SceneObject& sceneObject)
	{
		// extract and copy AABBs to buffers
		extractAndCopyAABBsToBuffer(sceneObject.spheres,
		                            aabbBuffers.spheresAABBBufferHandles,
		                            aabbBuffers.spheresAABBBufferAllocations,
		                            aabbsSpheres);

		// std::vector<Tetrahedron2> tetrahedrons2List
		//     = extractAndCopyAABBsToBuffer(tetrahedrons2,
		//                                   aabbBuffers.tetrahedronsAABBBufferHandles,
		//                                   aabbBuffers.tetrahedronsAABBBufferAllocations,
		//                                   aabbsTetrahedrons);

		extractAndCopyAABBsToBuffer(sceneObject.bezierTriangles2,
		                            aabbBuffers.bezierTriangles2AABBBufferHandles,
		                            aabbBuffers.bezierTriangles2AABBBufferAllocations,
		                            aabbsBezierTriangles2);

		extractAndCopyAABBsToBuffer(sceneObject.bezierTriangles3,
		                            aabbBuffers.bezierTriangles3AABBBufferHandles,
		                            aabbBuffers.bezierTriangles3AABBBufferAllocations,
		                            aabbsBezierTriangles3);

		extractAndCopyAABBsToBuffer(sceneObject.bezierTriangles4,
		                            aabbBuffers.bezierTriangles4AABBBufferHandles,
		                            aabbBuffers.bezierTriangles4AABBBufferAllocations,
		                            aabbsBezierTriangles4);

		extractAndCopyAABBsToBuffer(
		    sceneObject.rectangularBezierSurfaces2x2,
		    aabbBuffers.rectangularBezierSurfacesAABB2x2BufferHandles,
		    aabbBuffers.rectangularBezierSurfacesAABB2x2AABBBufferAllocations,
		    aabbsRectangularSurfaces2x2);
	}

	[[nodiscard]] const std::vector<BLASBuildData>
	createBLASBuildDataForSceneObject(RaytracingObjectAABBBuffers aabbBuffers,
	                                  const SceneObject& sceneObject)
	{
		std::vector<BLASBuildData> blasBuildDataList;

		// for each object add the extracted object data to the gpuObjects vector and create the
		// buildData in blasBuildDataList
		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList, sceneObject.spheres, aabbBuffers.spheresAABBBufferHandles);

		// addObjectsToBLASBuildDataListAndGPUObjectsList(blasBuildDataList,
		//                                                tetrahedrons2,
		//                                                ObjectType::t_Tetrahedron2,
		//                                                tetrahedrons2List,
		//                                                aabbBuffers.tetrahedronsAABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    sceneObject.bezierTriangles2,
		    aabbBuffers.bezierTriangles2AABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    sceneObject.bezierTriangles3,
		    aabbBuffers.bezierTriangles3AABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    sceneObject.bezierTriangles4,
		    aabbBuffers.bezierTriangles4AABBBufferHandles);

		addObjectsToBLASBuildDataListAndGPUObjectsList(
		    blasBuildDataList,
		    sceneObject.rectangularBezierSurfaces2x2,
		    aabbBuffers.rectangularBezierSurfacesAABB2x2BufferHandles);

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

		return blasBuildDataList;
	}

	void copyGPUInstancesToBuffer([[maybe_unused]] const bool isFullRebuild)
	{
		size_t instancesCount = spheresList.size() /*+ tetrahedrons2List.size()*/
		                        + bezierTriangles2List.size() + bezierTriangles3List.size()
		                        + bezierTriangles4List.size() + rectangularSurfaces2x2List.size();

		// NOTE: chaning the amount of objects only allowed when we do a full rebuild
		// also this function only needs to be called when doing a full rebuild
		assert(!isFullRebuild
		       || instancesCount == gpuObjects.size()
		              && "Amount of objects in the scene does not match the "
		                 "previous amount of objects in the GPU buffer. Changing the object "
		                 "count is "
		                 "not yetsceneObject. "
		                 "supported!");

		// TODO: the size of the elements does not change as of now, we need to  implement
		// recreation/resizing of the buffer otherwise
		assert(gpuObjectsBufferHandle == VK_NULL_HANDLE && "GPU buffer already created");

		if (instancesCount > 0)
		{
			createBuffer(physicalDevice,
			             logicalDevice,
			             vmaAllocator,
			             deletionQueueForAccelerationStructure,
			             sizeof(GPUInstance) * instancesCount,
			             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			             memoryAllocateFlagsInfo,
			             gpuObjectsBufferHandle,
			             gpuObjectsBufferAllocation);
		}

		// update buffer data
		if (instancesCount > 0)
		{
			copyDataToBuffer(vmaAllocator,
			                 gpuObjectsBufferAllocation,
			                 gpuObjects.data(),
			                 sizeof(GPUInstance) * instancesCount);
		}
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

	[[nodiscard]] const VkAccelerationStructureInstanceKHR
	buildBLASInstancesFromBuildDataList(const BLASSceneObjectBuildData& BLASBuildData,
	                                    const VkCommandBuffer bottomLevelCommandBuffer,
	                                    const VkQueue graphicsQueue,
	                                    const VkFence accelerationStructureBuildFence)
	{
		//  build the BLAS on GPI
		VkAccelerationStructureKHR bottomLevelAccelerationStructure
		    = buildBottomLevelAccelerationStructure(physicalDevice,
		                                            logicalDevice,
		                                            vmaAllocator,
		                                            deletionQueueForAccelerationStructure,
		                                            bottomLevelCommandBuffer,
		                                            graphicsQueue,
		                                            BLASBuildData,
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
		    .transform = BLASBuildData.transformMatrix,
		    // TODO: maybe add a method that makes sure objectType does not exceed 24 bits
		    // see:
		    // https://registry.khronos.org/vulkan/specs/latest/man/html/InstanceCustomIndexKHR.html
		    // only grab 24 bits
		    .instanceCustomIndex
		    = static_cast<uint32_t>(BLASBuildData.instanceCustomIndex) & 0xFFFFFF,
		    .mask = 0xFF,
		    .instanceShaderBindingTableRecordOffset = 0,
		    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		    .accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress,
		};
		return bottomLevelGeometryInstance;
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
	std::vector<SlicingPlane> slicingPlanes;
	std::vector<GPUInstance> gpuObjects;

	std::vector<tracer::AABB> aabbsSpheres;
	std::vector<tracer::AABB> aabbsTetrahedrons;
	std::vector<tracer::AABB> aabbsBezierTriangles2;
	std::vector<tracer::AABB> aabbsBezierTriangles3;
	std::vector<tracer::AABB> aabbsBezierTriangles4;
	std::vector<tracer::AABB> aabbsRectangularSurfaces2x2;

	// stores the data of all objects added to the scene
	// each SceneObject references to these objects
	// std::vector<rt::std::shared_ptr<RaytracingWorldObject<T>>etrahedron2>> tetrahedrons2;
	std::vector<std::shared_ptr<RaytracingWorldObject<RectangularBezierSurface2x2>>>
	    rectangularBezierSurfaces2x2;
	std::vector<std::shared_ptr<RaytracingWorldObject<Sphere>>> spheres;
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle2>>> bezierTriangles2;
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle3>>> bezierTriangles3;
	std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle4>>> bezierTriangles4;

	// TODO: does this really need to be a member variable? maybe export into some struct that
	// holds the data temporarily and gets passed around temporarily used to collect all
	// internal object data to write into a buffer for the GPU
	std::vector<Sphere> spheresList;
	std::vector<BezierTriangle2> bezierTriangles2List;
	std::vector<BezierTriangle3> bezierTriangles3List;
	std::vector<BezierTriangle4> bezierTriangles4List;
	std::vector<RectangularBezierSurface2x2> rectangularSurfaces2x2List;

	VmaAllocation gpuObjectsBufferAllocation = VK_NULL_HANDLE;
	VmaAllocation slicingPlanesBufferAllocation = VK_NULL_HANDLE;

	// VmaAllocation tetrahedronsBufferAllocation = VK_NULL_HANDLE;
	VmaAllocation bezierTriangles2BufferAllocation = VK_NULL_HANDLE;
	VmaAllocation bezierTriangles3BuffersAllocation = VK_NULL_HANDLE;
	VmaAllocation bezierTriangles4BuffersAllocation = VK_NULL_HANDLE;
	VmaAllocation spheresBufferAllocation = VK_NULL_HANDLE;
	VmaAllocation rectangularBezierSurfaces2x2BufferAllocation = VK_NULL_HANDLE;

	// NOTE: not used in renderer
	// std::vector<MeshObject> meshObjects;

	// basically gives the objects we add as blasInstances names so we can reference them
	// later e.g. "light" will reference the blas that represents the light
	std::map<std::string, SceneObject&> objectNameToSceneObjectMap;

	// keeps track of how many SceneObjects are currently added to this scene
	uint32_t sceneObjectCounter = 0;
	std::vector<SceneObject> sceneObjects;
	std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

	size_t blasInstancesCount = 0;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VmaAllocator vmaAllocator;
	DeletionQueue deletionQueueForAccelerationStructure;

	int currentSceneNr = INITIAL_SCENE;
};

} // namespace rt
} // namespace tracer
