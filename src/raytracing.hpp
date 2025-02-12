#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float3.hpp>

#include "src/aabb.hpp"
#include "src/blas.hpp"
#include "src/camera.hpp"
#include "src/common_types.h"
#include "src/deletion_queue.hpp"
#include "src/model.hpp"
#include "src/shader_module.hpp"
#include "src/tlas.hpp"
#include "src/types.hpp"
#include "src/visualizations.hpp"
#include "src/device_procedures.hpp"
#include "src/vk_utils.hpp"

#include "tiny_obj_loader.h"

namespace ltracer
{
namespace rt
{

// TODO: utilize https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html
// to get better error messages

// TODO: figure out a better way to handle this
static VkQueue graphicsQueueHandle = VK_NULL_HANDLE;

/**
 * @brief Updates the camera position, forward, up and right vectors in the uniform structure.
 *
 * @param raytracingInfo The raytracingInfo object that should be updated
 * @param position camera position
 * @param forward camera forward
 * @param up camera up
 * @param right camera right
 */
inline void updateUniformStructure(RaytracingInfo& raytracingInfo,
                                   const glm::vec3& position,
                                   const glm::vec3& forward,
                                   const glm::vec3& up,
                                   const glm::vec3& right)
{
	raytracingInfo.uniformStructure.cameraPosition[0] = position[0];
	raytracingInfo.uniformStructure.cameraPosition[1] = position[1];
	raytracingInfo.uniformStructure.cameraPosition[2] = position[2];

	raytracingInfo.uniformStructure.cameraForward[0] = forward[0];
	raytracingInfo.uniformStructure.cameraForward[1] = forward[1];
	raytracingInfo.uniformStructure.cameraForward[2] = forward[2];

	raytracingInfo.uniformStructure.cameraUp[0] = up[0];
	raytracingInfo.uniformStructure.cameraUp[1] = up[1];
	raytracingInfo.uniformStructure.cameraUp[2] = up[2];

	raytracingInfo.uniformStructure.cameraRight[0] = right[0];
	raytracingInfo.uniformStructure.cameraRight[1] = right[1];
	raytracingInfo.uniformStructure.cameraRight[2] = right[2];
}

/**
 * @brief resets the frameCount forcing the ray tracing to start anew
 *
 * @param raytracingInfo The raytracing info object that should be reset
 */
inline void resetFrameCount(RaytracingInfo& raytracingInfo)
{
	raytracingInfo.uniformStructure.frameCount = 0;
}

/**
 * @brief Constructs the ray tracing pipeline features needed to create the logical device
 *
 * @return Physical device ray tracing pipeline features
 */
inline VkPhysicalDeviceRayTracingPipelineFeaturesKHR getRaytracingPipelineFeatures()
{
	// =========================================================================
	// Physical Device Features

	VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	    .pNext = NULL,
	    .bufferDeviceAddress = VK_TRUE,
	    .bufferDeviceAddressCaptureReplay = VK_FALSE,
	    .bufferDeviceAddressMultiDevice = VK_FALSE,
	};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
	    .pNext = &physicalDeviceBufferDeviceAddressFeatures,
	    .accelerationStructure = VK_TRUE,
	    .accelerationStructureCaptureReplay = VK_FALSE,
	    .accelerationStructureIndirectBuild = VK_FALSE,
	    .accelerationStructureHostCommands = VK_FALSE,
	    .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE,
	};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
	    .pNext = &physicalDeviceAccelerationStructureFeatures,
	    .rayTracingPipeline = VK_TRUE,
	    .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
	    .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
	    .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
	    .rayTraversalPrimitiveCulling = VK_FALSE,
	};

	// NOTE: is hardcoded into createLogicalDevice
	// VkPhysicalDeviceFeatures deviceFeatures{
	//    .geometryShader = VK_TRUE,
	// };

	return physicalDeviceRayTracingPipelineFeatures;
}

/**
 * @brief create a buffer for a vector of objects, used to create buffers for the spheres,
 * tetrahedrons and other geometry objects that will be converted to AABBs
 *
 * @tparam T The geometry type, e.g. Sphere, Tetrahedron, etc..
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue The buffer will be added to the deletion queue
 * @param objects List of objects of Type <T> that will be copied into the buffer
 * @return VkBuffer The buffer handle
 */
template <typename T>
inline VkBuffer createObjectsBuffer(VkPhysicalDevice physicalDevice,
                                    VkDevice logicalDevice,
                                    DeletionQueue& deletionQueue,
                                    const std::vector<T>& objects)
{
	VkBuffer bufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(T) * objects.size(),
	             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             bufferHandle,
	             deviceMemoryHandle);

	void* memoryBuffer;
	VkResult result = vkMapMemory(
	    logicalDevice, deviceMemoryHandle, 0, sizeof(T) * objects.size(), 0, &memoryBuffer);

	memcpy(memoryBuffer, objects.data(), sizeof(T) * objects.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("createObjectsBuffer - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, deviceMemoryHandle);

	return bufferHandle;
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
inline VkBuffer createAABBBuffer(VkPhysicalDevice physicalDevice,
                                 VkDevice logicalDevice,
                                 DeletionQueue& deletionQueue,
                                 const std::vector<AABB>& aabbs)
{
	std::vector<VkAabbPositionsKHR> aabbPositions;
	for (auto&& aabb : aabbs)
		aabbPositions.push_back(aabb.getAabb());

	// we need an alignment of 8 bytes, VkAabbPositionsKHR is 24 bytes
	uint32_t blockSize = sizeof(VkAabbPositionsKHR);

	VkBuffer bufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory aabbDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             blockSize * aabbPositions.size(),
	             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             bufferHandle,
	             aabbDeviceMemoryHandle);

	void* aabbPositionsMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              aabbDeviceMemoryHandle,
	                              0,
	                              blockSize * aabbPositions.size(),
	                              0,
	                              &aabbPositionsMemoryBuffer);

	memcpy(aabbPositionsMemoryBuffer, aabbPositions.data(), blockSize * aabbPositions.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error(
		    "createAndBuildBottomLevelAccelerationStructureAABB - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, aabbDeviceMemoryHandle);

	return bufferHandle;
}

/**
 * @brief Creates the command pool used for the ray tracing operations
 *
 * @param logicalDevice
 * @param deletionQueue the command pool will be added to the deletion queue
 * @param raytracingInfo the ray tracing info object
 * @param commandBufferPoolHandle The handle will be stored in
 */
inline VkCommandPool createCommandPool(VkDevice logicalDevice,
                                       DeletionQueue& deletionQueue,
                                       RaytracingInfo& raytracingInfo)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
	};

	VkCommandPool commandBufferPoolHandle = VK_NULL_HANDLE;
	VkResult result = vkCreateCommandPool(
	    logicalDevice, &commandPoolCreateInfo, NULL, &commandBufferPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateCommandPool failed");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyCommandPool(logicalDevice, commandBufferPoolHandle, nullptr); });

	return commandBufferPoolHandle;
}

/**
 * @brief Retrieves the raytracing properties from the physical device and stores them in the
 * physicalDeviceRayTracingPipelineProperties reference
 *
 * @param physicalDevice
 * @param physicalDeviceRayTracingPipelineProperties the properties object to be filled
 */
inline void requestRaytracingProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	physicalDeviceRayTracingPipelineProperties.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	physicalDeviceRayTracingPipelineProperties.pNext = NULL;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	    .pNext = &physicalDeviceRayTracingPipelineProperties,
	    .properties = physicalDeviceProperties,
	};

	// TODO: The VkPhysicalDeviceRayTracingPipelinePropertiesKHR object is passed in via pNext and
	// gets filled.
	// Because we are only interested in the VkPhysicalDeviceRayTracingPipelinePropertiesKHR
	// object it is fine that the VkPhysicalDeviceProperties2 object gets deleted when this
	// function scope ends.
	// However, maybe we wanna instead pass in the VkPhysicalDeviceProperties2
	// object as a reference as well and fill it, for now though its not needed
	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);
}

/**
 * @brief Creates the command buffer for the top and bottom level acceleration structure creation
 * and stores it inside the raytracingInfo object
 * (raytracingInfo.commandBufferBuildTopAndBottomLevel)
 *
 * @param logicalDevice
 * @param deletionQueue the command buffer is added to the deletion queue
 * @param raytracingInfo
 * @param commandBufferPoolHandle
 * @return VkCommandPool The command pool handle
 */
inline void createCommandBufferBuildTopAndBottomLevel(VkDevice logicalDevice,
                                                      DeletionQueue& deletionQueue,
                                                      RaytracingInfo& raytracingInfo,
                                                      VkCommandPool& commandBufferPoolHandle)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .pNext = NULL,
	    .commandPool = commandBufferPoolHandle,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = 1,
	};

	// TODO: make this function return the command buffer instead and store it inside the
	// raytracingInfo object outside of this function
	VkResult result = vkAllocateCommandBuffers(logicalDevice,
	                                           &commandBufferAllocateInfo,
	                                           &raytracingInfo.commandBufferBuildTopAndBottomLevel);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateCommandBuffers failed");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    {
		    vkFreeCommandBuffers(logicalDevice,
		                         commandBufferPoolHandle,
		                         1,
		                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
	    });
}

/**
 * @brief Creates the descriptor pool used for the raytracing, containing the various buffers used
 * in the shaders
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor pool is added to the deletion queue
 * @return VkDescriptorPool The descriptor pool handle
 */
inline VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, DeletionQueue& deletionQueue)
{
	VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
	    {.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 4},
	    {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = 2,
	    .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizeList.size()),
	    .pPoolSizes = descriptorPoolSizeList.data(),
	};

	VkResult result = vkCreateDescriptorPool(
	    logicalDevice, &descriptorPoolCreateInfo, NULL, &descriptorPoolHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorPool");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorPool(logicalDevice, descriptorPoolHandle, NULL); });

	return descriptorPoolHandle;
}

/**
 * @brief Creates the descriptor set layout for the ray tracing pipeline
 * The layout defines the various buffer types and at what stage they are used in the shader
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor set layout is added to the deletion queue
 * @return VkDescriptorSetLayout The descriptor set layout handle
 */
inline VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice,
                                                       DeletionQueue& deletionQueue)

{
	VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList = {
	    {
	        .binding = 0,
	        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 2,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 3,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 4,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	        .descriptorCount = 1,
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 5,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 6,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 7,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	    {
	        .binding = 8,
	        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	        .descriptorCount = 1,
	        .stageFlags
	        = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .pImmutableSamplers = NULL,
	    },
	};

	std::vector<VkDescriptorBindingFlags> bindingFlags = std::vector<VkDescriptorBindingFlags>(
	    descriptorSetLayoutBindingList.size(), VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlags = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
	    .pNext = NULL,
	    .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
	    .pBindingFlags = bindingFlags.data(),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	    .pNext = &layoutBindingFlags,
	    .flags = 0,
	    .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindingList.size()),
	    .pBindings = descriptorSetLayoutBindingList.data(),
	};

	VkResult result = vkCreateDescriptorSetLayout(
	    logicalDevice, &descriptorSetLayoutCreateInfo, NULL, &descriptorSetLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorSetLayout");
	}

	deletionQueue.push_function(
	    [=]() { vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayoutHandle, NULL); });

	return descriptorSetLayoutHandle;
}

/**
 * @brief Creates the descriptor set layout for the materials
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor set layout is added to the deletion queue
 * @return the descriptor set layout handle
 */
inline VkDescriptorSetLayout createMaterialDescriptorSetLayout(VkDevice logicalDevice,
                                                               DeletionQueue& deletionQueue)
{
	VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> materialDescriptorSetLayoutBindingList = {
	    {.binding = 0,
	     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	     .descriptorCount = 1,
	     .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	     .pImmutableSamplers = NULL},
	    {.binding = 1,
	     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	     .descriptorCount = 1,
	     .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	     .pImmutableSamplers = NULL},
	};

	VkDescriptorSetLayoutCreateInfo materialDescriptorSetLayoutCreateInfo
	    = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
	       .pNext = NULL,
	       .flags = 0,
	       .bindingCount = (uint32_t)materialDescriptorSetLayoutBindingList.size(),
	       .pBindings = materialDescriptorSetLayoutBindingList.data()};

	VkResult result = vkCreateDescriptorSetLayout(logicalDevice,
	                                              &materialDescriptorSetLayoutCreateInfo,
	                                              NULL,
	                                              &materialDescriptorSetLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateDescriptorSetLayout");
	}

	deletionQueue.push_function(
	    [=]()
	    { vkDestroyDescriptorSetLayout(logicalDevice, materialDescriptorSetLayoutHandle, NULL); });

	return materialDescriptorSetLayoutHandle;
}

/**
 * @brief Allocates the descriptor sets from the pool
 *
 * @param logicalDevice
 * @param descriptorPoolHandle the pool to allocate the descriptor sets from
 * @param raytracingInfo the descriptor set handles are stored in the raytracingInfo object
 * (raytracingInfo.descriptorSetHandleList)
 * @param descriptorSetLayoutHandleList the list of descriptor set layouts to allocate
 */
inline void
allocateDescriptorSetLayouts(VkDevice logicalDevice,
                             VkDescriptorPool& descriptorPoolHandle,
                             RaytracingInfo& raytracingInfo,
                             std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = NULL,
	    .descriptorPool = descriptorPoolHandle,
	    .descriptorSetCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	};

	VkResult result = vkAllocateDescriptorSets(
	    logicalDevice, &descriptorSetAllocateInfo, raytracingInfo.descriptorSetHandleList.data());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkAllocateDescriptorSets");
	}
}

/**
 * @brief creates the pipeline layout for the ray tracing pipeline
 *
 * @param logicalDevice
 * @param deletionQueue the pipeline layout is added to the deletion queue
 * @param raytracingInfo the pipeline layout handle is stored in the raytracingInfo object
 * @param descriptorSetLayoutHandleList the list of descriptor set layouts to use in the pipeline,
 * see allocateDescriptorSetLayouts, createMaterialDescriptorSetLayout and
 * createDescriptorSetLayout
 */
inline void
createPipelineLayout(VkDevice logicalDevice,
                     DeletionQueue& deletionQueue,
                     RaytracingInfo& raytracingInfo,
                     const std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList)
{
	std::vector<VkPushConstantRange> pushConstantRanges{
	    {
	        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	                      | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	        .offset = 0,
	        .size = sizeof(RaytracingDataConstants),
	    },
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .setLayoutCount = static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
	    .pSetLayouts = descriptorSetLayoutHandleList.data(),
	    .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
	    .pPushConstantRanges = pushConstantRanges.data(),
	};

	// TODO: consider returning the pipeline layout handle instead of storing it in the
	// raytracingInfo directly and assigning it to the raytracingInfo object outside of this
	// function
	VkResult result = vkCreatePipelineLayout(
	    logicalDevice, &pipelineLayoutCreateInfo, NULL, &raytracingInfo.pipelineLayoutHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreatePipelineLayout");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    { vkDestroyPipelineLayout(logicalDevice, raytracingInfo.pipelineLayoutHandle, NULL); });
}

/**
 * @brief creates the shader module instances for the shaders used in ray tracing, e.g. closed hit
 * shader, intersection shader, etc.
 *
 * @param logicalDevice
 * @param deletionQueue the modules are added to the deletion queue
 * @param raytracingInfo the modules are stored inside the raytracingInfo object
 */
inline void loadShaderModules(VkDevice logicalDevice,
                              DeletionQueue& deletionQueue,
                              RaytracingInfo& raytracingInfo)
{

	// =========================================================================
	// Ray Closest Hit Shader Module
	ltracer::shader::createShaderModule("shaders/shader.rchit.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayClosestHitShaderModuleHandle);

	// =========================================================================
	// Ray Generate Shader Module
	ltracer::shader::createShaderModule("shaders/shader.rgen.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayGenerateShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module

	ltracer::shader::createShaderModule("shaders/shader.rmiss.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayMissShaderModuleHandle);

	// =========================================================================
	// Ray Miss Shader Module (Shadow)

	ltracer::shader::createShaderModule("shaders/shader_shadow.rmiss.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayMissShadowShaderModuleHandle);

	// =========================================================================
	// Ray AABB Intersection Module
	ltracer::shader::createShaderModule("shaders/shader_aabb.rint.spv",
	                                    logicalDevice,
	                                    deletionQueue,
	                                    raytracingInfo.rayAABBIntersectionModuleHandle);
}

/**
 * @brief Creates the ray tracing pipeline
 *
 * @param logicalDevice
 * @param deletionQueue the pipeline is added to the deletion queue
 * @param raytracingInfo the pipeline handle is stored in the raytracingInfo object
 */
inline void createRaytracingPipeline(VkDevice logicalDevice,
                                     DeletionQueue& deletionQueue,
                                     RaytracingInfo& raytracingInfo)
{
	enum shaderIndices
	{
		ClosestHit = 0,
		RayGen = 1,
		RayMiss = 2,
		RayMissShadow = 3,
		RayAABBIntersection = 4,
		ShaderStageCount
	};

	std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfoList;
	pipelineShaderStageCreateInfoList.resize(ShaderStageCount);

	pipelineShaderStageCreateInfoList[shaderIndices::ClosestHit] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	    .module = raytracingInfo.rayClosestHitShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayGen] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	    .module = raytracingInfo.rayGenerateShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayMiss] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	    .module = raytracingInfo.rayMissShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayMissShadow] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
	    .module = raytracingInfo.rayMissShadowShaderModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};
	pipelineShaderStageCreateInfoList[shaderIndices::RayAABBIntersection] = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	    .module = raytracingInfo.rayAABBIntersectionModuleHandle,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> rayTracingShaderGroupCreateInfoList;

	raytracingInfo.shaderGroupCount = 4;
	rayTracingShaderGroupCreateInfoList.resize(raytracingInfo.shaderGroupCount);

	// Group 1
	raytracingInfo.hitGroupSize = 1;
	raytracingInfo.hitGroupOffset = 0;
	// Ray Closest Hit & Ray Intersection
	rayTracingShaderGroupCreateInfoList[raytracingInfo.hitGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
	    .generalShader = VK_SHADER_UNUSED_KHR,
	    .closestHitShader = shaderIndices::ClosestHit,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = shaderIndices::RayAABBIntersection,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 2
	raytracingInfo.rayGenGroupSize = 1;
	raytracingInfo.rayGenGroupOffset = raytracingInfo.hitGroupOffset + raytracingInfo.hitGroupSize;
	// Ray Gen
	rayTracingShaderGroupCreateInfoList[raytracingInfo.rayGenGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayGen,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 3
	raytracingInfo.missGroupSize = 2;
	raytracingInfo.missGroupOffset
	    = raytracingInfo.rayGenGroupOffset + raytracingInfo.rayGenGroupSize;
	// Ray Miss
	rayTracingShaderGroupCreateInfoList[raytracingInfo.missGroupOffset + 0] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayMiss,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	// Group 4
	// Ray Miss Shadow
	rayTracingShaderGroupCreateInfoList[raytracingInfo.missGroupOffset + 1] = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
	    .generalShader = shaderIndices::RayMissShadow,
	    .closestHitShader = VK_SHADER_UNUSED_KHR,
	    .anyHitShader = VK_SHADER_UNUSED_KHR,
	    .intersectionShader = VK_SHADER_UNUSED_KHR,
	    .pShaderGroupCaptureReplayHandle = NULL,
	};

	//==========================================================================
	// Create Ray Tracing Pipeline
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
	    .pNext = NULL,
	    .flags = 0,
	    .stageCount = static_cast<uint32_t>(pipelineShaderStageCreateInfoList.size()),
	    .pStages = pipelineShaderStageCreateInfoList.data(),
	    .groupCount = static_cast<uint32_t>(rayTracingShaderGroupCreateInfoList.size()),
	    .pGroups = rayTracingShaderGroupCreateInfoList.data(),
	    .maxPipelineRayRecursionDepth = 1,
	    .pLibraryInfo = NULL,
	    .pLibraryInterface = NULL,
	    .pDynamicState = NULL,
	    .layout = raytracingInfo.pipelineLayoutHandle,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = 0,
	};

	VkResult result = ltracer::procedures::pvkCreateRayTracingPipelinesKHR(
	    logicalDevice,
	    VK_NULL_HANDLE,
	    VK_NULL_HANDLE,
	    1,
	    &rayTracingPipelineCreateInfo,
	    NULL,
	    &raytracingInfo.rayTracingPipelineHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkCreateRayTracingPipelinesKHR");
	}

	deletionQueue.push_function(
	    [=, &raytracingInfo]()
	    { vkDestroyPipeline(logicalDevice, raytracingInfo.rayTracingPipelineHandle, NULL); });
}

/**
 * @brief Updates the buffers used in the ray tracing shaders, e.g. acceleration structure handle,
 * image handle, etc.
 *
 * @param logicalDevice
 * @param raytracingInfo
 * @param meshObjects list of mesh objects that are used in the ray tracing shaders, if not using
 * AABBs with intersection shader
 */
inline void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                                     RaytracingInfo& raytracingInfo,
                                                     const std::vector<MeshObject>& meshObjects)
{
	VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptorInfo = {
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
	    .pNext = NULL,
	    .accelerationStructureCount = 1,
	    .pAccelerationStructures = &raytracingInfo.topLevelAccelerationStructureHandle,
	};

	VkDescriptorBufferInfo uniformDescriptorInfo = {
	    .buffer = raytracingInfo.uniformBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo tetrahedronsDescriptorInfo = {
	    .buffer = raytracingInfo.objectBuffers.tetrahedronsBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo spheresDescriptorInfo = {
	    .buffer = raytracingInfo.objectBuffers.spheresBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo rectangularBezierSurfaces2x2DescriptorInfo = {
	    .buffer = raytracingInfo.objectBuffers.rectangularBezierSurfaces2x2BufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	VkDescriptorBufferInfo slicingPlanesDescriptorInfo = {
	    .buffer = raytracingInfo.objectBuffers.slicingPlanesBufferHandle,
	    .offset = 0,
	    .range = VK_WHOLE_SIZE,
	};

	std::vector<VkWriteDescriptorSet> writeDescriptorSetList;

	// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
	// descriptor set, for now we are using the same vertex and index buffer
	VkDescriptorBufferInfo indexDescriptorInfo = {};
	VkDescriptorBufferInfo vertexDescriptorInfo = {};

	if (meshObjects.size() > 0)
	{
		indexDescriptorInfo.buffer = meshObjects[0].indexBufferHandle;
		indexDescriptorInfo.offset = 0;
		indexDescriptorInfo.range = VK_WHOLE_SIZE;

		// TODO: replace meshObjects[0] with the correct mesh object and dynamically select
		// descriptor set, for now we are using the same vertex and index buffer
		vertexDescriptorInfo.buffer = meshObjects[0].vertexBufferHandle;
		vertexDescriptorInfo.offset = 0;
		vertexDescriptorInfo.range = VK_WHOLE_SIZE;

		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 2,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &indexDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 3,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &vertexDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	VkDescriptorImageInfo rayTraceImageDescriptorInfo = {
	    .sampler = VK_NULL_HANDLE,
	    .imageView = raytracingInfo.rayTraceImageViewHandle,
	    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	writeDescriptorSetList.push_back({
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = &accelerationStructureDescriptorInfo,
	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	    .dstBinding = 0,
	    .dstArrayElement = 0,
	    .descriptorCount = 1,
	    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	    .pImageInfo = NULL,
	    .pBufferInfo = NULL,
	    .pTexelBufferView = NULL,
	});

	writeDescriptorSetList.push_back({
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = NULL,
	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	    .dstBinding = 1,
	    .dstArrayElement = 0,
	    .descriptorCount = 1,
	    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	    .pImageInfo = NULL,
	    .pBufferInfo = &uniformDescriptorInfo,
	    .pTexelBufferView = NULL,
	});

	writeDescriptorSetList.push_back({
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = NULL,
	    .dstSet = raytracingInfo.descriptorSetHandleList[0],
	    .dstBinding = 4,
	    .dstArrayElement = 0,
	    .descriptorCount = 1,
	    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	    .pImageInfo = &rayTraceImageDescriptorInfo,
	    .pBufferInfo = NULL,
	    .pTexelBufferView = NULL,
	});

	if (raytracingInfo.objectBuffers.tetrahedronsBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 5,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &tetrahedronsDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingInfo.objectBuffers.spheresBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 6,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &spheresDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingInfo.objectBuffers.rectangularBezierSurfaces2x2BufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 7,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &rectangularBezierSurfaces2x2DescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}

	if (raytracingInfo.objectBuffers.slicingPlanesBufferHandle != VK_NULL_HANDLE)
	{
		writeDescriptorSetList.push_back({
		    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		    .pNext = NULL,
		    .dstSet = raytracingInfo.descriptorSetHandleList[0],
		    .dstBinding = 8,
		    .dstArrayElement = 0,
		    .descriptorCount = 1,
		    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    .pImageInfo = NULL,
		    .pBufferInfo = &slicingPlanesDescriptorInfo,
		    .pTexelBufferView = NULL,
		});
	}
	vkUpdateDescriptorSets(logicalDevice,
	                       static_cast<uint32_t>(writeDescriptorSetList.size()),
	                       writeDescriptorSetList.data(),
	                       0,
	                       NULL);
}

/**
 * @brief creates the vertex and index buffer for the model and copies the data from the meshObject
 * into it
 *
 * @param logicalDevice
 * @param physicalDevice
 * @param deletionQueue the buffers are added to the deletion queue
 * @param meshObject the mesh object containing the vertices and indices that will be copies into
 * the buffers
 */
inline void loadAndCreateVertexAndIndexBufferForModel(VkDevice logicalDevice,
                                                      VkPhysicalDevice physicalDevice,
                                                      DeletionQueue& deletionQueue,
                                                      MeshObject& meshObject)
{

	// =========================================================================
	// Vertex Buffer
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(float) * meshObject.vertices.size() * 3,
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             meshObject.vertexBufferHandle,
	             meshObject.vertexBufferDeviceMemoryHandle);

	void* hostVertexMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              meshObject.vertexBufferDeviceMemoryHandle,
	                              0,
	                              sizeof(float) * meshObject.vertices.size() * 3,
	                              0,
	                              &hostVertexMemoryBuffer);

	memcpy(hostVertexMemoryBuffer,
	       meshObject.vertices.data(),
	       sizeof(float) * meshObject.vertices.size() * 3);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, meshObject.vertexBufferDeviceMemoryHandle);

	VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = meshObject.vertexBufferHandle};

	meshObject.vertexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &vertexBufferDeviceAddressInfo);

	// =========================================================================
	// Index Buffer

	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             sizeof(uint32_t) * meshObject.indices.size(),
	             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	                 | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
	                 | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             meshObject.indexBufferHandle,
	             meshObject.indexBufferDeviceMemoryHandle);

	void* hostIndexMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     meshObject.indexBufferDeviceMemoryHandle,
	                     0,
	                     sizeof(uint32_t) * meshObject.indices.size(),
	                     0,
	                     &hostIndexMemoryBuffer);

	memcpy(hostIndexMemoryBuffer,
	       meshObject.indices.data(),
	       sizeof(uint32_t) * meshObject.indices.size());

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, meshObject.indexBufferDeviceMemoryHandle);

	VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
	    = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	       .pNext = NULL,
	       .buffer = meshObject.indexBufferHandle};

	meshObject.indexBufferDeviceAddress = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	    logicalDevice, &indexBufferDeviceAddressInfo);
}

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
 * @param aabbs the list of Axis Aligned Bounding Boxes for the objects, see AABB::from* method, it
 * extracts the aabb from the object
 * @param blasInstancesData the vector that holds all BLAS (Bottom Level Acceleration Structure)
 * instances, to which the acceleration structure is added
 * @param objectsBufferHandle the buffer handle of the objects
 * @param aabbObjectsBufferHandle the buffer handle of the AABBs for the objects
 */
template <typename T>
void createBottomLevelAccelerationStructuresForObjects(VkPhysicalDevice physicalDevice,
                                                       VkDevice logicalDevice,
                                                       DeletionQueue& deletionQueue,
                                                       const std::vector<T>& objects,
                                                       const ObjectType objectType,
                                                       std::vector<AABB>& aabbs,
                                                       std::vector<BLASInstance>& blasInstancesData,
                                                       VkBuffer& objectsBufferHandle,
                                                       VkBuffer& aabbObjectsBufferHandle)
{
	objectsBufferHandle
	    = createObjectsBuffer(physicalDevice, logicalDevice, deletionQueue, objects);

	aabbObjectsBufferHandle = createAABBBuffer(physicalDevice, logicalDevice, deletionQueue, aabbs);

	// TODO: Make it possible to have multiple BLAS with their individual AABBs
	blasInstancesData.push_back(createBottomLevelAccelerationStructureAABB(
	    logicalDevice, aabbObjectsBufferHandle, objectType, static_cast<uint32_t>(aabbs.size())));
}

/**
 * @brief Initializes the ray tracing pipeline, including the command buffers, command pool, etc.
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue the objects are added to the deletion queue
 * @param raytracingInfo the ray tracing info object that holds all the handles and data
 */
inline void initRayTracing(VkPhysicalDevice physicalDevice,
                           VkDevice logicalDevice,
                           DeletionQueue& deletionQueue,
                           RaytracingInfo& raytracingInfo)
{
	// Requesting ray tracing properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR physicalDeviceRayTracingPipelineProperties{};
	requestRaytracingProperties(physicalDevice, physicalDeviceRayTracingPipelineProperties);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	VkResult result;

	// std::vector<float> queuePrioritiesList = {1.0f};
	// VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
	//     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	//     .pNext = NULL,
	//     .flags = 0,
	//     .queueFamilyIndex =
	//         raytracingInfo.queueFamilyIndices.presentFamily.value(),
	//     .queueCount = 1,
	//     .pQueuePriorities = queuePrioritiesList.data(),
	// };

	// =========================================================================
	// Submission Queue
	if (raytracingInfo.queueFamilyIndices.graphicsFamily.has_value())
	{
		vkGetDeviceQueue(logicalDevice,
		                 raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
		                 0,
		                 &graphicsQueueHandle);
	}
	else
	{
		throw std::runtime_error("initRayTracing - vkGetDeviceQueue");
	}

	// =========================================================================
	// Device Pointer Functions
	ltracer::procedures::grabDeviceProcAddr(logicalDevice);

	// =========================================================================
	// Command Pool

	VkCommandPool commandBufferPoolHandle{
	    createCommandPool(logicalDevice, deletionQueue, raytracingInfo)};

	// =========================================================================
	// Command Buffers

	createCommandBufferBuildTopAndBottomLevel(
	    logicalDevice, deletionQueue, raytracingInfo, commandBufferPoolHandle);

	// =========================================================================
	// Surface Features
	// VkSurfaceCapabilitiesKHR surfaceCapabilities;
	// result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	//     physicalDevice, window->getVkSurface(), &surfaceCapabilities);

	// if (result != VK_SUCCESS) {
	//   throw new std::runtime_error(
	//       "initRayTracing - vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
	// }

	// =========================================================================
	// Descriptor Pool

	VkDescriptorPool descriptorPoolHandle{createDescriptorPool(logicalDevice, deletionQueue)};

	// =========================================================================
	// Descriptor Set Layout

	VkDescriptorSetLayout descriptorSetLayoutHandle{
	    createDescriptorSetLayout(logicalDevice, deletionQueue)};

	// =========================================================================
	// Material Descriptor Set Layout

	VkDescriptorSetLayout materialDescriptorSetLayoutHandle{
	    createMaterialDescriptorSetLayout(logicalDevice, deletionQueue)};

	// =========================================================================
	// Allocate Descriptor Sets

	// TODO: currently we hardcoded that we use 2 descriptor set layouts, make this more dynamic or
	// at least dont hardcode it
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList
	    = {descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

	allocateDescriptorSetLayouts(
	    logicalDevice, descriptorPoolHandle, raytracingInfo, descriptorSetLayoutHandleList);

	// =========================================================================
	// Pipeline Layout
	createPipelineLayout(
	    logicalDevice, deletionQueue, raytracingInfo, descriptorSetLayoutHandleList);
	// =========================================================================
	// Shader Modules
	loadShaderModules(logicalDevice, deletionQueue, raytracingInfo);

	// =========================================================================
	// Ray Tracing Pipeline
	createRaytracingPipeline(logicalDevice, deletionQueue, raytracingInfo);

	// =========================================================================
	// load OBJ Models
	std::string scenePath = "3d-models/cube_scene.obj";
	{
		MeshObject meshObject = MeshObject::loadFromPath(scenePath);
		meshObject.translate(20, 0, 0);
		raytracingInfo.meshObjects.push_back(std::move(meshObject));
	}
	{
		MeshObject meshObject2 = MeshObject::loadFromPath(scenePath);
		// TODO: figure out why this movement causes artifacts
		meshObject2.translate(-5, 0, 0);
		// meshObject2.transform.scale = glm::vec3(1, 1, 1);
		// meshObjects.push_back(std::move(meshObject2));
	}

	// =========================================================================
	// create aabbs (temporary)

	std::vector<Tetrahedron> tetrahedrons
	    = {Tetrahedron{glm::vec3(0, 0, 0), glm::vec3(2, 0, 0), glm::vec3(0, 0, 2)}};

	std::vector<Sphere> spheres{
	    // show origin
	    Sphere{glm::vec3(), 0.3f, static_cast<int>(ColorIdx::t_white)},
	};

	// {
	// 	std::random_device rd;
	// 	std::mt19937 gen(rd());

	// 	std::uniform_int_distribution<> positionDist(-20, 20);
	// 	std::uniform_int_distribution<> sizeDist(-20, 20);
	// 	for (int i = 0; i < 20; i++)
	// 	{
	// 		auto randomX = positionDist(gen);
	// 		auto randomY = positionDist(gen);
	// 		auto randomZ = positionDist(gen);
	// 		if (false)
	// 		{
	// 			tetrahedrons.emplace_back(
	// 			    glm::vec3(randomX, randomY, randomZ),
	// 			    glm::vec3(
	// 			        randomX + sizeDist(gen), randomY + sizeDist(gen), randomZ + sizeDist(gen)),
	// 			    glm::vec3(
	// 			        randomX + sizeDist(gen), randomY + sizeDist(gen), randomZ + sizeDist(gen)));
	// 		}
	// 		else
	// 		{
	// 			float sphereScaling = 0.1f;
	// 			spheres.emplace_back(glm::vec3(randomX, randomY, randomZ),
	// 			                     static_cast<float>(std::abs(sizeDist(gen))) * sphereScaling);
	// 		}
	// 	}
	// }

	auto surfaceTest = RectangularBezierSurface(3,
	                                            3,
	                                            {
	                                                glm::vec3(0, 0, 0),
	                                                glm::vec3(5, 0, 0),
	                                                glm::vec3(10, 0, 0),
	                                                glm::vec3(15, 0, 0),

	                                                glm::vec3(0, 0, 5),
	                                                glm::vec3(5, 0, 5),
	                                                glm::vec3(10, 0, 5),
	                                                glm::vec3(15, 0, 5),

	                                                glm::vec3(0, 0, 10),
	                                                glm::vec3(5, 0, 10),
	                                                glm::vec3(10, 0, 10),
	                                                glm::vec3(15, 0, 10),

	                                                glm::vec3(0, 5, 15),
	                                                glm::vec3(5, 5, 15),
	                                                glm::vec3(10, 5, 15),
	                                                glm::vec3(15, 5, 15),
	                                            });

	std::vector<RectangularBezierSurface> rectangularBezierSurfaces = {
	    surfaceTest,
	};

	// create vector to hold all BLAS instances
	std::vector<BLASInstance> blasInstancesData;

	// =========================================================================
	// Create AABB Buffer and BLAS for Tetrahedrons, Spheres...
	if (tetrahedrons.size() > 0)
	{
		// TODO: create some kind of tetrahedron collection and add a getAABBs() method to that or
		// something like that
		std::vector<AABB> aabbsTetrahedrons;
		for (auto&& tetrahedron : tetrahedrons)
		{
			aabbsTetrahedrons.push_back(AABB::fromTetrahedron(tetrahedron));
		}

		createBottomLevelAccelerationStructuresForObjects<Tetrahedron>(
		    physicalDevice,
		    logicalDevice,
		    deletionQueue,
		    tetrahedrons,
		    ObjectType::t_Tetrahedron,
		    aabbsTetrahedrons,
		    blasInstancesData,
		    raytracingInfo.objectBuffers.tetrahedronsBufferHandle,
		    raytracingInfo.objectBuffers.tetrahedronsAABBBufferHandle);
	}

	{
		// Visualize surface using spheres
		std::vector<RectangularBezierSurface2x2> rectangularBezierSurfaces2x2;
		if (!surfaceTest.tryConvertToRectangularSurfaces2x2(rectangularBezierSurfaces2x2))
		{
			throw std::runtime_error("failed to convert rectangularBezierSurfaces[0] to 2x2");
		}

		visualizeBezierSurface(spheres, rectangularBezierSurfaces2x2[0]);
	}

	{
		// Visualize slicing planes
		auto slicingPlanes = std::vector<SlicingPlane>{
		    {
		        glm::vec3(5, 0, 0),
		        glm::vec3(-1, 0, 0),
		    },
		};

		raytracingInfo.objectBuffers.slicingPlanesBufferHandle
		    = createObjectsBuffer(physicalDevice, logicalDevice, deletionQueue, slicingPlanes);
	}

	if (rectangularBezierSurfaces.size() > 0)
	{

		std::vector<RectangularBezierSurface2x2> rectangularBezierSurfaces2x2;
		if (!rectangularBezierSurfaces[0].tryConvertToRectangularSurfaces2x2(
		        rectangularBezierSurfaces2x2))
		{
			throw std::runtime_error("failed to convert rectangularBezierSurfaces[0] to 2x2");
		}

		std::vector<AABB> aabbsRectangularSurfaces2x2;
		for (auto&& rectangularBezierSurface2x2 : rectangularBezierSurfaces2x2)
		{
			aabbsRectangularSurfaces2x2.push_back(
			    AABB::fromRectangularBezierSurface(rectangularBezierSurface2x2));
		}

		createBottomLevelAccelerationStructuresForObjects<RectangularBezierSurface2x2>(
		    physicalDevice,
		    logicalDevice,
		    deletionQueue,
		    rectangularBezierSurfaces2x2,
		    ObjectType::t_RectangularBezierSurface2x2,
		    aabbsRectangularSurfaces2x2,
		    blasInstancesData,
		    raytracingInfo.objectBuffers.rectangularBezierSurfaces2x2BufferHandle,
		    raytracingInfo.objectBuffers.rectangularBezierSurfacesAABB2x2BufferHandle);
	}

	if (spheres.size() > 0)
	{
		std::vector<AABB> aabbsSpheres;
		for (auto&& sphere : spheres)
		{
			aabbsSpheres.push_back(AABB::fromSphere(sphere));
		}

		createBottomLevelAccelerationStructuresForObjects<Sphere>(
		    physicalDevice,
		    logicalDevice,
		    deletionQueue,
		    spheres,
		    ObjectType::t_Sphere,
		    aabbsSpheres,
		    blasInstancesData,
		    raytracingInfo.objectBuffers.spheresBufferHandle,
		    raytracingInfo.objectBuffers.spheresAABBBufferHandle);
	}

	// =========================================================================
	// convert to bottom level acceleration structure
	for (auto&& obj : raytracingInfo.meshObjects)
	{
		loadAndCreateVertexAndIndexBufferForModel(
		    logicalDevice, physicalDevice, deletionQueue, obj);

		// =========================================================================
		// Bottom Level Acceleration Structure
		BLASInstance triangleBLAS = createBottomLevelAccelerationStructureTriangle(
		    obj.primitiveCount,
		    static_cast<uint32_t>(obj.vertices.size()),
		    obj.vertexBufferDeviceAddress,
		    obj.indexBufferDeviceAddress,
		    obj.transform.getTransformMatrix());
		blasInstancesData.push_back(triangleBLAS);
	}

	// =========================================================================
	// Top Level Acceleration Structure

	std::vector<VkAccelerationStructureInstanceKHR> instances;

	for (auto&& blasData : blasInstancesData)
	{
		VkAccelerationStructureKHR bottomLevelAccelerationStructure
		    = buildBottomLevelAccelerationStructure(physicalDevice,
		                                            logicalDevice,
		                                            deletionQueue,
		                                            raytracingInfo,
		                                            graphicsQueueHandle,
		                                            blasData);
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

		auto bottomLevelGeometryInstance = VkAccelerationStructureInstanceKHR{
		    .transform = blasData.transformMatrix,
		    // TODO: maybe add a method that makes sure objectType does not exceed 24 bits see:
		    // https://registry.khronos.org/vulkan/specs/latest/man/html/InstanceCustomIndexKHR.html
		    // only grab 24 bits
		    .instanceCustomIndex = static_cast<uint32_t>(blasData.objectType) & 0xFFFFFF,

		    .mask = 0xFF,
		    .instanceShaderBindingTableRecordOffset = 0,
		    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		    .accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress,
		};

		instances.push_back(std::move(bottomLevelGeometryInstance));
	}

	createAndBuildTopLevelAccelerationStructure(instances,
	                                            deletionQueue,
	                                            logicalDevice,
	                                            physicalDevice,
	                                            raytracingInfo,
	                                            graphicsQueueHandle);
	// =========================================================================
	// Uniform Buffer
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
		result = vkMapMemory(logicalDevice,
		                     raytracingInfo.uniformDeviceMemoryHandle,
		                     0,
		                     sizeof(UniformStructure),
		                     0,
		                     &hostUniformMemoryBuffer);
		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error("initRayTraci - vkMapMemory");
		}
		memcpy(hostUniformMemoryBuffer, &raytracingInfo.uniformStructure, sizeof(UniformStructure));

		vkUnmapMemory(logicalDevice, raytracingInfo.uniformDeviceMemoryHandle);
	}

	// =========================================================================
	// Update Descriptor Set
	updateAccelerationStructureDescriptorSet(
	    logicalDevice, raytracingInfo, raytracingInfo.meshObjects);

	// =========================================================================
	// Material Index Buffer

	if (raytracingInfo.meshObjects.size() > 0)
	{
		std::vector<uint32_t> materialIndexList;
		// TODO: Material list needs to include all possible materials from all objects, for now
		// we only have one object
		for (tinyobj::shape_t shape : raytracingInfo.meshObjects[0].shapes)
		{
			for (int index : shape.mesh.material_ids)
			{
				materialIndexList.push_back(static_cast<uint32_t>(index));
			}
		}

		VkBuffer materialIndexBufferHandle = VK_NULL_HANDLE;
		VkDeviceMemory materialIndexDeviceMemoryHandle = VK_NULL_HANDLE;
		createBuffer(physicalDevice,
		             logicalDevice,
		             deletionQueue,
		             sizeof(uint32_t) * materialIndexList.size(),
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		             memoryAllocateFlagsInfo,
		             materialIndexBufferHandle,
		             materialIndexDeviceMemoryHandle);

		void* hostMaterialIndexMemoryBuffer;
		result = vkMapMemory(logicalDevice,
		                     materialIndexDeviceMemoryHandle,
		                     0,
		                     sizeof(uint32_t) * materialIndexList.size(),
		                     0,
		                     &hostMaterialIndexMemoryBuffer);

		memcpy(hostMaterialIndexMemoryBuffer,
		       materialIndexList.data(),
		       sizeof(uint32_t) * materialIndexList.size());

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error("initRayTraci - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, materialIndexDeviceMemoryHandle);

		// =========================================================================
		// Material Buffer

		// TODO: Material list needs to include all possible materials from all objects, for now
		// we only have one object
		std::vector<Material> materialList(raytracingInfo.meshObjects[0].materials.size());
		for (uint32_t i = 0; i < raytracingInfo.meshObjects[0].materials.size(); i++)
		{
			memcpy(&materialList[i].ambient,
			       raytracingInfo.meshObjects[0].materials[i].ambient,
			       sizeof(float) * 3);
			memcpy(&materialList[i].diffuse,
			       raytracingInfo.meshObjects[0].materials[i].diffuse,
			       sizeof(float) * 3);
			memcpy(&materialList[i].specular,
			       raytracingInfo.meshObjects[0].materials[i].specular,
			       sizeof(float) * 3);
			memcpy(&materialList[i].emission,
			       raytracingInfo.meshObjects[0].materials[i].emission,
			       sizeof(float) * 3);
		}

		VkBuffer materialBufferHandle = VK_NULL_HANDLE;
		VkDeviceMemory materialDeviceMemoryHandle = VK_NULL_HANDLE;
		createBuffer(physicalDevice,
		             logicalDevice,
		             deletionQueue,
		             sizeof(Material) * materialList.size(),
		             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		             memoryAllocateFlagsInfo,
		             materialBufferHandle,
		             materialDeviceMemoryHandle);

		void* hostMaterialMemoryBuffer;
		result = vkMapMemory(logicalDevice,
		                     materialDeviceMemoryHandle,
		                     0,
		                     sizeof(Material) * materialList.size(),
		                     0,
		                     &hostMaterialMemoryBuffer);

		memcpy(
		    hostMaterialMemoryBuffer, materialList.data(), sizeof(Material) * materialList.size());

		if (result != VK_SUCCESS)
		{
			throw new std::runtime_error("initRayTraci - vkMapMemory");
		}

		vkUnmapMemory(logicalDevice, materialDeviceMemoryHandle);
		// =========================================================================
		// Update Material Descriptor Set

		VkDescriptorBufferInfo materialIndexDescriptorInfo
		    = {.buffer = materialIndexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

		VkDescriptorBufferInfo materialDescriptorInfo
		    = {.buffer = materialBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

		std::vector<VkWriteDescriptorSet> materialWriteDescriptorSetList = {
		    {
		        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		        .pNext = NULL,
		        .dstSet = raytracingInfo.descriptorSetHandleList[1],
		        .dstBinding = 0,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		        .pImageInfo = NULL,
		        .pBufferInfo = &materialIndexDescriptorInfo,
		        .pTexelBufferView = NULL,
		    },
		    {
		        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		        .pNext = NULL,
		        .dstSet = raytracingInfo.descriptorSetHandleList[1],
		        .dstBinding = 1,
		        .dstArrayElement = 0,
		        .descriptorCount = 1,
		        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		        .pImageInfo = NULL,
		        .pBufferInfo = &materialDescriptorInfo,
		        .pTexelBufferView = NULL,
		    },
		};

		vkUpdateDescriptorSets(logicalDevice,
		                       static_cast<uint32_t>(materialWriteDescriptorSetList.size()),
		                       materialWriteDescriptorSetList.data(),
		                       0,
		                       NULL);
	}

	// =========================================================================
	// Shader Binding Table
	VkDeviceSize progSize = physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;

	VkDeviceSize shaderBindingTableSize = progSize * raytracingInfo.shaderGroupCount;

	VkBuffer shaderBindingTableBufferHandle = VK_NULL_HANDLE;
	VkDeviceMemory shaderBindingTableDeviceMemoryHandle = VK_NULL_HANDLE;
	createBuffer(physicalDevice,
	             logicalDevice,
	             deletionQueue,
	             shaderBindingTableSize,
	             VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
	                 | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	             memoryAllocateFlagsInfo,
	             shaderBindingTableBufferHandle,
	             shaderBindingTableDeviceMemoryHandle);

	char* shaderHandleBuffer = new char[shaderBindingTableSize];
	result = ltracer::procedures::pvkGetRayTracingShaderGroupHandlesKHR(
	    logicalDevice,
	    raytracingInfo.rayTracingPipelineHandle,
	    0,
	    raytracingInfo.shaderGroupCount,
	    shaderBindingTableSize,
	    shaderHandleBuffer);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkGetRayTracingShaderGroupHandlesKHR");
	}

	void* hostShaderBindingTableMemoryBuffer;
	result = vkMapMemory(logicalDevice,
	                     shaderBindingTableDeviceMemoryHandle,
	                     0,
	                     shaderBindingTableSize,
	                     0,
	                     &hostShaderBindingTableMemoryBuffer);

	for (uint32_t x = 0; x < 4; x++)
	{
		memcpy(hostShaderBindingTableMemoryBuffer,
		       shaderHandleBuffer
		           + x * physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize,
		       physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);

		hostShaderBindingTableMemoryBuffer
		    = reinterpret_cast<char*>(hostShaderBindingTableMemoryBuffer)
		      + physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
	}

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, shaderBindingTableDeviceMemoryHandle);

	VkBufferDeviceAddressInfo shaderBindingTableBufferDeviceAddressInfo = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	    .pNext = NULL,
	    .buffer = shaderBindingTableBufferHandle,
	};

	VkDeviceAddress shaderBindingTableBufferDeviceAddress
	    = ltracer::procedures::pvkGetBufferDeviceAddressKHR(
	        logicalDevice, &shaderBindingTableBufferDeviceAddressInfo);

	VkDeviceSize hitGroupOffset = raytracingInfo.hitGroupOffset * progSize;
	VkDeviceSize rayGenOffset = raytracingInfo.rayGenGroupOffset * progSize;
	VkDeviceSize missOffset = raytracingInfo.missGroupOffset * progSize;

	raytracingInfo.rchitShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + hitGroupOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.hitGroupSize,
	};

	raytracingInfo.rgenShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + rayGenOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.rayGenGroupSize,
	};

	raytracingInfo.rmissShaderBindingTable = {
	    .deviceAddress = shaderBindingTableBufferDeviceAddress + missOffset,
	    .stride = progSize,
	    .size = progSize * raytracingInfo.missGroupSize,
	};

	raytracingInfo.callableShaderBindingTable = {};
} // namespace rt

/**
 * @brief Records the commands for the ray tracing: preparing the image, doing the ray tracing,
 * copying to the image, etc.
 *
 * @param currentExtent the current extent of the window
 * @param commandBuffer the command buffer to record to
 * @param raytracingInfo
 */
inline void recordRaytracingCommandBuffer(VkCommandBuffer commandBuffer,
                                          VkExtent2D currentExtent,
                                          RaytracingInfo& raytracingInfo)
{
	VkImageMemoryBarrier rayTraceGeneralMemoryBarrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = NULL,
      .srcAccessMask = 0,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex =
          raytracingInfo.queueFamilyIndices.presentFamily.value(),
      .dstQueueFamilyIndex =
          raytracingInfo.queueFamilyIndices.presentFamily.value(),
      .image = raytracingInfo.rayTraceImageHandle,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

	vkCmdPipelineBarrier(commandBuffer,
	                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	                     0,
	                     0,
	                     NULL,
	                     0,
	                     NULL,
	                     1,
	                     &rayTraceGeneralMemoryBarrier);

	// =========================================================================
	// Record Render Pass Command Buffers
	vkCmdBindPipeline(commandBuffer,
	                  VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
	                  raytracingInfo.rayTracingPipelineHandle);

	vkCmdBindDescriptorSets(commandBuffer,
	                        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
	                        raytracingInfo.pipelineLayoutHandle,
	                        0,
	                        static_cast<uint32_t>(raytracingInfo.descriptorSetHandleList.size()),
	                        raytracingInfo.descriptorSetHandleList.data(),
	                        0,
	                        NULL);

	// upload the matrix to the GPU via push constants
	vkCmdPushConstants(commandBuffer,
	                   raytracingInfo.pipelineLayoutHandle,
	                   VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
	                       | VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	                   0,
	                   sizeof(RaytracingDataConstants),
	                   &raytracingInfo.raytracingConstants);

	ltracer::procedures::pvkCmdTraceRaysKHR(commandBuffer,
	                                        &raytracingInfo.rgenShaderBindingTable,
	                                        &raytracingInfo.rmissShaderBindingTable,
	                                        &raytracingInfo.rchitShaderBindingTable,
	                                        &raytracingInfo.callableShaderBindingTable,
	                                        currentExtent.width,
	                                        currentExtent.height,
	                                        1);

	VkImageMemoryBarrier2 barrierRaytraceImage = {};
	barrierRaytraceImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrierRaytraceImage.srcAccessMask = 0; // No prior access
	barrierRaytraceImage.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrierRaytraceImage.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrierRaytraceImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrierRaytraceImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierRaytraceImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierRaytraceImage.image = raytracingInfo.rayTraceImageHandle;
	barrierRaytraceImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierRaytraceImage.subresourceRange.baseMipLevel = 0;
	barrierRaytraceImage.subresourceRange.levelCount = 1;
	barrierRaytraceImage.subresourceRange.baseArrayLayer = 0;
	barrierRaytraceImage.subresourceRange.layerCount = 1;
	barrierRaytraceImage.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	barrierRaytraceImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

	VkDependencyInfo dependencyInfoRaytraceImage = {};
	dependencyInfoRaytraceImage.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfoRaytraceImage.dependencyFlags = 0;
	dependencyInfoRaytraceImage.imageMemoryBarrierCount = 1;
	dependencyInfoRaytraceImage.pImageMemoryBarriers = &barrierRaytraceImage;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoRaytraceImage);
}

/**
 * @brief updates the ray tracing buffer: updates the uniform structure (holding camera transform
 * data, etc.) and increments the frame count or resets it
 *
 * @param logicalDevice
 * @param raytracingInfo
 * @param camera
 * @param resetFrameCountRequested if true, the frame count is reset to 0 causing it to redraw the
 * scene (e.g. after moving the camera, resizing the window, changing light colors, etc.)
 */
inline void updateRaytraceBuffer(VkDevice logicalDevice,
                                 RaytracingInfo& raytracingInfo,
                                 const ltracer::Camera& camera,
                                 const bool resetFrameCountRequested)
{
	if (resetFrameCountRequested)
	{
		updateUniformStructure(raytracingInfo,
		                       camera.transform.position,
		                       camera.transform.getForward(),
		                       camera.transform.getUp(),
		                       camera.transform.getRight());
		resetFrameCount(raytracingInfo);
	}
	else
	{
		raytracingInfo.uniformStructure.frameCount += 1;
	}

	// TODO: we need some synchronization here probably
	void* hostUniformMemoryBuffer;
	VkResult result = vkMapMemory(logicalDevice,
	                              raytracingInfo.uniformDeviceMemoryHandle,
	                              0,
	                              sizeof(UniformStructure),
	                              0,
	                              &hostUniformMemoryBuffer);

	memcpy(hostUniformMemoryBuffer, &raytracingInfo.uniformStructure, sizeof(UniformStructure));

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTracing - vkMapMemory");
	}

	vkUnmapMemory(logicalDevice, raytracingInfo.uniformDeviceMemoryHandle);
}

/**
 * @brief frees the image and image view and image device memory that is used for ray tracing, this
 * is needed for example when resizing the window
 *
 * @param logicalDevice
 * @param rayTraceImageHandle
 * @param rayTraceImageViewHandle
 * @param rayTraceImageDeviceMemoryHandle
 */
inline void freeRaytraceImageAndImageView(VkDevice logicalDevice,
                                          VkImage& rayTraceImageHandle,
                                          VkImageView& rayTraceImageViewHandle,
                                          VkDeviceMemory& rayTraceImageDeviceMemoryHandle)
{
	assert(rayTraceImageHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkDestroyImage(logicalDevice, rayTraceImageHandle, NULL);

	assert(rayTraceImageViewHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkDestroyImageView(logicalDevice, rayTraceImageViewHandle, nullptr);

	assert(rayTraceImageDeviceMemoryHandle != VK_NULL_HANDLE
	       && "This should only be called when recreating the image and imageview");
	vkFreeMemory(logicalDevice, rayTraceImageDeviceMemoryHandle, nullptr);
}

/**
 * @brief Creates the image that is used for ray tracing
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param swapChainFormat the format of the swap chain
 * @param currentExtent the current window size
 * @param raytracingInfo the handles will be stored in the raytracingInfo struct
 */
inline void createRaytracingImage(VkPhysicalDevice physicalDevice,
                                  VkDevice logicalDevice,
                                  VkFormat swapChainFormat,
                                  VkExtent2D currentExtent,
                                  RaytracingInfo& raytracingInfo)
{
	VkImageCreateInfo rayTraceImageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = swapChainFormat,
        .extent =
            {
                .width = currentExtent.width,
                .height = currentExtent.height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

	// TODO: return this image instead of storing it in the raytracingInfo struct directly
	VkResult result = vkCreateImage(
	    logicalDevice, &rayTraceImageCreateInfo, NULL, &raytracingInfo.rayTraceImageHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkCreateImage");
	}

	VkMemoryRequirements rayTraceImageMemoryRequirements;
	vkGetImageMemoryRequirements(
	    logicalDevice, raytracingInfo.rayTraceImageHandle, &rayTraceImageMemoryRequirements);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	uint32_t rayTraceImageMemoryTypeIndex = 0;
	for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount; x++)
	{
		if ((rayTraceImageMemoryRequirements.memoryTypeBits & (1 << x))
		    && (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags
		        & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		           == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{

			rayTraceImageMemoryTypeIndex = x;
			break;
		}
	}

	VkMemoryAllocateInfo rayTraceImageMemoryAllocateInfo
	    = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	       .pNext = &memoryAllocateFlagsInfo,
	       .allocationSize = rayTraceImageMemoryRequirements.size,
	       .memoryTypeIndex = rayTraceImageMemoryTypeIndex};

	// TODO: is reallocating memory here necessary? just rebinding should be enough
	result = vkAllocateMemory(logicalDevice,
	                          &rayTraceImageMemoryAllocateInfo,
	                          NULL,
	                          &raytracingInfo.rayTraceImageDeviceMemoryHandle);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkAllocateMemory");
	}

	result = vkBindImageMemory(logicalDevice,
	                           raytracingInfo.rayTraceImageHandle,
	                           raytracingInfo.rayTraceImageDeviceMemoryHandle,
	                           0);
	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkBindImageMemory");
	}
}

/**
 * @brief creates the image view object for the ray tracing image
 *
 * @param logicalDevice
 * @param swapChainFormat
 * @param rayTraceImageHandle
 * @return VkImageView the image view for the image
 */
inline VkImageView createRaytracingImageView(VkDevice logicalDevice,
                                             VkFormat swapChainFormat,
                                             const VkImage& rayTraceImageHandle)
{
	VkImageView rayTraceImageViewHandle = VK_NULL_HANDLE;
	VkImageViewCreateInfo rayTraceImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = rayTraceImageHandle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapChainFormat,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

	VkResult result = vkCreateImageView(
	    logicalDevice, &rayTraceImageViewCreateInfo, NULL, &rayTraceImageViewHandle);

	if (result != VK_SUCCESS)
	{
		throw new std::runtime_error("initRayTraci - vkCreateImageView");
	}
	return rayTraceImageViewHandle;
}

/**
 * @brief frees the current ray tracing image and recreates it with the new window dimensions, this
 * is needed for example after resizing the window
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param window
 * @param raytracingInfo
 */
inline void recreateRaytracingImageBuffer(VkPhysicalDevice physicalDevice,
                                          VkDevice logicalDevice,
                                          VkFormat swapChainImageFormat,
                                          VkExtent2D windowExtent,
                                          RaytracingInfo& raytracingInfo)
{
	freeRaytraceImageAndImageView(logicalDevice,
	                              raytracingInfo.rayTraceImageHandle,
	                              raytracingInfo.rayTraceImageViewHandle,
	                              raytracingInfo.rayTraceImageDeviceMemoryHandle);

	createRaytracingImage(
	    physicalDevice, logicalDevice, swapChainImageFormat, windowExtent, raytracingInfo);

	raytracingInfo.rayTraceImageViewHandle = createRaytracingImageView(
	    logicalDevice, swapChainImageFormat, raytracingInfo.rayTraceImageHandle);

	updateAccelerationStructureDescriptorSet(
	    logicalDevice, raytracingInfo, raytracingInfo.meshObjects);

	// reset frame count so the window gets refreshed properly
	resetFrameCount(raytracingInfo);
}

} // namespace rt
} // namespace ltracer
