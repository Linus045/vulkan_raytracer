#pragma once

#include "raytracing_scene.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

#include "aabb.hpp"
#include "blas.hpp"
#include "camera.hpp"
#include "common_types.h"
#include "deletion_queue.hpp"
#include "model.hpp"
#include "types.hpp"

#include "tiny_obj_loader.h"

namespace tracer
{
namespace rt
{

// TODO: utilize https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html
// to get better error messages

/**
 * @brief Updates the buffers used in the ray tracing shaders, e.g. acceleration structure handle,
 * image handle, etc.
 *
 * @param logicalDevice
 * @param raytracingInfo
 * @param meshObjects list of mesh objects that are used in the ray tracing shaders, if not using
 * AABBs with intersection shader
 */
void updateAccelerationStructureDescriptorSet(VkDevice logicalDevice,
                                              const rt::RaytracingScene& raytracingScene,
                                              RaytracingInfo& raytracingInfo);

/**
 * @brief Retrieves the raytracing properties from the physical device and stores them in the
 * physicalDeviceRayTracingPipelineProperties reference
 *
 * @param physicalDevice
 * @param physicalDeviceRayTracingPipelineProperties the properties object to be filled
 */
void requestRaytracingProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR& physicalDeviceRayTracingPipelineProperties,
    VkPhysicalDeviceAccelerationStructurePropertiesKHR&
        physicalDeviceAccelerationStructureProperties);

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
void createCommandBufferBuildTopAndBottomLevel(VkDevice logicalDevice,
                                               DeletionQueue& deletionQueue,
                                               RaytracingInfo& raytracingInfo,
                                               VkCommandPool& commandBufferPoolHandle);

/**
 * @brief Creates the ray tracing pipeline
 *
 * @param logicalDevice
 * @param deletionQueue the pipeline is added to the deletion queue
 * @param raytracingInfo the pipeline handle is stored in the raytracingInfo object
 */
void createRaytracingPipeline(VkDevice logicalDevice,
                              DeletionQueue& deletionQueue,
                              RaytracingInfo& raytracingInfo);

/**
 * @brief Initializes the ray tracing pipeline, including the command buffers, command pool, etc.
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param deletionQueue the objects are added to the deletion queue
 * @param raytracingInfo the ray tracing info object that holds all the handles and data
 */
void initRayTracing(VkPhysicalDevice physicalDevice,
                    VkDevice logicalDevice,
                    VmaAllocator vmaAllocator,
                    DeletionQueue& deletionQueue,
                    RaytracingInfo& raytracingInfo,
                    VkPhysicalDeviceAccelerationStructurePropertiesKHR&
                        physicalDeviceAccelerationStructureProperties);

/**
 * @brief Records the commands for the ray tracing: preparing the image, doing the ray tracing,
 * copying to the image, etc.
 *
 * @param currentExtent the current extent of the window
 * @param commandBuffer the command buffer to record to
 * @param raytracingInfo
 */
void recordRaytracingCommandBuffer(VkCommandBuffer commandBuffer,
                                   VkExtent2D currentExtent,
                                   RaytracingInfo& raytracingInfo);

} // namespace rt

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
VkBuffer createObjectsBuffer(VkPhysicalDevice physicalDevice,
                             VkDevice logicalDevice,
                             DeletionQueue& deletionQueue,
                             const std::vector<T>& objects);

/**
 * @brief Creates the command pool used for the ray tracing operations
 *
 * @param logicalDevice
 * @param deletionQueue the command pool will be added to the deletion queue
 * @param raytracingInfo the ray tracing info object
 * @param commandBufferPoolHandle The handle will be stored in
 */
VkCommandPool createCommandPool(VkDevice logicalDevice,
                                DeletionQueue& deletionQueue,
                                RaytracingInfo& raytracingInfo);

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
 * @brief Creates the descriptor pool used for the raytracing, containing the various buffers used
 * in the shaders
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor pool is added to the deletion queue
 * @return VkDescriptorPool The descriptor pool handle
 */
VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, DeletionQueue& deletionQueue);

/**
 * @brief Creates the descriptor set layout for the ray tracing pipeline
 * The layout defines the various buffer types and at what stage they are used in the shader
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor set layout is added to the deletion queue
 * @return VkDescriptorSetLayout The descriptor set layout handle
 */
VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice,
                                                DeletionQueue& deletionQueue);

/**
 * @brief Creates the descriptor set layout for the materials
 *
 * @param logicalDevice
 * @param deletionQueue the descriptor set layout is added to the deletion queue
 * @return the descriptor set layout handle
 */
VkDescriptorSetLayout createMaterialDescriptorSetLayout(VkDevice logicalDevice,
                                                        DeletionQueue& deletionQueue);

VkDescriptorSetLayout createRaytracingImageDescriptorSetLayout(VkDevice logicalDevice,
                                                               DeletionQueue& deletionQueue);
std::vector<VkDescriptorSet>
allocateDescriptorSetLayouts(VkDevice logicalDevice,
                             VkDescriptorPool& descriptorPoolHandle,
                             std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList);

/**
 * @brief creates the pipeline layout for the ray tracing pipeline
 *
 * @param logicalDevice
 * @param deletionQueue the pipeline layout is added to the deletion queue
 * @param raytracingInfo the pipeline layout handle is stored in the raytracingInfo object
 * @param descriptorSetLayoutHandleList the list of descriptor set layouts to use in the
 * pipeline, see allocateDescriptorSetLayouts, createMaterialDescriptorSetLayout and
 * createDescriptorSetLayout
 */
void createPipelineLayout(VkDevice logicalDevice,
                          DeletionQueue& deletionQueue,
                          RaytracingInfo& raytracingInfo,
                          const std::vector<VkDescriptorSetLayout>& descriptorSetLayoutHandleList);

/**
 * @brief creates the shader module instances for the shaders used in ray tracing, e.g. closed hit
 * shader, intersection shader, etc.
 *
 * @param logicalDevice
 * @param deletionQueue the modules are added to the deletion queue
 * @param raytracingInfo the modules are stored inside the raytracingInfo object
 */
void loadShaderModules(VkDevice logicalDevice,
                       DeletionQueue& deletionQueue,
                       RaytracingInfo& raytracingInfo);

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
void loadAndCreateVertexAndIndexBufferForModel(VkDevice logicalDevice,
                                               VkPhysicalDevice physicalDevice,
                                               DeletionQueue& deletionQueue,
                                               MeshObject& meshObject);

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
void updateRaytraceBuffer(VkDevice logicalDevice,
                          VmaAllocator vmaAllocator,
                          RaytracingInfo& raytracingInfo,
                          const bool resetFrameCountRequested);

/**
 * @brief frees the image and image view and image device memory that is used for ray tracing, this
 * is needed for example when resizing the window
 *
 * @param logicalDevice
 * @param rayTraceImageHandle
 * @param rayTraceImageViewHandle
 * @param rayTraceImageDeviceMemoryHandle
 */
void freeRaytraceImageAndImageView(VkDevice logicalDevice,
                                   VmaAllocator vmaAllocator,
                                   VkImage& rayTraceImageHandle,
                                   VkImageView& rayTraceImageViewHandle,
                                   VmaAllocation& rayTraceImageAllocation);

/**
 * @brief Creates the image that is used for ray tracing
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param swapChainFormat the format of the swap chain
 * @param currentExtent the current window size
 * @param raytracingInfo the handles will be stored in the raytracingInfo struct
 */
void createRaytracingImage(VkPhysicalDevice physicalDevice,
                           VmaAllocator vmaAllocator,
                           VkExtent2D currentExtent,
                           RaytracingInfo& raytracingInfo);

/**
 * @brief creates the image view object for the ray tracing image
 *
 * @param logicalDevice
 * @param swapChainFormat
 * @param rayTraceImageHandle
 * @return VkImageView the image view for the image
 */
VkImageView createRaytracingImageView(VkDevice logicalDevice, const VkImage& rayTraceImageHandle);

/**
 * @brief frees the current ray tracing image and recreates it with the new window dimensions, this
 * is needed for example after resizing the window
 *
 * @param physicalDevice
 * @param logicalDevice
 * @param window
 * @param raytracingInfo
 */
void recreateRaytracingImageBuffer(VkPhysicalDevice physicalDevice,
                                   VkDevice logicalDevice,
                                   VmaAllocator vmaAllocator,
                                   VkExtent2D windowExtent,
                                   RaytracingInfo& raytracingInfo);

void prepareRaytracingImageLayout(VkDevice logicalDevice, const RaytracingInfo& raytracingInfo);

} // namespace tracer
