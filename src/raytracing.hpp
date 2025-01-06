#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "src/camera.hpp"
#include "src/deletion_queue.hpp"
#include "tiny_obj_loader.h"

#include "src/types.hpp"
#include "src/window.hpp"

namespace ltracer {

static UniformStructure uniformStructure;

static VkDeviceMemory vertexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkBuffer shaderBindingTableBufferHandle = VK_NULL_HANDLE;
static VkBuffer materialBufferHandle = VK_NULL_HANDLE;
static VkBuffer materialIndexBufferHandle = VK_NULL_HANDLE;
static VkBuffer uniformBufferHandle = VK_NULL_HANDLE;
static VkFence topLevelAccelerationStructureBuildFenceHandle = VK_NULL_HANDLE;
static VkBuffer topLevelAccelerationStructureScratchBufferHandle =
    VK_NULL_HANDLE;
static VkAccelerationStructureKHR topLevelAccelerationStructureHandle =
    VK_NULL_HANDLE;
static VkBuffer topLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
static VkBuffer bottomLevelGeometryInstanceBufferHandle = VK_NULL_HANDLE;
static VkFence bottomLevelAccelerationStructureBuildFenceHandle =
    VK_NULL_HANDLE;
static VkBuffer bottomLevelAccelerationStructureScratchBufferHandle =
    VK_NULL_HANDLE;
static VkAccelerationStructureKHR bottomLevelAccelerationStructureHandle =
    VK_NULL_HANDLE;
static VkBuffer bottomLevelAccelerationStructureBufferHandle = VK_NULL_HANDLE;
static VkBuffer indexBufferHandle = VK_NULL_HANDLE;
static VkBuffer vertexBufferHandle = VK_NULL_HANDLE;
static VkShaderModule rayMissShadowShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayMissShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayGenerateShaderModuleHandle = VK_NULL_HANDLE;
static VkShaderModule rayClosestHitShaderModuleHandle = VK_NULL_HANDLE;
static VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
static VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
static VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
static VkDeviceMemory rayTraceImageDeviceMemoryHandle = VK_NULL_HANDLE;
static VkCommandPool commandPoolHandle = VK_NULL_HANDLE;
static VkDeviceMemory uniformDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory indexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory bottomLevelAccelerationStructureDeviceMemoryHandle =
    VK_NULL_HANDLE;
static VkDeviceMemory
    bottomLevelAccelerationStructureDeviceScratchMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory bottomLevelGeometryInstanceDeviceMemoryHandle =
    VK_NULL_HANDLE;
static VkDeviceMemory topLevelAccelerationStructureDeviceMemoryHandle =
    VK_NULL_HANDLE;
static VkDeviceMemory topLevelAccelerationStructureDeviceScratchMemoryHandle =
    VK_NULL_HANDLE;
static VkDeviceMemory materialIndexDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory materialDeviceMemoryHandle = VK_NULL_HANDLE;
static VkDeviceMemory shaderBindingTableDeviceMemoryHandle = VK_NULL_HANDLE;
static void *hostUniformMemoryBuffer;
static VkQueue queueHandle = VK_NULL_HANDLE;

// Device Pointer Functions
static PFN_vkCmdTraceRaysKHR pvkCmdTraceRaysKHR = NULL;
static PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = NULL;
static PFN_vkCreateRayTracingPipelinesKHR pvkCreateRayTracingPipelinesKHR =
    NULL;
static PFN_vkGetAccelerationStructureBuildSizesKHR
    pvkGetAccelerationStructureBuildSizesKHR = NULL;
static PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR =
    NULL;
static PFN_vkGetAccelerationStructureDeviceAddressKHR
    pvkGetAccelerationStructureDeviceAddressKHR = NULL;
static PFN_vkCmdBuildAccelerationStructuresKHR
    pvkCmdBuildAccelerationStructuresKHR = NULL;
static PFN_vkDestroyAccelerationStructureKHR
    pvkDestroyAccelerationStructureKHR = NULL;
static PFN_vkGetRayTracingShaderGroupHandlesKHR
    pvkGetRayTracingShaderGroupHandlesKHR = NULL;

inline void updateUniformStructure(glm::vec3 position, glm::vec3 right,
                                   glm::vec3 up, glm::vec3 forward) {
  uniformStructure.cameraPosition[0] = position[0];
  uniformStructure.cameraPosition[1] = position[1];
  uniformStructure.cameraPosition[2] = position[2];

  uniformStructure.cameraRight[0] = right[0];
  uniformStructure.cameraRight[1] = right[1];
  uniformStructure.cameraRight[2] = right[2];

  uniformStructure.cameraUp[0] = up[0];
  uniformStructure.cameraUp[1] = up[1];
  uniformStructure.cameraUp[2] = up[2];

  uniformStructure.cameraForward[0] = forward[0];
  uniformStructure.cameraForward[1] = forward[1];
  uniformStructure.cameraForward[2] = forward[2];
}

inline VkPhysicalDeviceRayTracingPipelineFeaturesKHR
getRaytracingPipelineFeatures() {
  // =========================================================================
  // Physical Device Features

  VkPhysicalDeviceBufferDeviceAddressFeatures
      physicalDeviceBufferDeviceAddressFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
          .pNext = NULL,
          .bufferDeviceAddress = VK_TRUE,
          .bufferDeviceAddressCaptureReplay = VK_FALSE,
          .bufferDeviceAddressMultiDevice = VK_FALSE,
      };

  VkPhysicalDeviceAccelerationStructureFeaturesKHR
      physicalDeviceAccelerationStructureFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
          .pNext = &physicalDeviceBufferDeviceAddressFeatures,
          .accelerationStructure = VK_TRUE,
          .accelerationStructureCaptureReplay = VK_FALSE,
          .accelerationStructureIndirectBuild = VK_FALSE,
          .accelerationStructureHostCommands = VK_FALSE,
          .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE,
      };

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR
      physicalDeviceRayTracingPipelineFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
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

inline void grabDeviceProcAddr(VkDevice logicalDevice) {
  pvkGetBufferDeviceAddressKHR =
      (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkGetBufferDeviceAddressKHR");

  pvkCreateRayTracingPipelinesKHR =
      (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkCreateRayTracingPipelinesKHR");

  pvkGetAccelerationStructureBuildSizesKHR =
      (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");

  pvkCreateAccelerationStructureKHR =
      (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkCreateAccelerationStructureKHR");

  pvkDestroyAccelerationStructureKHR =
      (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkDestroyAccelerationStructureKHR");

  pvkGetAccelerationStructureDeviceAddressKHR =
      (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");

  pvkCmdBuildAccelerationStructuresKHR =
      (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkCmdBuildAccelerationStructuresKHR");

  pvkGetRayTracingShaderGroupHandlesKHR =
      (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
          logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");

  pvkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(
      logicalDevice, "vkCmdTraceRaysKHR");
}

inline void
buildRaytracingAccelerationStructures(VkDevice logicalDevice,
                                      RaytracingInfo raytracingInfo,
                                      VkCommandBuffer commandBuffer) {
  // =========================================================================
  // Ray Trace Image Barrier
  // (VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_GENERAL)

  // VkSubmitInfo rayTraceImageBarrierAccelerationStructureBuildSubmitInfo = {
  //     .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
  //     .pNext = NULL,
  //     .waitSemaphoreCount = 0,
  //     .pWaitSemaphores = NULL,
  //     .pWaitDstStageMask = NULL,
  //     .commandBufferCount = 1,
  //     .pCommandBuffers = &commandBuffer,
  //     .signalSemaphoreCount = 0,
  //     .pSignalSemaphores = NULL,
  // };

  // VkFenceCreateInfo
  //     rayTraceImageBarrierAccelerationStructureBuildFenceCreateInfo = {
  //         .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
  //         .pNext = NULL,
  //         .flags = 0,
  //     };

  // VkFence rayTraceImageBarrierAccelerationStructureBuildFenceHandle =
  //     VK_NULL_HANDLE;
  // VkResult result = vkCreateFence(
  //     logicalDevice,
  //     &rayTraceImageBarrierAccelerationStructureBuildFenceCreateInfo, NULL,
  //     &rayTraceImageBarrierAccelerationStructureBuildFenceHandle);

  // if (result != VK_SUCCESS) {
  //   throw new std::runtime_error("initRayTraci - vkCreateFence");
  // }

  // result = vkQueueSubmit(
  //     queueHandle, 1,
  //     &rayTraceImageBarrierAccelerationStructureBuildSubmitInfo,
  //     rayTraceImageBarrierAccelerationStructureBuildFenceHandle);

  // if (result != VK_SUCCESS) {
  //   throw new std::runtime_error("initRayTraci - vkQueueSubmit");
  // }

  // result = vkWaitForFences(
  //     logicalDevice, 1,
  //     &rayTraceImageBarrierAccelerationStructureBuildFenceHandle, true,
  //     UINT32_MAX);

  // if (result != VK_SUCCESS && result != VK_TIMEOUT) {
  //   throw new std::runtime_error("initRayTraci - vkWaitForFences");
  // }
}

inline void createCommandPool(VkDevice logicalDevice,
                              std::shared_ptr<DeletionQueue> deletionQueue,
                              RaytracingInfo &raytracingInfo) {
  VkCommandPoolCreateInfo commandPoolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex =
          raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
  };

  VkResult result = vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo,
                                        NULL, &commandPoolHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateCommandPool failed");
  }

  deletionQueue->push_function([=]() {
    vkDestroyCommandPool(logicalDevice, commandPoolHandle, nullptr);
  });
}

// TODO: Split into multiple functions for each step
inline void initRayTracing(VkPhysicalDevice physicalDevice,
                           VkDevice logicalDevice,
                           std::shared_ptr<DeletionQueue> deletionQueue,
                           std::shared_ptr<ltracer::Window> window,
                           VkSwapchainKHR swapChain,
                           std::vector<VkImageView> &swapChainImageViews,
                           VkFormat swapChainFormat, VkExtent2D extent,
                           RaytracingInfo &raytracingInfo) {
  // Requesting ray tracing properties
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR
      physicalDeviceRayTracingPipelineProperties = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
          .pNext = NULL,
      };

  VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &physicalDeviceRayTracingPipelineProperties,
      .properties = physicalDeviceProperties,
  };

  vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);

  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                      &physicalDeviceMemoryProperties);

  VkResult result;

  std::vector<float> queuePrioritiesList = {1.0f};
  VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex =
          raytracingInfo.queueFamilyIndices.presentFamily.value(),
      .queueCount = 1,
      .pQueuePriorities = queuePrioritiesList.data(),
  };
  // =========================================================================
  // Submission Queue
  vkGetDeviceQueue(logicalDevice,
                   raytracingInfo.queueFamilyIndices.graphicsFamily.value(), 0,
                   &queueHandle);

  // =========================================================================
  // Device Pointer Functions
  grabDeviceProcAddr(logicalDevice);

  // =========================================================================
  // Command Pool
  createCommandPool(logicalDevice, deletionQueue, raytracingInfo);

  // =========================================================================
  // Command Buffers

  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .commandPool = commandPoolHandle,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  result = vkAllocateCommandBuffers(
      logicalDevice, &commandBufferAllocateInfo,
      &raytracingInfo.commandBufferBuildTopAndBottomLevel);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkAllocateCommandBuffers failed");
  }

  deletionQueue->push_function([=]() {
    vkFreeCommandBuffers(logicalDevice, commandPoolHandle, 1,
                         &raytracingInfo.commandBufferBuildTopAndBottomLevel);
  });

  // =========================================================================
  // Surface Features
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice, window->getVkSurface(), &surfaceCapabilities);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
  }

  // =========================================================================
  // Descriptor Pool

  std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
      {.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
       .descriptorCount = 1},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 4},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},
  };

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 2,
      .poolSizeCount = (uint32_t)descriptorPoolSizeList.size(),
      .pPoolSizes = descriptorPoolSizeList.data(),
  };

  result = vkCreateDescriptorPool(logicalDevice, &descriptorPoolCreateInfo,
                                  NULL, &descriptorPoolHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateDescriptorPool");
  }

  deletionQueue->push_function([=]() {
    vkDestroyDescriptorPool(logicalDevice, descriptorPoolHandle, NULL);
  });

  // =========================================================================
  // Descriptor Set Layout

  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
          .pImmutableSamplers = NULL,
      },
      {
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
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
      }};

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .bindingCount = (uint32_t)descriptorSetLayoutBindingList.size(),
      .pBindings = descriptorSetLayoutBindingList.data()};

  result =
      vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo,
                                  NULL, &descriptorSetLayoutHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkCreateDescriptorSetLayout");
  }

  deletionQueue->push_function([=]() {
    vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayoutHandle,
                                 NULL);
  });

  // =========================================================================
  // Material Descriptor Set Layout

  std::vector<VkDescriptorSetLayoutBinding>
      materialDescriptorSetLayoutBindingList = {
          {.binding = 0,
           .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .pImmutableSamplers = NULL},
          {.binding = 1,
           .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .pImmutableSamplers = NULL}};

  VkDescriptorSetLayoutCreateInfo materialDescriptorSetLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .bindingCount = (uint32_t)materialDescriptorSetLayoutBindingList.size(),
      .pBindings = materialDescriptorSetLayoutBindingList.data()};

  result = vkCreateDescriptorSetLayout(
      logicalDevice, &materialDescriptorSetLayoutCreateInfo, NULL,
      &materialDescriptorSetLayoutHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkCreateDescriptorSetLayout");
  }

  deletionQueue->push_function([=]() {
    vkDestroyDescriptorSetLayout(logicalDevice,
                                 materialDescriptorSetLayoutHandle, NULL);
  });
  // =========================================================================
  // Allocate Descriptor Sets

  std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList = {
      descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = NULL,
      .descriptorPool = descriptorPoolHandle,
      .descriptorSetCount = (uint32_t)descriptorSetLayoutHandleList.size(),
      .pSetLayouts = descriptorSetLayoutHandleList.data(),
  };

  result =
      vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocateInfo,
                               raytracingInfo.descriptorSetHandleList.data());

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateDescriptorSets");
  }

  // =========================================================================
  // Pipeline Layout

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .setLayoutCount =
          static_cast<uint32_t>(descriptorSetLayoutHandleList.size()),
      .pSetLayouts = descriptorSetLayoutHandleList.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL};

  result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo,
                                  NULL, &raytracingInfo.pipelineLayoutHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreatePipelineLayout");
  }

  deletionQueue->push_function([=]() {
    vkDestroyPipelineLayout(logicalDevice, raytracingInfo.pipelineLayoutHandle,
                            NULL);
  });
  // =========================================================================
  // Ray Closest Hit Shader Module

  std::ifstream rayClosestHitFile("shaders/shader.rchit.spv",
                                  std::ios::binary | std::ios::ate);
  std::streamsize rayClosestHitFileSize = rayClosestHitFile.tellg();
  rayClosestHitFile.seekg(0, std::ios::beg);
  std::vector<uint32_t> rayClosestHitShaderSource(rayClosestHitFileSize /
                                                  sizeof(uint32_t));

  rayClosestHitFile.read(
      reinterpret_cast<char *>(rayClosestHitShaderSource.data()),
      rayClosestHitFileSize);

  rayClosestHitFile.close();

  VkShaderModuleCreateInfo rayClosestHitShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)rayClosestHitShaderSource.size() * sizeof(uint32_t),
      .pCode = rayClosestHitShaderSource.data()};

  result =
      vkCreateShaderModule(logicalDevice, &rayClosestHitShaderModuleCreateInfo,
                           NULL, &rayClosestHitShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateShaderModule");
  }

  deletionQueue->push_function([=]() {
    vkDestroyShaderModule(logicalDevice, rayClosestHitShaderModuleHandle, NULL);
  });

  // =========================================================================
  // Ray Generate Shader Module

  std::ifstream rayGenerateFile("shaders/shader.rgen.spv",
                                std::ios::binary | std::ios::ate);
  std::streamsize rayGenerateFileSize = rayGenerateFile.tellg();
  rayGenerateFile.seekg(0, std::ios::beg);
  std::vector<uint32_t> rayGenerateShaderSource(rayGenerateFileSize /
                                                sizeof(uint32_t));

  rayGenerateFile.read(reinterpret_cast<char *>(rayGenerateShaderSource.data()),
                       rayGenerateFileSize);

  rayGenerateFile.close();

  VkShaderModuleCreateInfo rayGenerateShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)rayGenerateShaderSource.size() * sizeof(uint32_t),
      .pCode = rayGenerateShaderSource.data()};

  result =
      vkCreateShaderModule(logicalDevice, &rayGenerateShaderModuleCreateInfo,
                           NULL, &rayGenerateShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateShaderModule");
  }

  deletionQueue->push_function([=]() {
    vkDestroyShaderModule(logicalDevice, rayGenerateShaderModuleHandle, NULL);
  });

  // =========================================================================
  // Ray Miss Shader Module

  std::ifstream rayMissFile("shaders/shader.rmiss.spv",
                            std::ios::binary | std::ios::ate);
  std::streamsize rayMissFileSize = rayMissFile.tellg();
  rayMissFile.seekg(0, std::ios::beg);
  std::vector<uint32_t> rayMissShaderSource(rayMissFileSize / sizeof(uint32_t));

  rayMissFile.read(reinterpret_cast<char *>(rayMissShaderSource.data()),
                   rayMissFileSize);

  rayMissFile.close();

  VkShaderModuleCreateInfo rayMissShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)rayMissShaderSource.size() * sizeof(uint32_t),
      .pCode = rayMissShaderSource.data()};

  result = vkCreateShaderModule(logicalDevice, &rayMissShaderModuleCreateInfo,
                                NULL, &rayMissShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateShaderModule");
  }
  deletionQueue->push_function([=]() {
    vkDestroyShaderModule(logicalDevice, rayMissShaderModuleHandle, NULL);
  });

  // =========================================================================
  // Ray Miss Shader Module (Shadow)

  std::ifstream rayMissShadowFile("shaders/shader_shadow.rmiss.spv",
                                  std::ios::binary | std::ios::ate);
  std::streamsize rayMissShadowFileSize = rayMissShadowFile.tellg();
  rayMissShadowFile.seekg(0, std::ios::beg);
  std::vector<uint32_t> rayMissShadowShaderSource(rayMissShadowFileSize /
                                                  sizeof(uint32_t));

  rayMissShadowFile.read(
      reinterpret_cast<char *>(rayMissShadowShaderSource.data()),
      rayMissShadowFileSize);

  rayMissShadowFile.close();

  VkShaderModuleCreateInfo rayMissShadowShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)rayMissShadowShaderSource.size() * sizeof(uint32_t),
      .pCode = rayMissShadowShaderSource.data()};

  result =
      vkCreateShaderModule(logicalDevice, &rayMissShadowShaderModuleCreateInfo,
                           NULL, &rayMissShadowShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateShaderModule");
  }
  deletionQueue->push_function([=]() {
    vkDestroyShaderModule(logicalDevice, rayMissShadowShaderModuleHandle, NULL);
  });
  // =========================================================================
  // Ray Tracing Pipeline

  std::vector<VkPipelineShaderStageCreateInfo>
      pipelineShaderStageCreateInfoList = {
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .module = rayClosestHitShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
           .module = rayGenerateShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
           .module = rayMissShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
           .module = rayMissShadowShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL}};

  std::vector<VkRayTracingShaderGroupCreateInfoKHR>
      rayTracingShaderGroupCreateInfoList = {
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
           .generalShader = VK_SHADER_UNUSED_KHR,
           .closestHitShader = 0,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 1,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 2,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 3,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL}};

  VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .stageCount = 4,
      .pStages = pipelineShaderStageCreateInfoList.data(),
      .groupCount = 4,
      .pGroups = rayTracingShaderGroupCreateInfoList.data(),
      .maxPipelineRayRecursionDepth = 1,
      .pLibraryInfo = NULL,
      .pLibraryInterface = NULL,
      .pDynamicState = NULL,
      .layout = raytracingInfo.pipelineLayoutHandle,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
  };

  result = pvkCreateRayTracingPipelinesKHR(
      logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
      &rayTracingPipelineCreateInfo, NULL,
      &raytracingInfo.rayTracingPipelineHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkCreateRayTracingPipelinesKHR");
  }

  deletionQueue->push_function([=]() {
    vkDestroyPipeline(logicalDevice, raytracingInfo.rayTracingPipelineHandle,
                      NULL);
  });

  // =========================================================================
  // load OBJ Model

  // std::filesystem::path arrowPath = "3d-models/up_arrow.obj";
  // auto loaded_model = loadModel(arrowPath);

  // =========================================================================
  // OBJ Model

  tinyobj::ObjReaderConfig reader_config;
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile("3d-models/cube_scene.obj", reader_config)) {
    if (!reader.Error().empty()) {
      std::cerr << "TinyObjReader: " << reader.Error();
    }
    exit(1);
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t> &materials = reader.GetMaterials();

  uint32_t primitiveCount = 0;
  std::vector<uint32_t> indexList;
  for (tinyobj::shape_t shape : shapes) {

    primitiveCount += shape.mesh.num_face_vertices.size();

    for (tinyobj::index_t index : shape.mesh.indices) {
      indexList.push_back(index.vertex_index);
    }
  }

  // =========================================================================
  // Vertex Buffer
  VkBufferCreateInfo vertexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(float) * attrib.vertices.size() * 3,
      .usage =
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &vertexBufferCreateInfo, NULL,
                          &vertexBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }

  deletionQueue->push_function(
      [=]() { vkDestroyBuffer(logicalDevice, vertexBufferHandle, NULL); });

  VkMemoryRequirements vertexMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, vertexBufferHandle,
                                &vertexMemoryRequirements);

  // VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  // vkGetPhysicalDeviceMemoryProperties(physicalDevice,
  //                                     &physicalDeviceMemoryProperties);
  uint32_t vertexMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((vertexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      vertexMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo vertexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = vertexMemoryRequirements.size,
      .memoryTypeIndex = vertexMemoryTypeIndex};

  result = vkAllocateMemory(logicalDevice, &vertexMemoryAllocateInfo, NULL,
                            &vertexDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, vertexBufferHandle,
                              vertexDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  void *hostVertexMemoryBuffer;
  result = vkMapMemory(logicalDevice, vertexDeviceMemoryHandle, 0,
                       sizeof(float) * attrib.vertices.size() * 3, 0,
                       &hostVertexMemoryBuffer);

  memcpy(hostVertexMemoryBuffer, attrib.vertices.data(),
         sizeof(float) * attrib.vertices.size() * 3);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, vertexDeviceMemoryHandle);

  deletionQueue->push_function(
      [=]() { vkFreeMemory(logicalDevice, vertexDeviceMemoryHandle, NULL); });
  VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .pNext = NULL,
      .buffer = vertexBufferHandle};

  VkDeviceAddress vertexBufferDeviceAddress = pvkGetBufferDeviceAddressKHR(
      logicalDevice, &vertexBufferDeviceAddressInfo);

  // =========================================================================
  // Index Buffer

  VkBufferCreateInfo indexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(uint32_t) * indexList.size(),
      .usage =
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &indexBufferCreateInfo, NULL,
                          &indexBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }
  deletionQueue->push_function(
      [=]() { vkDestroyBuffer(logicalDevice, indexBufferHandle, NULL); });

  VkMemoryRequirements indexMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, indexBufferHandle,
                                &indexMemoryRequirements);

  uint32_t indexMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((indexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      indexMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo indexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = indexMemoryRequirements.size,
      .memoryTypeIndex = indexMemoryTypeIndex};

  result = vkAllocateMemory(logicalDevice, &indexMemoryAllocateInfo, NULL,
                            &indexDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, indexBufferHandle,
                              indexDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  void *hostIndexMemoryBuffer;
  result = vkMapMemory(logicalDevice, indexDeviceMemoryHandle, 0,
                       sizeof(uint32_t) * indexList.size(), 0,
                       &hostIndexMemoryBuffer);

  memcpy(hostIndexMemoryBuffer, indexList.data(),
         sizeof(uint32_t) * indexList.size());

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, indexDeviceMemoryHandle);
  deletionQueue->push_function(
      [=]() { vkFreeMemory(logicalDevice, indexDeviceMemoryHandle, NULL); });

  VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .pNext = NULL,
      .buffer = indexBufferHandle};

  VkDeviceAddress indexBufferDeviceAddress = pvkGetBufferDeviceAddressKHR(
      logicalDevice, &indexBufferDeviceAddressInfo);

  // =========================================================================
  // Bottom Level Acceleration Structure

  VkAccelerationStructureGeometryDataKHR
      bottomLevelAccelerationStructureGeometryData = {
          .triangles = {
              .sType =
                  VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
              .pNext = NULL,
              .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
              .vertexData = {.deviceAddress = vertexBufferDeviceAddress},
              .vertexStride = sizeof(float) * 3,
              .maxVertex = (uint32_t)attrib.vertices.size(),
              .indexType = VK_INDEX_TYPE_UINT32,
              .indexData = {.deviceAddress = indexBufferDeviceAddress},
              .transformData = {.deviceAddress = 0}}};

  VkAccelerationStructureGeometryKHR bottomLevelAccelerationStructureGeometry =
      {
          .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
          .pNext = NULL,
          .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
          .geometry = bottomLevelAccelerationStructureGeometryData,
          .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
      };

  VkAccelerationStructureBuildGeometryInfoKHR
      bottomLevelAccelerationStructureBuildGeometryInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
          .pNext = NULL,
          .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
          .flags = 0,
          .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
          .srcAccelerationStructure = VK_NULL_HANDLE,
          .dstAccelerationStructure = VK_NULL_HANDLE,
          .geometryCount = 1,
          .pGeometries = &bottomLevelAccelerationStructureGeometry,
          .ppGeometries = NULL,
          .scratchData = {.deviceAddress = 0}};

  VkAccelerationStructureBuildSizesInfoKHR
      bottomLevelAccelerationStructureBuildSizesInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
          .pNext = NULL,
          .accelerationStructureSize = 0,
          .updateScratchSize = 0,
          .buildScratchSize = 0};

  std::vector<uint32_t> bottomLevelMaxPrimitiveCountList = {primitiveCount};

  pvkGetAccelerationStructureBuildSizesKHR(
      logicalDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &bottomLevelAccelerationStructureBuildGeometryInfo,
      bottomLevelMaxPrimitiveCountList.data(),
      &bottomLevelAccelerationStructureBuildSizesInfo);

  VkBufferCreateInfo bottomLevelAccelerationStructureBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = bottomLevelAccelerationStructureBuildSizesInfo
                  .accelerationStructureSize,
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice,
                          &bottomLevelAccelerationStructureBufferCreateInfo,
                          NULL, &bottomLevelAccelerationStructureBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }

  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice, bottomLevelAccelerationStructureBufferHandle,
                    NULL);
  });

  VkMemoryRequirements bottomLevelAccelerationStructureMemoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, bottomLevelAccelerationStructureBufferHandle,
      &bottomLevelAccelerationStructureMemoryRequirements);

  uint32_t bottomLevelAccelerationStructureMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {

    if ((bottomLevelAccelerationStructureMemoryRequirements.memoryTypeBits &
         (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

      bottomLevelAccelerationStructureMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo bottomLevelAccelerationStructureMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = bottomLevelAccelerationStructureMemoryRequirements.size,
      .memoryTypeIndex = bottomLevelAccelerationStructureMemoryTypeIndex,
  };

  result = vkAllocateMemory(
      logicalDevice, &bottomLevelAccelerationStructureMemoryAllocateInfo, NULL,
      &bottomLevelAccelerationStructureDeviceMemoryHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(
      logicalDevice, bottomLevelAccelerationStructureBufferHandle,
      bottomLevelAccelerationStructureDeviceMemoryHandle, 0);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  VkAccelerationStructureCreateInfoKHR
      bottomLevelAccelerationStructureCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
          .pNext = NULL,
          .createFlags = 0,
          .buffer = bottomLevelAccelerationStructureBufferHandle,
          .offset = 0,
          .size = bottomLevelAccelerationStructureBuildSizesInfo
                      .accelerationStructureSize,
          .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
          .deviceAddress = 0};

  result = pvkCreateAccelerationStructureKHR(
      logicalDevice, &bottomLevelAccelerationStructureCreateInfo, NULL,
      &bottomLevelAccelerationStructureHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkCreateAccelerationStructureKHR");
  }

  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice,
                 bottomLevelAccelerationStructureDeviceMemoryHandle, NULL);
    pvkDestroyAccelerationStructureKHR(
        logicalDevice, bottomLevelAccelerationStructureHandle, NULL);
  });

  // =========================================================================
  // Build Bottom Level Acceleration Structure

  VkAccelerationStructureDeviceAddressInfoKHR
      bottomLevelAccelerationStructureDeviceAddressInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
          .pNext = NULL,
          .accelerationStructure = bottomLevelAccelerationStructureHandle};

  VkDeviceAddress bottomLevelAccelerationStructureDeviceAddress =
      pvkGetAccelerationStructureDeviceAddressKHR(
          logicalDevice, &bottomLevelAccelerationStructureDeviceAddressInfo);

  VkBufferCreateInfo bottomLevelAccelerationStructureScratchBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = bottomLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(
      logicalDevice, &bottomLevelAccelerationStructureScratchBufferCreateInfo,
      NULL, &bottomLevelAccelerationStructureScratchBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }

  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice,
                    bottomLevelAccelerationStructureScratchBufferHandle, NULL);
  });

  VkMemoryRequirements
      bottomLevelAccelerationStructureScratchMemoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, bottomLevelAccelerationStructureScratchBufferHandle,
      &bottomLevelAccelerationStructureScratchMemoryRequirements);

  uint32_t bottomLevelAccelerationStructureScratchMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {

    if ((bottomLevelAccelerationStructureScratchMemoryRequirements
             .memoryTypeBits &
         (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

      bottomLevelAccelerationStructureScratchMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo
      bottomLevelAccelerationStructureScratchMemoryAllocateInfo = {
          .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          .pNext = &memoryAllocateFlagsInfo,
          .allocationSize =
              bottomLevelAccelerationStructureScratchMemoryRequirements.size,
          .memoryTypeIndex =
              bottomLevelAccelerationStructureScratchMemoryTypeIndex};

  result = vkAllocateMemory(
      logicalDevice, &bottomLevelAccelerationStructureScratchMemoryAllocateInfo,
      NULL, &bottomLevelAccelerationStructureDeviceScratchMemoryHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(
      logicalDevice, bottomLevelAccelerationStructureScratchBufferHandle,
      bottomLevelAccelerationStructureDeviceScratchMemoryHandle, 0);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  VkBufferDeviceAddressInfo
      bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
          .pNext = NULL,
          .buffer = bottomLevelAccelerationStructureScratchBufferHandle};

  VkDeviceAddress bottomLevelAccelerationStructureScratchBufferDeviceAddress =
      pvkGetBufferDeviceAddressKHR(
          logicalDevice,
          &bottomLevelAccelerationStructureScratchBufferDeviceAddressInfo);

  bottomLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure =
      bottomLevelAccelerationStructureHandle;

  bottomLevelAccelerationStructureBuildGeometryInfo.scratchData = {
      .deviceAddress =
          bottomLevelAccelerationStructureScratchBufferDeviceAddress};

  VkAccelerationStructureBuildRangeInfoKHR
      bottomLevelAccelerationStructureBuildRangeInfo = {.primitiveCount =
                                                            primitiveCount,
                                                        .primitiveOffset = 0,
                                                        .firstVertex = 0,
                                                        .transformOffset = 0};

  const VkAccelerationStructureBuildRangeInfoKHR
      *bottomLevelAccelerationStructureBuildRangeInfos =
          &bottomLevelAccelerationStructureBuildRangeInfo;

  VkCommandBufferBeginInfo bottomLevelCommandBufferBeginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL,
  };

  result =
      vkBeginCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
                           &bottomLevelCommandBufferBeginInfo);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBeginCommandBuffer");
  }

  pvkCmdBuildAccelerationStructuresKHR(
      raytracingInfo.commandBufferBuildTopAndBottomLevel, 1,
      &bottomLevelAccelerationStructureBuildGeometryInfo,
      &bottomLevelAccelerationStructureBuildRangeInfos);

  result =
      vkEndCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkEndCommandBuffer");
  }

  VkSubmitInfo bottomLevelAccelerationStructureBuildSubmitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = NULL,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .pWaitDstStageMask = NULL,
      .commandBufferCount = 1,
      .pCommandBuffers = &raytracingInfo.commandBufferBuildTopAndBottomLevel,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = NULL};

  VkFenceCreateInfo bottomLevelAccelerationStructureBuildFenceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
  };

  result = vkCreateFence(
      logicalDevice, &bottomLevelAccelerationStructureBuildFenceCreateInfo,
      NULL, &bottomLevelAccelerationStructureBuildFenceHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateFence");
  }

  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice,
                 bottomLevelAccelerationStructureDeviceScratchMemoryHandle,
                 NULL);
    vkDestroyFence(logicalDevice,
                   bottomLevelAccelerationStructureBuildFenceHandle, NULL);
  });

  result = vkQueueSubmit(queueHandle, 1,
                         &bottomLevelAccelerationStructureBuildSubmitInfo,
                         bottomLevelAccelerationStructureBuildFenceHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkQueueSubmit");
  }

  result = vkWaitForFences(logicalDevice, 1,
                           &bottomLevelAccelerationStructureBuildFenceHandle,
                           true, UINT32_MAX);

  if (result != VK_SUCCESS && result != VK_TIMEOUT) {
    throw new std::runtime_error("initRayTracing - vkWaitForFences");
  }

  // vkResetCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
  // 0);

  // =========================================================================
  // Top Level Acceleration Structure

  VkAccelerationStructureInstanceKHR bottomLevelAccelerationStructureInstance =
      {.transform = {.matrix = {{1.0, 0.0, 0.0, 0.0},
                                {0.0, 1.0, 0.0, 0.0},
                                {0.0, 0.0, 1.0, 0.0}}},
       .instanceCustomIndex = 0,
       .mask = 0xFF,
       .instanceShaderBindingTableRecordOffset = 0,
       .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
       .accelerationStructureReference =
           bottomLevelAccelerationStructureDeviceAddress};

  VkBufferCreateInfo bottomLevelGeometryInstanceBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(VkAccelerationStructureInstanceKHR),
      .usage =
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice,
                          &bottomLevelGeometryInstanceBufferCreateInfo, NULL,
                          &bottomLevelGeometryInstanceBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }

  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice, bottomLevelGeometryInstanceBufferHandle,
                    NULL);
  });

  VkMemoryRequirements bottomLevelGeometryInstanceMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice,
                                bottomLevelGeometryInstanceBufferHandle,
                                &bottomLevelGeometryInstanceMemoryRequirements);

  uint32_t bottomLevelGeometryInstanceMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {

    if ((bottomLevelGeometryInstanceMemoryRequirements.memoryTypeBits &
         (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      bottomLevelGeometryInstanceMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo bottomLevelGeometryInstanceMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = bottomLevelGeometryInstanceMemoryRequirements.size,
      .memoryTypeIndex = bottomLevelGeometryInstanceMemoryTypeIndex};

  result = vkAllocateMemory(
      logicalDevice, &bottomLevelGeometryInstanceMemoryAllocateInfo, NULL,
      &bottomLevelGeometryInstanceDeviceMemoryHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result =
      vkBindBufferMemory(logicalDevice, bottomLevelGeometryInstanceBufferHandle,
                         bottomLevelGeometryInstanceDeviceMemoryHandle, 0);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  void *hostbottomLevelGeometryInstanceMemoryBuffer;
  result =
      vkMapMemory(logicalDevice, bottomLevelGeometryInstanceDeviceMemoryHandle,
                  0, sizeof(VkAccelerationStructureInstanceKHR), 0,
                  &hostbottomLevelGeometryInstanceMemoryBuffer);

  memcpy(hostbottomLevelGeometryInstanceMemoryBuffer,
         &bottomLevelAccelerationStructureInstance,
         sizeof(VkAccelerationStructureInstanceKHR));

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, bottomLevelGeometryInstanceDeviceMemoryHandle);
  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice, bottomLevelGeometryInstanceDeviceMemoryHandle,
                 NULL);
  });

  VkBufferDeviceAddressInfo bottomLevelGeometryInstanceDeviceAddressInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .pNext = NULL,
      .buffer = bottomLevelGeometryInstanceBufferHandle};

  VkDeviceAddress bottomLevelGeometryInstanceDeviceAddress =
      pvkGetBufferDeviceAddressKHR(
          logicalDevice, &bottomLevelGeometryInstanceDeviceAddressInfo);

  VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData =
      {.instances = {
           .sType =
               VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
           .pNext = NULL,
           .arrayOfPointers = VK_FALSE,
           .data = {.deviceAddress =
                        bottomLevelGeometryInstanceDeviceAddress}}};

  VkAccelerationStructureGeometryKHR topLevelAccelerationStructureGeometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .pNext = NULL,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry = topLevelAccelerationStructureGeometryData,
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR};

  VkAccelerationStructureBuildGeometryInfoKHR
      topLevelAccelerationStructureBuildGeometryInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
          .pNext = NULL,
          .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
          .flags = 0,
          .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
          .srcAccelerationStructure = VK_NULL_HANDLE,
          .dstAccelerationStructure = VK_NULL_HANDLE,
          .geometryCount = 1,
          .pGeometries = &topLevelAccelerationStructureGeometry,
          .ppGeometries = NULL,
          .scratchData = {.deviceAddress = 0}};

  VkAccelerationStructureBuildSizesInfoKHR
      topLevelAccelerationStructureBuildSizesInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
          .pNext = NULL,
          .accelerationStructureSize = 0,
          .updateScratchSize = 0,
          .buildScratchSize = 0};

  std::vector<uint32_t> topLevelMaxPrimitiveCountList = {1};

  pvkGetAccelerationStructureBuildSizesKHR(
      logicalDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &topLevelAccelerationStructureBuildGeometryInfo,
      topLevelMaxPrimitiveCountList.data(),
      &topLevelAccelerationStructureBuildSizesInfo);

  VkBufferCreateInfo topLevelAccelerationStructureBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size =
          topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize,
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice,
                          &topLevelAccelerationStructureBufferCreateInfo, NULL,
                          &topLevelAccelerationStructureBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }
  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice, topLevelAccelerationStructureBufferHandle,
                    NULL);
  });
  VkMemoryRequirements topLevelAccelerationStructureMemoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, topLevelAccelerationStructureBufferHandle,
      &topLevelAccelerationStructureMemoryRequirements);

  uint32_t topLevelAccelerationStructureMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {

    if ((topLevelAccelerationStructureMemoryRequirements.memoryTypeBits &
         (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

      topLevelAccelerationStructureMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo topLevelAccelerationStructureMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = topLevelAccelerationStructureMemoryRequirements.size,
      .memoryTypeIndex = topLevelAccelerationStructureMemoryTypeIndex};

  result = vkAllocateMemory(
      logicalDevice, &topLevelAccelerationStructureMemoryAllocateInfo, NULL,
      &topLevelAccelerationStructureDeviceMemoryHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(
      logicalDevice, topLevelAccelerationStructureBufferHandle,
      topLevelAccelerationStructureDeviceMemoryHandle, 0);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  VkAccelerationStructureCreateInfoKHR topLevelAccelerationStructureCreateInfo =
      {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
       .pNext = NULL,
       .createFlags = 0,
       .buffer = topLevelAccelerationStructureBufferHandle,
       .offset = 0,
       .size = topLevelAccelerationStructureBuildSizesInfo
                   .accelerationStructureSize,
       .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
       .deviceAddress = 0};

  result = pvkCreateAccelerationStructureKHR(
      logicalDevice, &topLevelAccelerationStructureCreateInfo, NULL,
      &topLevelAccelerationStructureHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTracing - vkCreateAccelerationStructureKHR");
  }

  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice, topLevelAccelerationStructureDeviceMemoryHandle,
                 NULL);

    pvkDestroyAccelerationStructureKHR(
        logicalDevice, topLevelAccelerationStructureHandle, NULL);
  });

  // =========================================================================
  // Build Top Level Acceleration Structure

  VkAccelerationStructureDeviceAddressInfoKHR
      topLevelAccelerationStructureDeviceAddressInfo = {
          .sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
          .pNext = NULL,
          .accelerationStructure = topLevelAccelerationStructureHandle};

  VkDeviceAddress topLevelAccelerationStructureDeviceAddress =
      pvkGetAccelerationStructureDeviceAddressKHR(
          logicalDevice, &topLevelAccelerationStructureDeviceAddressInfo);

  VkBufferCreateInfo topLevelAccelerationStructureScratchBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = topLevelAccelerationStructureBuildSizesInfo.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(
      logicalDevice, &topLevelAccelerationStructureScratchBufferCreateInfo,
      NULL, &topLevelAccelerationStructureScratchBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateBuffer");
  }

  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice,
                    topLevelAccelerationStructureScratchBufferHandle, NULL);
  });

  VkMemoryRequirements topLevelAccelerationStructureScratchMemoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, topLevelAccelerationStructureScratchBufferHandle,
      &topLevelAccelerationStructureScratchMemoryRequirements);

  uint32_t topLevelAccelerationStructureScratchMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {

    if ((topLevelAccelerationStructureScratchMemoryRequirements.memoryTypeBits &
         (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

      topLevelAccelerationStructureScratchMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo topLevelAccelerationStructureScratchMemoryAllocateInfo =
      {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
       .pNext = &memoryAllocateFlagsInfo,
       .allocationSize =
           topLevelAccelerationStructureScratchMemoryRequirements.size,
       .memoryTypeIndex = topLevelAccelerationStructureScratchMemoryTypeIndex};

  result = vkAllocateMemory(
      logicalDevice, &topLevelAccelerationStructureScratchMemoryAllocateInfo,
      NULL, &topLevelAccelerationStructureDeviceScratchMemoryHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkAllocateMemory");
  }

  result = vkBindBufferMemory(
      logicalDevice, topLevelAccelerationStructureScratchBufferHandle,
      topLevelAccelerationStructureDeviceScratchMemoryHandle, 0);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBindBufferMemory");
  }

  VkBufferDeviceAddressInfo
      topLevelAccelerationStructureScratchBufferDeviceAddressInfo = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
          .pNext = NULL,
          .buffer = topLevelAccelerationStructureScratchBufferHandle};

  VkDeviceAddress topLevelAccelerationStructureScratchBufferDeviceAddress =
      pvkGetBufferDeviceAddressKHR(
          logicalDevice,
          &topLevelAccelerationStructureScratchBufferDeviceAddressInfo);

  topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure =
      topLevelAccelerationStructureHandle;

  topLevelAccelerationStructureBuildGeometryInfo.scratchData = {
      .deviceAddress = topLevelAccelerationStructureScratchBufferDeviceAddress};

  VkAccelerationStructureBuildRangeInfoKHR
      topLevelAccelerationStructureBuildRangeInfo = {.primitiveCount = 1,
                                                     .primitiveOffset = 0,
                                                     .firstVertex = 0,
                                                     .transformOffset = 0};

  const VkAccelerationStructureBuildRangeInfoKHR
      *topLevelAccelerationStructureBuildRangeInfos =
          &topLevelAccelerationStructureBuildRangeInfo;

  VkCommandBufferBeginInfo topLevelCommandBufferBeginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL};

  result =
      vkBeginCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel,
                           &topLevelCommandBufferBeginInfo);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkBeginCommandBuffer");
  }

  pvkCmdBuildAccelerationStructuresKHR(
      raytracingInfo.commandBufferBuildTopAndBottomLevel, 1,
      &topLevelAccelerationStructureBuildGeometryInfo,
      &topLevelAccelerationStructureBuildRangeInfos);

  result =
      vkEndCommandBuffer(raytracingInfo.commandBufferBuildTopAndBottomLevel);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkEndCommandBuffer");
  }

  VkSubmitInfo topLevelAccelerationStructureBuildSubmitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = NULL,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .pWaitDstStageMask = NULL,
      .commandBufferCount = 1,
      .pCommandBuffers = &raytracingInfo.commandBufferBuildTopAndBottomLevel,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = NULL};

  VkFenceCreateInfo topLevelAccelerationStructureBuildFenceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};

  result = vkCreateFence(logicalDevice,
                         &topLevelAccelerationStructureBuildFenceCreateInfo,
                         NULL, &topLevelAccelerationStructureBuildFenceHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkCreateFence");
  }

  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice,
                 topLevelAccelerationStructureDeviceScratchMemoryHandle, NULL);

    vkDestroyFence(logicalDevice, topLevelAccelerationStructureBuildFenceHandle,
                   NULL);
  });

  result = vkQueueSubmit(queueHandle, 1,
                         &topLevelAccelerationStructureBuildSubmitInfo,
                         topLevelAccelerationStructureBuildFenceHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTracing - vkQueueSubmit");
  }

  result = vkWaitForFences(logicalDevice, 1,
                           &topLevelAccelerationStructureBuildFenceHandle, true,
                           UINT32_MAX);

  if (result != VK_SUCCESS && result != VK_TIMEOUT) {
    throw new std::runtime_error("initRayTracing - vkWaitForFences");
  }

  // =========================================================================
  // Uniform Buffer
  VkBufferCreateInfo uniformBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(UniformStructure),
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &uniformBufferCreateInfo, NULL,
                          &uniformBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkCreateBuffer");
  }

  deletionQueue->push_function(
      [=]() { vkDestroyBuffer(logicalDevice, uniformBufferHandle, NULL); });

  VkMemoryRequirements uniformMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, uniformBufferHandle,
                                &uniformMemoryRequirements);

  uint32_t uniformMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((uniformMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      uniformMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo uniformMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = uniformMemoryRequirements.size,
      .memoryTypeIndex = uniformMemoryTypeIndex};

  result = vkAllocateMemory(logicalDevice, &uniformMemoryAllocateInfo, NULL,
                            &uniformDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, uniformBufferHandle,
                              uniformDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkBindBufferMemory");
  }

  result = vkMapMemory(logicalDevice, uniformDeviceMemoryHandle, 0,
                       sizeof(UniformStructure), 0, &hostUniformMemoryBuffer);

  memcpy(hostUniformMemoryBuffer, &uniformStructure, sizeof(UniformStructure));

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, uniformDeviceMemoryHandle);

  deletionQueue->push_function(
      [=]() { vkFreeMemory(logicalDevice, uniformDeviceMemoryHandle, NULL); });
  // =========================================================================
  // Update Descriptor Set

  VkWriteDescriptorSetAccelerationStructureKHR
      accelerationStructureDescriptorInfo = {
          .sType =
              VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
          .pNext = NULL,
          .accelerationStructureCount = 1,
          .pAccelerationStructures = &topLevelAccelerationStructureHandle,
      };

  VkDescriptorBufferInfo uniformDescriptorInfo = {
      .buffer = uniformBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

  VkDescriptorBufferInfo indexDescriptorInfo = {
      .buffer = indexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

  VkDescriptorBufferInfo vertexDescriptorInfo = {
      .buffer = vertexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

  VkDescriptorImageInfo rayTraceImageDescriptorInfo = {
      .sampler = VK_NULL_HANDLE,
      .imageView = raytracingInfo.rayTraceImageViewHandle,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

  std::vector<VkWriteDescriptorSet> writeDescriptorSetList = {
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = &accelerationStructureDescriptorInfo,
       .dstSet = raytracingInfo.descriptorSetHandleList[0],
       .dstBinding = 0,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
       .pImageInfo = NULL,
       .pBufferInfo = NULL,
       .pTexelBufferView = NULL},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = NULL,
       .dstSet = raytracingInfo.descriptorSetHandleList[0],
       .dstBinding = 1,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .pImageInfo = NULL,
       .pBufferInfo = &uniformDescriptorInfo,
       .pTexelBufferView = NULL},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = NULL,
       .dstSet = raytracingInfo.descriptorSetHandleList[0],
       .dstBinding = 2,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .pImageInfo = NULL,
       .pBufferInfo = &indexDescriptorInfo,
       .pTexelBufferView = NULL},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = NULL,
       .dstSet = raytracingInfo.descriptorSetHandleList[0],
       .dstBinding = 3,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .pImageInfo = NULL,
       .pBufferInfo = &vertexDescriptorInfo,
       .pTexelBufferView = NULL},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = NULL,
       .dstSet = raytracingInfo.descriptorSetHandleList[0],
       .dstBinding = 4,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .pImageInfo = &rayTraceImageDescriptorInfo,
       .pBufferInfo = NULL,
       .pTexelBufferView = NULL}};

  vkUpdateDescriptorSets(logicalDevice, writeDescriptorSetList.size(),
                         writeDescriptorSetList.data(), 0, NULL);

  // =========================================================================
  // Material Index Buffer

  std::vector<uint32_t> materialIndexList;
  for (tinyobj::shape_t shape : shapes) {
    for (int index : shape.mesh.material_ids) {
      materialIndexList.push_back(index);
    }
  }

  VkBufferCreateInfo materialIndexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(uint32_t) * materialIndexList.size(),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &materialIndexBufferCreateInfo, NULL,
                          &materialIndexBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkCreateBuffer");
  }

  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice, materialIndexBufferHandle, NULL);
  });

  VkMemoryRequirements materialIndexMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, materialIndexBufferHandle,
                                &materialIndexMemoryRequirements);

  uint32_t materialIndexMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((materialIndexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      materialIndexMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo materialIndexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = materialIndexMemoryRequirements.size,
      .memoryTypeIndex = materialIndexMemoryTypeIndex};

  result = vkAllocateMemory(logicalDevice, &materialIndexMemoryAllocateInfo,
                            NULL, &materialIndexDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, materialIndexBufferHandle,
                              materialIndexDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkBindBufferMemory");
  }

  void *hostMaterialIndexMemoryBuffer;
  result = vkMapMemory(logicalDevice, materialIndexDeviceMemoryHandle, 0,
                       sizeof(uint32_t) * materialIndexList.size(), 0,
                       &hostMaterialIndexMemoryBuffer);

  memcpy(hostMaterialIndexMemoryBuffer, materialIndexList.data(),
         sizeof(uint32_t) * materialIndexList.size());

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, materialIndexDeviceMemoryHandle);
  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice, materialIndexDeviceMemoryHandle, NULL);
  });

  // =========================================================================
  // Material Buffer

  struct Material {
    float ambient[4] = {0, 0, 0, 0};
    float diffuse[4] = {0, 0, 0, 0};
    float specular[4] = {0, 0, 0, 0};
    float emission[4] = {0, 0, 0, 0};
  };

  std::vector<Material> materialList(materials.size());
  for (uint32_t x = 0; x < materials.size(); x++) {
    memcpy(materialList[x].ambient, materials[x].ambient, sizeof(float) * 3);
    memcpy(materialList[x].diffuse, materials[x].diffuse, sizeof(float) * 3);
    memcpy(materialList[x].specular, materials[x].specular, sizeof(float) * 3);
    memcpy(materialList[x].emission, materials[x].emission, sizeof(float) * 3);
  }

  VkBufferCreateInfo materialBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(Material) * materialList.size(),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &materialBufferCreateInfo, NULL,
                          &materialBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkCreateBuffer");
  }

  deletionQueue->push_function(
      [=]() { vkDestroyBuffer(logicalDevice, materialBufferHandle, NULL); });

  VkMemoryRequirements materialMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, materialBufferHandle,
                                &materialMemoryRequirements);

  uint32_t materialMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((materialMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      materialMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo materialMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = materialMemoryRequirements.size,
      .memoryTypeIndex = materialMemoryTypeIndex};

  result = vkAllocateMemory(logicalDevice, &materialMemoryAllocateInfo, NULL,
                            &materialDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, materialBufferHandle,
                              materialDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkBindBufferMemory");
  }

  void *hostMaterialMemoryBuffer;
  result = vkMapMemory(logicalDevice, materialDeviceMemoryHandle, 0,
                       sizeof(Material) * materialList.size(), 0,
                       &hostMaterialMemoryBuffer);

  memcpy(hostMaterialMemoryBuffer, materialList.data(),
         sizeof(Material) * materialList.size());

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, materialDeviceMemoryHandle);
  deletionQueue->push_function(
      [=]() { vkFreeMemory(logicalDevice, materialDeviceMemoryHandle, NULL); });

  // =========================================================================
  // Update Material Descriptor Set

  VkDescriptorBufferInfo materialIndexDescriptorInfo = {
      .buffer = materialIndexBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

  VkDescriptorBufferInfo materialDescriptorInfo = {
      .buffer = materialBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

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

  vkUpdateDescriptorSets(logicalDevice, materialWriteDescriptorSetList.size(),
                         materialWriteDescriptorSetList.data(), 0, NULL);

  // =========================================================================
  // Shader Binding Table

  VkDeviceSize progSize =
      physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;

  VkDeviceSize shaderBindingTableSize = progSize * 4;

  VkBufferCreateInfo shaderBindingTableBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = shaderBindingTableSize,
      .usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices =
          &raytracingInfo.queueFamilyIndices.presentFamily.value()};

  result = vkCreateBuffer(logicalDevice, &shaderBindingTableBufferCreateInfo,
                          NULL, &shaderBindingTableBufferHandle);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkCreateBuffer");
  }
  deletionQueue->push_function([=]() {
    vkDestroyBuffer(logicalDevice, shaderBindingTableBufferHandle, NULL);
  });

  VkMemoryRequirements shaderBindingTableMemoryRequirements;
  vkGetBufferMemoryRequirements(logicalDevice, shaderBindingTableBufferHandle,
                                &shaderBindingTableMemoryRequirements);

  uint32_t shaderBindingTableMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((shaderBindingTableMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      shaderBindingTableMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo shaderBindingTableMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = shaderBindingTableMemoryRequirements.size,
      .memoryTypeIndex = shaderBindingTableMemoryTypeIndex};

  result =
      vkAllocateMemory(logicalDevice, &shaderBindingTableMemoryAllocateInfo,
                       NULL, &shaderBindingTableDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkAllocateMemory");
  }

  result = vkBindBufferMemory(logicalDevice, shaderBindingTableBufferHandle,
                              shaderBindingTableDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkBindBufferMemory");
  }

  char *shaderHandleBuffer = new char[shaderBindingTableSize];
  result = pvkGetRayTracingShaderGroupHandlesKHR(
      logicalDevice, raytracingInfo.rayTracingPipelineHandle, 0, 4,
      shaderBindingTableSize, shaderHandleBuffer);

  if (result != VK_SUCCESS) {
    throw new std::runtime_error(
        "initRayTraci - vkGetRayTracingShaderGroupHandlesKHR");
  }

  void *hostShaderBindingTableMemoryBuffer;
  result = vkMapMemory(logicalDevice, shaderBindingTableDeviceMemoryHandle, 0,
                       shaderBindingTableSize, 0,
                       &hostShaderBindingTableMemoryBuffer);

  for (uint32_t x = 0; x < 4; x++) {
    memcpy(hostShaderBindingTableMemoryBuffer,
           shaderHandleBuffer + x * physicalDeviceRayTracingPipelineProperties
                                        .shaderGroupHandleSize,
           physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize);

    hostShaderBindingTableMemoryBuffer =
        reinterpret_cast<char *>(hostShaderBindingTableMemoryBuffer) +
        physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment;
  }

  if (result != VK_SUCCESS) {
    throw new std::runtime_error("initRayTraci - vkMapMemory");
  }

  vkUnmapMemory(logicalDevice, shaderBindingTableDeviceMemoryHandle);
  deletionQueue->push_function([=]() {
    vkFreeMemory(logicalDevice, shaderBindingTableDeviceMemoryHandle, NULL);
  });

  VkBufferDeviceAddressInfo shaderBindingTableBufferDeviceAddressInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .pNext = NULL,
      .buffer = shaderBindingTableBufferHandle};

  VkDeviceAddress shaderBindingTableBufferDeviceAddress =
      pvkGetBufferDeviceAddressKHR(logicalDevice,
                                   &shaderBindingTableBufferDeviceAddressInfo);

  VkDeviceSize hitGroupOffset = 0u * progSize;
  VkDeviceSize rayGenOffset = 1u * progSize;
  VkDeviceSize missOffset = 2u * progSize;

  raytracingInfo.rchitShaderBindingTable = {
      .deviceAddress = shaderBindingTableBufferDeviceAddress + hitGroupOffset,
      .stride = progSize,
      .size = progSize};

  raytracingInfo.rgenShaderBindingTable = {
      .deviceAddress = shaderBindingTableBufferDeviceAddress + rayGenOffset,
      .stride = progSize,
      .size = progSize};

  raytracingInfo.rmissShaderBindingTable = {
      .deviceAddress = shaderBindingTableBufferDeviceAddress + missOffset,
      .stride = progSize,
      .size = progSize * 2};

  raytracingInfo.callableShaderBindingTable = {};
}

inline void recordRaytracingCommandBuffer(
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandBuffer commandBuffer, RaytracingInfo &raytracingInfo,
    std::shared_ptr<ltracer::Window> window) {

  ltracer::buildRaytracingAccelerationStructures(logicalDevice, raytracingInfo,
                                                 commandBuffer);

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

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL,
                       1, &rayTraceGeneralMemoryBarrier);

  // =========================================================================
  // Record Render Pass Command Buffers
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    raytracingInfo.rayTracingPipelineHandle);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          raytracingInfo.pipelineLayoutHandle, 0,
                          raytracingInfo.descriptorSetHandleList.size(),
                          raytracingInfo.descriptorSetHandleList.data(), 0,
                          NULL);

  auto currentExtent = window->getSwapChainExtent();
  pvkCmdTraceRaysKHR(commandBuffer, &raytracingInfo.rgenShaderBindingTable,
                     &raytracingInfo.rmissShaderBindingTable,
                     &raytracingInfo.rchitShaderBindingTable,
                     &raytracingInfo.callableShaderBindingTable,
                     currentExtent.width, currentExtent.height, 1);

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

// does the raytracing onto the provided image
inline void raytraceImage(VkDevice logicalDevice,
                          std::shared_ptr<ltracer::Camera> camera,
                          VkImage image) {
  if (camera->isCameraMoved()) {
    uniformStructure.cameraPosition[0] = camera->transform.position[0];
    uniformStructure.cameraPosition[1] = camera->transform.position[1];
    uniformStructure.cameraPosition[2] = camera->transform.position[2];

    uniformStructure.cameraForward[0] = camera->transform.getForward()[0];
    uniformStructure.cameraForward[1] = camera->transform.getForward()[1];
    uniformStructure.cameraForward[2] = camera->transform.getForward()[2];

    uniformStructure.cameraRight[0] = camera->transform.getRight()[0];
    uniformStructure.cameraRight[1] = camera->transform.getRight()[1];
    uniformStructure.cameraRight[2] = camera->transform.getRight()[2];

    uniformStructure.cameraUp[0] = camera->transform.getUp()[0];
    uniformStructure.cameraUp[1] = camera->transform.getUp()[1];
    uniformStructure.cameraUp[2] = camera->transform.getUp()[2];

    uniformStructure.frameCount = 0;

    camera->resetCameraMoved();
  } else {
    uniformStructure.frameCount += 1;
  }

  // VkResult result =
  //     vkMapMemory(logicalDevice, uniformDeviceMemoryHandle, 0,
  //                 sizeof(UniformStructure), 0, &hostUniformMemoryBuffer);

  // memcpy(hostUniformMemoryBuffer, &uniformStructure,
  // sizeof(UniformStructure));

  // if (result != VK_SUCCESS) {
  //   throw new std::runtime_error("initRayTracing - vkMapMemory");
  // }
}

} // namespace ltracer
