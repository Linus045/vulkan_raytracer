#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "renderer.hpp"
#include "raytracing.hpp"
#include "model.hpp"

const uint64_t TIMEOUT_SECONDS_5 = 5ll * 1000ll * 1000ll * 1000ll;

namespace ltracer
{

void Renderer::initRenderer(VkInstance& vulkanInstance)
{
	// this->worldObjects = worldObjects;

	createImageViews();

	createRenderPass();
	// createDescriptorSetLayoutGlobal();
	// createDescriptorSetLayoutModel();
	createGraphicsPipeline();
	createFramebuffers();
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
	ltracer::QueueFamilyIndices queueFamilyIndices
	    = ltracer::findQueueFamilies(physicalDevice, window.getVkSurface());

	ltracer::ui::initImgui(vulkanInstance,
	                       logicalDevice,
	                       physicalDevice,
	                       window,
	                       renderPass,
	                       graphicsQueue,
	                       deletionQueue);

	if (raytracingSupported)
	{
		ltracer::rt::createRaytracingImage(physicalDevice,
		                                   logicalDevice,
		                                   window.getSwapChainImageFormat(),
		                                   window.getSwapChainExtent(),
		                                   raytracingInfo);

		raytracingInfo.rayTraceImageViewHandle = ltracer::rt::createRaytracingImageView(
		    logicalDevice, window.getSwapChainImageFormat(), raytracingInfo.rayTraceImageHandle);

		raytracingInfo.queueFamilyIndices = queueFamilyIndices;

		raytracingScene = std::make_unique<rt::RaytracingScene>(physicalDevice, logicalDevice);

		ltracer::rt::initRayTracing(
		    physicalDevice, logicalDevice, deletionQueue, raytracingInfo, *raytracingScene);
	}

	raytracingInfo.raytracingConstants = {
	    .newtonErrorXTolerance = 1e-8f,
	    .newtonErrorFTolerance = 1e-3f,

	    .newtonErrorFIgnoreIncrease = 1.0f,
	    .newtonErrorFHitBelowTolerance = 1.0f,
	    .newtonErrorXIgnoreIncrease = 1.0f,
	    .newtonErrorXHitBelowTolerance = 0.0f,

	    .newtonMaxIterations = 10,
	    .someFloatingScalar = 1.00f,
	    .someScalar = -1,
	    .globalLightPosition = glm::vec3(7.0f, 4.0f, 2.0f),
	    .globalLightColor = glm::vec3(0.0, 0.8, 0.8),
	    .globalLightIntensity = 1.0f,
	    // .environmentColor = vec3(0.58, 0.81, 0.92),
	    //.environmentColor = vec3(0.10, 0.10, 0.20),
	    .environmentColor = vec3(1.0, 1.0, 1.0),
	    .environmentLightIntensity = 0.3f,
	    .debugShowAABBs = 0.0f,
	    .renderSideTriangle = 1.0f,
	    .renderSide1 = 1.0f,
	    .renderSide2 = 0.0f,
	    .renderSide3 = 0.0f,
	    .renderSide4 = 0.0f,
	    .recursiveRaysPerPixel = 1,
	    .debugPrintCrosshairRay = 1.0f,
	    .debugSlicingPlanes = 0.0f,
	    .enableSlicingPlanes = 0.0f,
	    .debugShowSubdivisions = 0.0f,
	    .cameraDir = glm::vec3(0),
	};

	// auto& cameraTransform = camera->transform;
	// ltracer::updateUniformStructure(
	//     cameraTransform.position, cameraTransform.getRight(),
	//     cameraTransform.getUp(), cameraTransform.getForward());
}

void Renderer::createImageViews()
{
	// retrieve images from the swapchain so we can access them
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(logicalDevice, window.getSwapChain(), &swapChainImageCount, nullptr);
	swapChainImages.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(
	    logicalDevice, window.getSwapChain(), &swapChainImageCount, swapChainImages.data());

	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = window.getSwapChainImageFormat();

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i])
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain image views!");
		}
	}
}

void Renderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {swapChainImageViews[i]};

		VkFramebufferCreateInfo framebufferInfo{
		    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .pNext = NULL,
		    .flags = 0,
		    .renderPass = renderPass,
		    .attachmentCount = 1,
		    .pAttachments = attachments,
		    .width = window.getSwapChainExtent().width,
		    .height = window.getSwapChainExtent().height,
		    .layers = 1,
		};

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i])
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer");
		}
	}
}

void Renderer::drawFrame(Camera& camera, [[maybe_unused]] double delta, ui::UIData& uiData)
{
	VkResult result = vkWaitForFences(
	    logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, TIMEOUT_SECONDS_5);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::drawFrame - failed to wait for fence");
	}
	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(logicalDevice,
	                               window.getSwapChain(),
	                               TIMEOUT_SECONDS_5,
	                               imageAvailableSemaphores[currentFrame],
	                               VK_NULL_HANDLE,
	                               &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		swapChainOutdated = true;
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image");
	}

	result = vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::drawFrame - failed to reset command buffer");
	}

	if (raytracingSupported)
	{
		if (uiData.recreateAccelerationStructures.isRecreateNeeded())
		{
			bool fullRebuild = uiData.recreateAccelerationStructures.isFullRebuildNeeded();
			if (fullRebuild)
			{
				// TODO: replace this with a fence to improve performance
				vkQueueWaitIdle(raytracingInfo.graphicsQueueHandle);
			}
			else
			{
				raytracingScene->copyObjectsToBuffers();
			}

			raytracingScene->recreateAccelerationStructures(raytracingInfo, fullRebuild);

			rt::updateAccelerationStructureDescriptorSet(
			    logicalDevice, *raytracingScene, raytracingInfo);

			uiData.recreateAccelerationStructures.reset();
			resetFrameCountRequested = true;
		}

		ltracer::rt::updateRaytraceBuffer(logicalDevice, raytracingInfo, resetFrameCountRequested);
		resetFrameCountRequested = false;
	}
	updateUniformBuffer(currentFrame);

	uiData.raytracingDataConstants.cameraDir = camera.transform.getForward();

	// TODO: create a setUIData() and retrieveUIData() function that sets/loads these values
	// correspondingly
	//
	// TODO: abstract this away so we just need get the reference and set the position
	auto instanceIdx = raytracingScene->getWorldObjectSpheres()[0].getInstanceIndex();
	if (instanceIdx)
	{
		// we assume the first sphere always represents the light
		raytracingScene->getWorldObjectSpheres()[0].setPosition(
		    uiData.raytracingDataConstants.globalLightPosition);
		auto transformMatrix
		    = raytracingScene->getWorldObjectSpheres()[0].getTransform().getTransformMatrix();
		raytracingScene->setTransformMatrixForInstance(instanceIdx.value(), transformMatrix);
	}

	recordCommandBuffer(commandBuffers[currentFrame], imageIndex, uiData);

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

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::drawFrame - failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	VkSwapchainKHR swapChains[] = {window.getSwapChain()};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapChainOutdated = true;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::drawFrame - failed to present swap chain image");
	}

	currentFrame = (currentFrame + 1) % static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
}

void Renderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT));
	renderFinishedSemaphores.resize(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT));
	inFlightFences.resize(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT));

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
	{

		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i])
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores");
		}
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i])
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores");
		}
		if (vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create fence");
		}
	}

	deletionQueue.push_function(
	    [=, this]()
	    {
		    for (size_t i = 0; i < imageAvailableSemaphores.size(); i++)
			    vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
	    });

	deletionQueue.push_function(
	    [=, this]()
	    {
		    for (size_t i = 0; i < renderFinishedSemaphores.size(); i++)
			    vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
	    });

	deletionQueue.push_function(
	    [=, this]()
	    {
		    for (size_t i = 0; i < inFlightFences.size(); i++)
			    vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	    });
}

void Renderer::createCommandBuffers()
{
	commandBuffers.resize(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT));

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	deletionQueue.push_function(
	    [=, this]()
	    {
		    vkFreeCommandBuffers(logicalDevice,
		                         commandPool,
		                         static_cast<uint32_t>(commandBuffers.size()),
		                         commandBuffers.data());
	    });
}

void Renderer::createCommandPools()
{
	ltracer::QueueFamilyIndices queueFamilyIndices
	    = findQueueFamilies(physicalDevice, window.getVkSurface());

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}

	deletionQueue.push_function([=, this]()
	                            { vkDestroyCommandPool(logicalDevice, commandPool, nullptr); });

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

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	return shaderModule;
}

void Renderer::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_VERTEX_BIT,
	    .module = vertShaderModule,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	    .module = fragShaderModule,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {
	    vertShaderStageCreateInfo,
	    fragShaderStageCreateInfo,
	};

	auto bindingDescription = ltracer::Vertex::getBindingDescription();
	// auto attributeDescriptions = ltracer::Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .vertexBindingDescriptionCount = 1,
	    .pVertexBindingDescriptions = &bindingDescription,
	    // TODO: figure out if this is neeeded? probably not for ray tracing
	    .vertexAttributeDescriptionCount
	    = 0, // static_cast<uint32_t>(attributeDescriptions.size()),
	    .pVertexAttributeDescriptions = VK_NULL_HANDLE, // attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .primitiveRestartEnable = VK_FALSE,
	};

	std::vector<VkDynamicState> dynamicStates = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamicState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
	    .pDynamicStates = dynamicStates.data(),
	};

	VkPipelineViewportStateCreateInfo viewportState{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .viewportCount = 1,
	    // viewport and scissors are dynamic, this pointer is ignored
	    .pViewports = NULL,
	    .scissorCount = 1,
	    // viewport and scissors are dynamic, this pointer is ignored
	    .pScissors = NULL,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
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
	    .pNext = NULL,
	    .flags = 0,
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
	    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
	                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlending{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
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
	    .pNext = NULL,
	    .flags = 0,
	    // .setLayoutCount = 2,
	    // .pSetLayouts = layouts.data(),
	    .setLayoutCount = 0,
	    .pSetLayouts = VK_NULL_HANDLE,
	    .pushConstantRangeCount = 0,
	    .pPushConstantRanges = nullptr,
	};

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout)
	    != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a pipeline layout!");
	}

	deletionQueue.push_function(
	    [=, this]() { vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr); });

	VkGraphicsPipelineCreateInfo pipelineInfo{
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stageCount = 2,
	    .pStages = shaderStages,
	    .pVertexInputState = &vertexInputInfo,
	    .pInputAssemblyState = &inputAssembly,
	    .pTessellationState = NULL,
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

	if (vkCreateGraphicsPipelines(
	        logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)
	    != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline");
	}

	deletionQueue.push_function([=, this]()
	                            { vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr); });

	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
}

void Renderer::createRenderPass()
{
	VkAttachmentDescription colorAttachment{
	    .flags = 0,
	    .format = window.getSwapChainImageFormat(),
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
	    .flags = 0,
	    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	    .inputAttachmentCount = 0,
	    .pInputAttachments = NULL,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachmentRef,
	    .pResolveAttachments = NULL,
	    .pDepthStencilAttachment = NULL,
	    .preserveAttachmentCount = 0,
	    .pPreserveAttachments = NULL,

	};

	VkSubpassDependency dependency{
	    .srcSubpass = VK_SUBPASS_EXTERNAL,
	    .dstSubpass = 0,
	    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .srcAccessMask = 0,
	    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	    .dependencyFlags = 0,
	};

	VkRenderPassCreateInfo renderPassInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .attachmentCount = 1,
	    .pAttachments = &colorAttachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	    .dependencyCount = 1,
	    .pDependencies = &dependency,
	};

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}

	deletionQueue.push_function([=, this]()
	                            { vkDestroyRenderPass(logicalDevice, renderPass, nullptr); });
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   ui::UIData& uiData)
{

	ltracer::QueueFamilyIndices queueFamilyIndices
	    = findQueueFamilies(physicalDevice, window.getVkSurface());

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(
		    "Renderer::recordCommandBuffer - failed to begin recording command buffer");
	}

	// Ensure the output image is in the right layout for clearing (transfer source layout)
	VkImageMemoryBarrier2 barrierClearSwapChainImage = {};
	barrierClearSwapChainImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrierClearSwapChainImage.srcAccessMask = 0; // No prior access
	barrierClearSwapChainImage.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrierClearSwapChainImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrierClearSwapChainImage.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrierClearSwapChainImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierClearSwapChainImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrierClearSwapChainImage.image = swapChainImages[imageIndex];
	barrierClearSwapChainImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrierClearSwapChainImage.subresourceRange.baseMipLevel = 0;
	barrierClearSwapChainImage.subresourceRange.levelCount = 1;
	barrierClearSwapChainImage.subresourceRange.baseArrayLayer = 0;
	barrierClearSwapChainImage.subresourceRange.layerCount = 1;
	barrierClearSwapChainImage.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	barrierClearSwapChainImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

	VkDependencyInfo dependencyClearSwapChainImage = {};
	dependencyClearSwapChainImage.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyClearSwapChainImage.dependencyFlags = 0;
	dependencyClearSwapChainImage.imageMemoryBarrierCount = 1;
	dependencyClearSwapChainImage.pImageMemoryBarriers = &barrierClearSwapChainImage;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyClearSwapChainImage);

	VkClearColorValue clearColor = {{0.6f, 0.1f, 0.1f, 1.0f}};
	vkCmdClearColorImage(commandBuffer,
	                     swapChainImages[imageIndex],
	                     VK_IMAGE_LAYOUT_GENERAL,
	                     &clearColor,
	                     1,
	                     &barrierClearSwapChainImage.subresourceRange);

	if (raytracingSupported)
	{
		ltracer::rt::recordRaytracingCommandBuffer(
		    commandBuffer, window.getSwapChainExtent(), raytracingInfo);

		//////////////////////////////////////////////////////////////////////////////////////////////////////

		VkImageMemoryBarrier2 barrierSwapChainImage = {};
		barrierSwapChainImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrierSwapChainImage.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // No prior access
		barrierSwapChainImage.dstAccessMask = 0;
		barrierSwapChainImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrierSwapChainImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierSwapChainImage.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrierSwapChainImage.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrierSwapChainImage.image = swapChainImages[imageIndex];
		barrierSwapChainImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
		    static_cast<uint32_t>(window.getSwapChainExtent().width),
		    static_cast<uint32_t>(window.getSwapChainExtent().height),
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
		barrierRaytraceImageRevert.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
		dependencyInfoRaytraceImageRevert.pImageMemoryBarriers = &barrierRaytraceImageRevert;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoRaytraceImageRevert);

		VkImageMemoryBarrier2 barrierSwapChainImagePresent = {};
		barrierSwapChainImagePresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrierSwapChainImagePresent.srcAccessMask
		    = VK_ACCESS_TRANSFER_WRITE_BIT; // No prior access
		barrierSwapChainImagePresent.dstAccessMask = 0;
		barrierSwapChainImagePresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierSwapChainImagePresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrierSwapChainImagePresent.srcQueueFamilyIndex = queueFamilyIndices.presentFamily.value();
		barrierSwapChainImagePresent.dstQueueFamilyIndex = queueFamilyIndices.presentFamily.value();
		barrierSwapChainImagePresent.image = swapChainImages[imageIndex];
		barrierSwapChainImagePresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrierSwapChainImagePresent.subresourceRange.baseMipLevel = 0;
		barrierSwapChainImagePresent.subresourceRange.levelCount = 1;
		barrierSwapChainImagePresent.subresourceRange.baseArrayLayer = 0;
		barrierSwapChainImagePresent.subresourceRange.layerCount = 1;
		barrierSwapChainImagePresent.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrierSwapChainImagePresent.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		VkDependencyInfo dependencyInfoSwapChainImagePresent = {};
		dependencyInfoSwapChainImagePresent.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfoSwapChainImagePresent.dependencyFlags = 0;
		dependencyInfoSwapChainImagePresent.imageMemoryBarrierCount = 1;
		dependencyInfoSwapChainImagePresent.pImageMemoryBarriers = &barrierSwapChainImagePresent;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoSwapChainImagePresent);
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	auto swapChainExtent = window.getSwapChainExtent();
	// TODO: don't clear image

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 0; // 1;
	renderPassInfo.pClearValues = NULL;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

	renderImguiFrame(commandBuffer, uiData);

	vkCmdEndRenderPass(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::recordCommandBuffer - failed to record command buffer");
	}
}
}; // namespace ltracer
