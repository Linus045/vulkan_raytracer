#pragma once

#include <vector>

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
	RaytracingScene(const VkPhysicalDevice& physicalDevice,
	                const VkDevice logicalDevice,
	                DeletionQueue& deletionQueue)
	    : physicalDevice(physicalDevice), logicalDevice(logicalDevice),
	      deletionQueue(deletionQueue) {};

	~RaytracingScene() = default;

	RaytracingScene(const RaytracingScene&) = delete;
	RaytracingScene& operator=(const RaytracingScene&) = delete;

	RaytracingScene(RaytracingScene&&) noexcept = delete;
	RaytracingScene& operator=(RaytracingScene&&) noexcept = delete;

	const RaytracingObjectBuffers& getObjectBuffers() const
	{
		return objectBuffers;
	}

	void addWorldObject(const MeshObject& meshObject)
	{
		meshObjects.push_back(meshObject);
	}

	void addWorldObject(const Sphere& sphere, const glm::vec3 position = glm::vec3(0))
	{
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

	void createAccelerationStructures(RaytracingInfo& raytracingInfo)
	{
		auto buildData = createBLASBuildDataForScene();
		blasInstances = buildBLASInstancesFromBuildDataList(buildData, raytracingInfo);
		createTLAS(false, raytracingInfo);
	}

	void recreateAccelerationStructures(RaytracingInfo& raytracingInfo)
	{
		freeBottomLevelAndTopLevelAccelerationStructure(raytracingInfo);
		// We dont want to recreate the BLAS's but only update the related transform matrix inside
		// the TLAS that links to the particular BLAS

		// createBLASInstances();
		createTLAS(true, raytracingInfo);
	}

	void
	freeBottomLevelAndTopLevelAccelerationStructure([[maybe_unused]] RaytracingInfo& raytracingInfo)
	{
		// TODO: the old acceleration structures need to be deleted otherwise we will create a bunch
		// of them!
	}

	void animateSphere(const glm::vec3& position)
	{
		VkDeviceMemory bufferMemory = getObjectBuffers().spheresDeviceMemoryHandles[0];

		void* memoryBuffer;
		VkResult result
		    = vkMapMemory(logicalDevice, bufferMemory, 0, sizeof(Sphere), 0, &memoryBuffer);

		auto sphere = &spheres[0].getGeometry().getData();
		sphere->center = position;
		memcpy(memoryBuffer, sphere, sizeof(Sphere));

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error("createObjectsBuffer - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, bufferMemory);

		blasInstances[0].transform = {
			.matrix = {
			{1, 0, 0, position.x},
			{0, 1, 0, position.y},
			{0, 0, 1, position.z},
			},
		};
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

		for (auto&& obj : spheres)
		{
			aabbsSpheres.push_back(obj.getGeometry().getAABB());
			spheresList.push_back(obj.getGeometry().getData());
		}

		for (auto&& obj : tetrahedrons2)
		{
			aabbsTetrahedrons.push_back(obj.getGeometry().getAABB());
			tetrahedrons2List.push_back(obj.getGeometry().getData());
		}

		for (auto&& obj : rectangularBezierSurfaces2x2)
		{
			aabbsRectangularSurfaces2x2.push_back(obj.getGeometry().getAABB());
			rectangularSurfaces2x2List.push_back(obj.getGeometry().getData());
		}

		// TODO: create TLAS for each object in the scene

		std::vector<BLASBuildData> blasBuildDataList;
		if (spheresList.size() > 0)
		{
			// create vector to hold all BLAS instances
			objectBuffers.spheresBufferHandle.resize(spheresList.size());
			objectBuffers.spheresDeviceMemoryHandles.resize(spheresList.size());
			objectBuffers.spheresAABBBufferHandle.resize(spheresList.size());
			for (size_t i = 0; i < spheresList.size(); i++)
			{
				auto aabb = aabbsSpheres[i];
				auto sphere = spheresList[i];
				auto buildData = createBottomLevelAccelerationStructureBuildDataForObject<Sphere>(
				    physicalDevice,
				    logicalDevice,
				    deletionQueue,
				    sphere,
				    ObjectType::t_Sphere,
				    aabb,
				    spheres[i].getTransformMatrix(),
				    objectBuffers.spheresBufferHandle[i],
				    objectBuffers.spheresDeviceMemoryHandles[i],
				    objectBuffers.spheresAABBBufferHandle[i]);

				blasBuildDataList.push_back(buildData);
			}
		}

		if (tetrahedrons2List.size() > 0)
		{
			objectBuffers.tetrahedronsBufferHandle.resize(tetrahedrons2List.size());
			objectBuffers.tetrahedronsDeviceMemoryHandles.resize(tetrahedrons2List.size());
			objectBuffers.tetrahedronsAABBBufferHandle.resize(tetrahedrons2List.size());
			for (size_t i = 0; i < tetrahedrons2List.size(); i++)
			{
				auto aabb = aabbsTetrahedrons[i];
				auto tetrahedron2 = tetrahedrons2List[i];
				auto buildData
				    = createBottomLevelAccelerationStructureBuildDataForObject<Tetrahedron2>(
				        physicalDevice,
				        logicalDevice,
				        deletionQueue,
				        tetrahedron2,
				        ObjectType::t_Tetrahedron2,
				        aabb,
				        tetrahedrons2[i].getTransformMatrix(),
				        objectBuffers.tetrahedronsBufferHandle[i],
				        objectBuffers.tetrahedronsDeviceMemoryHandles[i],
				        objectBuffers.tetrahedronsAABBBufferHandle[i]);
				blasBuildDataList.push_back(buildData);
			}
		}

		if (rectangularSurfaces2x2List.size() > 0)
		{
			for (size_t i = 0; i < rectangularSurfaces2x2List.size(); i++)
			{
				objectBuffers.rectangularBezierSurfaces2x2BufferHandle.resize(
				    rectangularSurfaces2x2List.size());
				objectBuffers.rectangularBezierSurfaces2x2DeviceMemoryHandles.resize(
				    rectangularSurfaces2x2List.size());
				objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandle.resize(
				    rectangularSurfaces2x2List.size());
				auto aabb = aabbsTetrahedrons[i];
				auto rectangularBezierSurface2x2 = rectangularSurfaces2x2List[i];
				auto buildData = createBottomLevelAccelerationStructureBuildDataForObject<
				    RectangularBezierSurface2x2>(
				    physicalDevice,
				    logicalDevice,
				    deletionQueue,
				    rectangularBezierSurface2x2,
				    ObjectType::t_RectangularBezierSurface2x2,
				    aabb,
				    rectangularBezierSurfaces2x2[i].getTransformMatrix(),
				    objectBuffers.rectangularBezierSurfaces2x2BufferHandle[i],
				    objectBuffers.rectangularBezierSurfaces2x2DeviceMemoryHandles[i],
				    objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandle[i]);
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
	                                    RaytracingInfo& raytracingInfo)
	{
		std::vector<VkAccelerationStructureInstanceKHR> instances;
		for (auto& buildData : BLASBuildDataList)
		{
			// build the BLAS on GPI
			VkAccelerationStructureKHR bottomLevelAccelerationStructure
			    = buildBottomLevelAccelerationStructure(
			        physicalDevice, logicalDevice, deletionQueue, raytracingInfo, buildData);

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
			    .instanceCustomIndex = static_cast<uint32_t>(buildData.objectType) & 0xFFFFFF,
			    .mask = 0xFF,
			    .instanceShaderBindingTableRecordOffset = 0,
			    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			    .accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress,
			};

			instances.push_back(std::move(bottomLevelGeometryInstance));
		}
		return instances;
	}

	void createTLAS(const bool onlyUpdate, RaytracingInfo& raytracingInfo)
	{
		if (blasInstances.size() > 0)
		{
			createAndBuildTopLevelAccelerationStructure(blasInstances,
			                                            deletionQueue,
			                                            logicalDevice,
			                                            physicalDevice,
			                                            raytracingInfo,
			                                            onlyUpdate);
			// =========================================================================
			// Uniform Buffer
			if (!onlyUpdate)
			{
				createBuffer(physicalDevice,
				             logicalDevice,
				             deletionQueue,
				             sizeof(UniformStructure),
				             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				             memoryAllocateFlagsInfo,
				             raytracingInfo.uniformBufferHandle,
				             raytracingInfo.uniformDeviceMemoryHandle);

				void* hostUniformMemoryBuffer;
				VkResult result = vkMapMemory(logicalDevice,
				                              raytracingInfo.uniformDeviceMemoryHandle,
				                              0,
				                              sizeof(UniformStructure),
				                              0,
				                              &hostUniformMemoryBuffer);
				if (result != VK_SUCCESS)
				{
					throw new std::runtime_error("initRayTraci - vkMapMemory");
				}
				memcpy(hostUniformMemoryBuffer,
				       &raytracingInfo.uniformStructure,
				       sizeof(UniformStructure));

				vkUnmapMemory(logicalDevice, raytracingInfo.uniformDeviceMemoryHandle);
			}
		}
	}

  private:
	std::vector<RaytracingWorldObject<Sphere>> spheres;
	std::vector<RaytracingWorldObject<Tetrahedron2>> tetrahedrons2;
	std::vector<RaytracingWorldObject<RectangularBezierSurface2x2>> rectangularBezierSurfaces2x2;
	std::vector<MeshObject> meshObjects;

	std::vector<VkAccelerationStructureInstanceKHR> blasInstances;
	std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	DeletionQueue& deletionQueue;

	// the objects that are rendered using ray tracing (with an intersection shader)
	RaytracingObjectBuffers objectBuffers;
};

} // namespace rt
} // namespace ltracer
