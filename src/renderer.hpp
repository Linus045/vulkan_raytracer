#pragma once

#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"

#include "src/camera.hpp"
#include "src/deletion_queue.hpp"
#include "src/raytracing.hpp"
#include "src/types.hpp"

#include "src/ui.hpp"
#include "src/window.hpp"
#include "src/worldobject.hpp"

namespace ltracer {

class Renderer {
public:
  bool swapChainOutdated = true;

  Renderer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
           std::shared_ptr<ltracer::DeletionQueue> deletionQueue,
           std::shared_ptr<ltracer::Window> window, VkQueue graphicsQueue,
           VkQueue presentQueue, VkQueue transferQueue,
           const ltracer::ui::UIData &uiData)
      : physicalDevice(physicalDevice), logicalDevice(logicalDevice),
        deletionQueue(deletionQueue), window(window),
        graphicsQueue(graphicsQueue), presentQueue(presentQueue),
        transferQueue(transferQueue), uiData(uiData) //
  {
    //
  }

  ~Renderer() = default;

  Renderer(const Renderer &) = delete;
  Renderer(const Renderer &&) = delete;
  Renderer &operator=(const Renderer &) = delete;

  void cleanupFramebufferAndImageViews() {
    for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(logicalDevice, imageView, nullptr);
    }
  }

  void cleanupRenderer() {
    ltracer::rt::freeRaytraceImageAndImageView(
        logicalDevice, raytracingInfo.rayTraceImageHandle,
        raytracingInfo.rayTraceImageViewHandle);
    cleanupFramebufferAndImageViews();
  }

  /// Initializes the renderer and creates the necessary Vulkan objects
  void initRenderer(VkInstance &vulkanInstance,
                    std::shared_ptr<std::vector<WorldObject>> worldObjects,
                    std::shared_ptr<ltracer::Camera> camera,
                    const uint32_t width, const uint32_t height,
                    ltracer::SwapChainSupportDetails swapChainSupport) {
    // this->worldObjects = worldObjects;

    createImageViews(logicalDevice);

    createRenderPass();
    // createDescriptorSetLayoutGlobal();
    // createDescriptorSetLayoutModel();
    createGraphicsPipeline();
    createFramebuffers(logicalDevice, window);
    createCommandPools();

    // createUniformBuffers();
    // createDescriptorPool();
    // createDescriptorSetsGlobal();
    // createDescriptorSetsModels();
    createCommandBuffers();
    createSyncObjects();

    // for (ltracer::MeshObject &obj : *worldObjects) {
    //   createVertexBuffer(obj);
    //   createIndexBuffer(obj);
    // }

    // grab the required queue families
    ltracer::QueueFamilyIndices queueFamilyIndices =
        ltracer::findQueueFamilies(physicalDevice, window->getVkSurface());

    ltracer::ui::initImgui(vulkanInstance, logicalDevice, physicalDevice,
                           window, queueFamilyIndices, renderPass,
                           deletionQueue);

    ltracer::rt::createRaytracingImage(
        physicalDevice, logicalDevice, window->getSwapChainImageFormat(),
        window->getSwapChainExtent(), queueFamilyIndices,
        raytracingInfo.rayTraceImageHandle);

    ltracer::rt::createRaytracingImageView(
        logicalDevice, window->getSwapChainImageFormat(),
        raytracingInfo.rayTraceImageHandle,
        raytracingInfo.rayTraceImageViewHandle);

    raytracingInfo.queueFamilyIndices = queueFamilyIndices;

    ltracer::rt::initRayTracing(physicalDevice, logicalDevice, deletionQueue,
                                window, swapChainImageViews, raytracingInfo);
    auto &cameraTransform = camera->transform;
    // ltracer::updateUniformStructure(
    //     cameraTransform.position, cameraTransform.getRight(),
    //     cameraTransform.getUp(), cameraTransform.getForward());
  }

  void recreateRaytracingImageAndImageView(
      const ltracer::QueueFamilyIndices &queueFamilyIndices) {
    ltracer::rt::recreateRaytracingImageBuffer(logicalDevice, physicalDevice,
                                               window, queueFamilyIndices,
                                               raytracingInfo);
  }

  std::vector<VkImageView> &getSwapChainImageViews() {
    return swapChainImageViews;
  }

  std::vector<VkFramebuffer> &getSwapChainFramebuffers() {
    return swapChainFramebuffers;
  }

  void createImageViews(VkDevice logicalDevice) {
    // retrieve images from the swapchain so we can access them
    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(logicalDevice, window->getSwapChain(),
                            &swapChainImageCount, nullptr);
    swapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(logicalDevice, window->getSwapChain(),
                            &swapChainImageCount, swapChainImages.data());

    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = window->getSwapChainImageFormat();

      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(logicalDevice, &createInfo, nullptr,
                            &swapChainImageViews[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain image views!");
      }
    }
  }

  void createFramebuffers(VkDevice logicalDevice,
                          std::shared_ptr<Window> window) {
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
      VkImageView attachments[] = {swapChainImageViews[i]};

      VkFramebufferCreateInfo framebufferInfo{
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = renderPass,
          .attachmentCount = 1,
          .pAttachments = attachments,
          .width = window->getSwapChainExtent().width,
          .height = window->getSwapChainExtent().height,
          .layers = 1,
      };

      if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr,
                              &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer");
      }
    }
  }

  void renderImguiFrame(const ltracer::ui::UIData &uiData) {
    ltracer::ui::beginFrame();
    ltracer::ui::renderMainPanel(uiData);
    ltracer::ui::endFrame();
  }

  void updateUniformBuffer(uint32_t currentImage) {
    // for (ltracer::WorldObject &obj : *worldObjects) {
    //   ltracer::UniformBufferObject ubo{};
    //   ubo.modelMatrix = obj.getModelMatrix();
    //   // logMat4("MODEL MATRIX for Frame:" + std::to_string(currentImage) +
    //   //             " and object:" + std::to_string(objectIdx),
    //   //         ubo.modelMatrix);

    //   memcpy(obj.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    // }
  }

  /// Draws the frame and updates the surface
  void drawFrame(std::shared_ptr<Camera> camera, float delta) {

    vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        logicalDevice, window->getSwapChain(), UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      swapChainOutdated = true;
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image");
    }

    vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

    // TODO: figure out if this reset for the command buffer is necessary
    // vkDeviceWaitIdle(logicalDevice);
    // vkResetCommandBuffer(commandBuffers[currentFrame],
    // VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    // (*worldObjects)[0].translate(0, -1.1 * delta, 0);
    // worldObjects[0].translate(0, 0, 0.8 * delta);
    // worldObjects[1].translate(0.8 * delta, 0, 0);

    ltracer::rt::updateRaytraceBuffer(logicalDevice, camera,
                                      raytracingInfo.rayTraceImageHandle);
    updateUniformBuffer(currentFrame);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_TRANSFER_BIT,
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult test = vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                                  inFlightFences[currentFrame]);
    if (test != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapChains[] = {window->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      swapChainOutdated = true;
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void updateViewProjectionMatrix(const glm::mat4 view, const glm::mat4 proj) {
    viewMatrix = view;
    projectionMatrix = proj;
    updateSharedInfoBuffer();
  }

  const RaytracingInfo getRaytracingInfo() { return raytracingInfo; }

private:
  // How many frames can be recorded at the same time
  const int MAX_FRAMES_IN_FLIGHT = 3;

  std::shared_ptr<DeletionQueue> deletionQueue;
  RaytracingInfo raytracingInfo = {};

  std::shared_ptr<ltracer::Window> window;

  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;

  const ui::UIData &uiData;

  // physical device handle
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  // Logical device to interact with
  VkDevice logicalDevice;

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  VkQueue transferQueue = VK_NULL_HANDLE;

  uint32_t currentFrame = 0;

  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkImage> swapChainImages;

  std::vector<VkFence> inFlightFences;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;

  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;

  // VkCommandPool commandPoolTransfer = VK_NULL_HANDLE;
  // VkDescriptorPool descriptorPool;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  // std::shared_ptr<std::vector<ltracer::WorldObject>> worldObjects;

  void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

      if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr,
                            &imageAvailableSemaphores[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores");
      }
      if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr,
                            &renderFinishedSemaphores[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores");
      }
      if (vkCreateFence(logicalDevice, &fenceInfo, nullptr,
                        &inFlightFences[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence");
      }
    }

    deletionQueue->push_function([=, this]() {
      for (size_t i = 0; i < imageAvailableSemaphores.size(); i++)
        vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
    });

    deletionQueue->push_function([=, this]() {
      for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
        vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
    });

    deletionQueue->push_function([=, this]() {
      for (size_t i = 0; i < inFlightFences.size(); i++)
        vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
    });
  }

  void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }

    deletionQueue->push_function([=, this]() {
      vkFreeCommandBuffers(logicalDevice, commandPool,
                           static_cast<uint32_t>(commandBuffers.size()),
                           commandBuffers.data());
    });
  }

  void createCommandPools() {
    ltracer::QueueFamilyIndices queueFamilyIndices =
        findQueueFamilies(physicalDevice, window->getVkSurface());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }

    deletionQueue->push_function([=, this]() {
      vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    });

    // VkCommandPoolCreateInfo poolInfoTransfer{};
    // poolInfoTransfer.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // poolInfoTransfer.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    // poolInfoTransfer.queueFamilyIndex =
    //     queueFamilyIndices.transferFamily.value();

    // if (vkCreateCommandPool(logicalDevice, &poolInfoTransfer, nullptr,
    //                         &commandPoolTransfer) != VK_SUCCESS) {
    //   throw std::runtime_error("failed to create command pool for
    //   transfer!");
    // }
  }

  VkShaderModule createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr,
                             &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
  }

  void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageCreateInfo,
        fragShaderStageCreateInfo,
    };

    auto bindingDescription = ltracer::Vertex::getBindingDescription();
    auto attributeDescriptions = ltracer::Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount =
            0, // static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions =
            VK_NULL_HANDLE, // attributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    // std::vector<VkDescriptorSetLayout> layouts = {descriptorSetLayoutGlobal,
    //                                               descriptorSetLayoutPerModel};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        // .setLayoutCount = 2,
        // .pSetLayouts = layouts.data(),
        .setLayoutCount = 0,
        .pSetLayouts = VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create a pipeline layout!");
    }

    deletionQueue->push_function([=, this]() {
      vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
    });

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline");
    }

    deletionQueue->push_function([=, this]() {
      vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    });

    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
  }

  void createRenderPass() {
    VkAttachmentDescription colorAttachment{
        .format = window->getSwapChainImageFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr,
                           &renderPass) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
    }

    deletionQueue->push_function([=, this]() {
      vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    });
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    ltracer::QueueFamilyIndices queueFamilyIndices =
        findQueueFamilies(physicalDevice, window->getVkSurface());

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer");
    }

    auto swapChainExtent = window->getSwapChainExtent();

    ltracer::rt::recordRaytracingCommandBuffer(
        physicalDevice, logicalDevice, commandBuffer, raytracingInfo, window);

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    VkImageMemoryBarrier2 barrierSwapChainImage = {};
    barrierSwapChainImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrierSwapChainImage.srcAccessMask =
        VK_ACCESS_TRANSFER_READ_BIT; // No prior access
    barrierSwapChainImage.dstAccessMask = 0;
    barrierSwapChainImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrierSwapChainImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierSwapChainImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierSwapChainImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierSwapChainImage.image = swapChainImages[imageIndex];
    barrierSwapChainImage.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    barrierSwapChainImage.subresourceRange.baseMipLevel = 0;
    barrierSwapChainImage.subresourceRange.levelCount = 1;
    barrierSwapChainImage.subresourceRange.baseArrayLayer = 0;
    barrierSwapChainImage.subresourceRange.layerCount = 1;
    barrierSwapChainImage.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrierSwapChainImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkDependencyInfo dependencyInfoSwapChainImage = {};
    dependencyInfoSwapChainImage.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfoSwapChainImage.dependencyFlags = 0;
    dependencyInfoSwapChainImage.imageMemoryBarrierCount = 1;
    dependencyInfoSwapChainImage.pImageMemoryBarriers = &barrierSwapChainImage;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoSwapChainImage);

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // VkImageMemoryBarrier2 barrierRaytraceImage = {};
    // barrierRaytraceImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    // barrierRaytraceImage.srcAccessMask =
    //     VK_ACCESS_TRANSFER_READ_BIT; // No prior access
    // barrierRaytraceImage.dstAccessMask = 0;
    // barrierRaytraceImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // barrierRaytraceImage.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    // barrierRaytraceImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrierRaytraceImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    // barrierRaytraceImage.image = rayTraceImageHandle;
    // barrierRaytraceImage.subresourceRange.aspectMask =
    //     VK_IMAGE_ASPECT_COLOR_BIT;
    // barrierRaytraceImage.subresourceRange.baseMipLevel = 0;
    // barrierRaytraceImage.subresourceRange.levelCount = 1;
    // barrierRaytraceImage.subresourceRange.baseArrayLayer = 0;
    // barrierRaytraceImage.subresourceRange.layerCount = 1;
    // barrierRaytraceImage.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    // barrierRaytraceImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    // VkDependencyInfo dependencyInfoRaytraceImage = {};
    // dependencyInfoRaytraceImage.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    // dependencyInfoRaytraceImage.dependencyFlags = 0;
    // dependencyInfoRaytraceImage.imageMemoryBarrierCount = 1;
    // dependencyInfoRaytraceImage.pImageMemoryBarriers = &barrierRaytraceImage;

    // vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoRaytraceImage);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    VkImageCopy2 region = {};
    region.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = {0, 0, 0}; // Source start

    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = {0, 0, 0}; // Destination start
    region.extent = {
        static_cast<uint32_t>(window->getSwapChainExtent().width),
        static_cast<uint32_t>(window->getSwapChainExtent().height),
        1,
    };

    VkCopyImageInfo2 copyImageInfo = {};
    copyImageInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
    copyImageInfo.srcImage = raytracingInfo.rayTraceImageHandle;
    copyImageInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    copyImageInfo.dstImage = swapChainImages[imageIndex];
    copyImageInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyImageInfo.pNext = NULL;
    copyImageInfo.regionCount = 1;
    copyImageInfo.pRegions = &region;

    vkCmdCopyImage2(commandBuffer, &copyImageInfo);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    VkImageMemoryBarrier2 barrierRaytraceImageRevert = {};
    barrierRaytraceImageRevert.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrierRaytraceImageRevert.srcAccessMask = 0; // No prior access
    barrierRaytraceImageRevert.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrierRaytraceImageRevert.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierRaytraceImageRevert.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrierRaytraceImageRevert.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierRaytraceImageRevert.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierRaytraceImageRevert.image = raytracingInfo.rayTraceImageHandle;
    barrierRaytraceImageRevert.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    barrierRaytraceImageRevert.subresourceRange.baseMipLevel = 0;
    barrierRaytraceImageRevert.subresourceRange.levelCount = 1;
    barrierRaytraceImageRevert.subresourceRange.baseArrayLayer = 0;
    barrierRaytraceImageRevert.subresourceRange.layerCount = 1;
    barrierRaytraceImageRevert.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrierRaytraceImageRevert.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkDependencyInfo dependencyInfoRaytraceImageRevert = {};
    dependencyInfoRaytraceImageRevert.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfoRaytraceImageRevert.dependencyFlags = 0;
    dependencyInfoRaytraceImageRevert.imageMemoryBarrierCount = 1;
    dependencyInfoRaytraceImageRevert.pImageMemoryBarriers =
        &barrierRaytraceImageRevert;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoRaytraceImageRevert);

    VkImageMemoryBarrier2 barrierSwapChainImagePresent = {};
    barrierSwapChainImagePresent.sType =
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrierSwapChainImagePresent.srcAccessMask =
        VK_ACCESS_TRANSFER_WRITE_BIT; // No prior access
    barrierSwapChainImagePresent.dstAccessMask = 0;
    barrierSwapChainImagePresent.oldLayout =
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierSwapChainImagePresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrierSwapChainImagePresent.srcQueueFamilyIndex =
        queueFamilyIndices.presentFamily.value();
    barrierSwapChainImagePresent.dstQueueFamilyIndex =
        queueFamilyIndices.presentFamily.value();
    barrierSwapChainImagePresent.image = swapChainImages[imageIndex];
    barrierSwapChainImagePresent.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    barrierSwapChainImagePresent.subresourceRange.baseMipLevel = 0;
    barrierSwapChainImagePresent.subresourceRange.levelCount = 1;
    barrierSwapChainImagePresent.subresourceRange.baseArrayLayer = 0;
    barrierSwapChainImagePresent.subresourceRange.layerCount = 1;
    barrierSwapChainImagePresent.srcStageMask =
        VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrierSwapChainImagePresent.dstStageMask =
        VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkDependencyInfo dependencyInfoSwapChainImagePresent = {};
    dependencyInfoSwapChainImagePresent.sType =
        VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfoSwapChainImagePresent.dependencyFlags = 0;
    dependencyInfoSwapChainImagePresent.imageMemoryBarrierCount = 1;
    dependencyInfoSwapChainImagePresent.pImageMemoryBarriers =
        &barrierSwapChainImagePresent;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoSwapChainImagePresent);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    // TODO: don't clear image
    VkClearValue clearColor = {{{0.6f, 0.1f, 0.1f, 1.0f}}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1; // 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = -static_cast<float>(swapChainExtent.height);
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    renderImguiFrame(uiData);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer");
    }
  }

  void updateSharedInfoBuffer() {
    // ltracer::SharedInfo sharedInfo;
    // sharedInfo.view = viewMatrix;
    // sharedInfo.proj = projectionMatrix;
    // memcpy(sharedInfoBufferMemoryMapped, &sharedInfo,
    //        sizeof(ltracer::SharedInfo));
  }
};

} // namespace ltracer
