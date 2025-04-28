#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "renderer.hpp"
#include "raytracing.hpp"
#include "model.hpp"

namespace tracer
{

void Renderer::initRenderer(VkInstance& vulkanInstance)
{
	// this->worldObjects = worldObjects;

	// grab the required queue families
	tracer::QueueFamilyIndices queueFamilyIndices
	    = tracer::findQueueFamilies(physicalDevice, window.getVkSurface());
	raytracingInfo.queueFamilyIndices = queueFamilyIndices;

	descriptorPool = createDescriptorPool();

	raytracingImageDescriptorSetLayoutHandle
	    = createRaytracingImageDescriptorSetLayout(logicalDevice, deletionQueue);

	std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList = {
	    raytracingImageDescriptorSetLayoutHandle,
	};

	descriptorSetHandleList = allocateDescriptorSetLayouts(
	    logicalDevice, descriptorPool, descriptorSetLayoutHandleList);

	createImageViews();

	createRenderPass();
	// createDescriptorSetLayoutGlobal();
	// createDescriptorSetLayoutModel();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPools();

	// create sampler
	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.maxLod = VK_LOD_CLAMP_NONE;

	VK_CHECK_RESULT(vkCreateSampler(logicalDevice, &createInfo, nullptr, &raytraceImageSampler));
	raytracingInfo.raytraceImageSampler = raytraceImageSampler;

	// createUniformBuffers();

	// createDescriptorSetsGlobal();
	// createDescriptorSetsModels();
	createCommandBuffers();
	createSyncObjects();

	// for (tracer::MeshObject &obj : *worldObjects) {
	//   createVertexBuffer(obj);
	//   createIndexBuffer(obj);
	// }

	tracer::ui::initImgui(vulkanInstance,
	                      logicalDevice,
	                      physicalDevice,
	                      window,
	                      renderPass,
	                      graphicsQueue,
	                      deletionQueue);

	// grab Graphics Queue
	if (raytracingInfo.queueFamilyIndices.graphicsFamily.has_value())
	{
		vkGetDeviceQueue(logicalDevice,
		                 raytracingInfo.queueFamilyIndices.graphicsFamily.value(),
		                 0,
		                 &raytracingInfo.graphicsQueueHandle);
	}
	else
	{
		throw std::runtime_error("initRenderer - no valid graphicsFamily index");
	}

	tracer::createRaytracingImage(
	    physicalDevice, logicalDevice, vmaAllocator, window.getSwapChainExtent(), raytracingInfo);

	raytracingInfo.rayTraceImageViewHandle
	    = tracer::createRaytracingImageView(logicalDevice, raytracingInfo.rayTraceImageHandle);

	createRaytracingRenderpassAndFramebuffer();
	updateRaytracingDescriptorSet();

	raytracingScene
	    = std::make_unique<rt::RaytracingScene>(physicalDevice, logicalDevice, vmaAllocator);

	if (raytracingSupported)
	{
		tracer::rt::initRayTracing(
		    physicalDevice, logicalDevice, vmaAllocator, deletionQueue, raytracingInfo);
	}

	raytracingInfo.raytracingConstants = {
	    .newtonErrorXTolerance = 1e-8f,
	    .newtonErrorFTolerance = 1e-4f,
	    .newtonErrorFIgnoreIncrease = 1.0f,
	    .newtonErrorFHitBelowTolerance = 1.0f,
	    .newtonMaxIterations = 10,
	    .newtonGuessesAmount = 6,
	    .globalLightPosition = glm::vec3(5.0f, 8.0f, 5.0f),
	    .globalLightColor = glm::vec3(1.0, 1.0, 1.0),
	    .globalLightIntensity = 0.5f,
	    .environmentColor = vec3(0.58, 0.81, 0.92),
	    // .environmentColor = vec3(0.10, 0.10, 0.20),
	    // .environmentColor = vec3(0.2, 0.2, 0.2),
	    .environmentLightIntensity = 0.7f,
	    .debugShowAABBs = 0.0f,
	    .renderSideTriangle = 1.0f,
	    .recursiveRaysPerPixel = 1,
	    .debugPrintCrosshairRay = 0.0f,
	    .debugSlicingPlanes = 0.0f,
	    .enableSlicingPlanes = 0.0f,
	    .renderShadows = 1.0f,
	    .debugHighlightObjectEdges = 1.0f,
	    .debugFastRenderMode = 0.0f,
	    .debugVisualizeControlPoints = 0.0f,
	    .debugVisualizeSampledSurface = 0.0f,
	    .debugVisualizeSampledVolume = 0.0f,
	    .cameraDir = glm::vec3(0),
	};

	// auto& cameraTransform = camera->transform;
	// tracer::updateUniformStructure(
	//     cameraTransform.position, cameraTransform.getRight(),
	//     cameraTransform.getUp(), cameraTransform.getForward());
}

void Renderer::updateRaytracingDescriptorSet()
{
	VkDescriptorImageInfo blitRayTraceImageDescriptorInfo = {
	    .sampler = raytracingInfo.raytraceImageSampler,
	    .imageView = raytracingInfo.rayTraceImageViewHandle,
	    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	std::vector<VkWriteDescriptorSet> writeDescriptorSetList{
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .pNext = NULL,
	        .dstSet = descriptorSetHandleList[0],
	        .dstBinding = 0,
	        .dstArrayElement = 0,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .pImageInfo = &blitRayTraceImageDescriptorInfo,
	        .pBufferInfo = NULL,
	        .pTexelBufferView = NULL,
	    },
	};

	vkUpdateDescriptorSets(logicalDevice,
	                       static_cast<uint32_t>(writeDescriptorSetList.size()),
	                       writeDescriptorSetList.data(),
	                       0,
	                       NULL);
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

VkDescriptorPool Renderer::createDescriptorPool()
{
	VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
	    {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1},
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = NULL,
	    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	    .maxSets = 1,
	    .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizeList.size()),
	    .pPoolSizes = descriptorPoolSizeList.data(),
	};

	VK_CHECK_RESULT(vkCreateDescriptorPool(
	    logicalDevice, &descriptorPoolCreateInfo, NULL, &descriptorPoolHandle));

	deletionQueue.push_function(
	    [=, this]() { vkDestroyDescriptorPool(logicalDevice, descriptorPoolHandle, NULL); });

	return descriptorPoolHandle;
}

void Renderer::drawFrame(Camera& camera,
                         [[maybe_unused]] double delta,
                         [[maybe_unused]] ui::UIData& uiData)
{
	VkResult result
	    = vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINTMAX_MAX);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Renderer::drawFrame - failed to wait for fence");
	}
	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(logicalDevice,
	                               window.getSwapChain(),
	                               UINTMAX_MAX,
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

	VK_CHECK_RESULT(vkResetCommandBuffer(commandBuffers[currentFrame], 0));

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
				raytracingScene->copyGPUObjectsToBuffers();
			}

			// make sure the light position is up-to-date
			{
				// TODO: create a setUIData() and retrieveUIData() function that sets/loads these
				// values correspondingly

				// TODO: abstract this away so we just need get the reference and set the position
				auto light = raytracingScene->getSceneObject("light");
				if (light.has_value())
				{
					if (light.value()->spheres.size() > 0)
					{
						auto& lightSphere = light.value()->spheres[0];
						// we assume the first sphere always represents the light
						lightSphere->setPosition(
						    uiData.raytracingDataConstants.globalLightPosition);
						std::printf("LightSphere Pos: %f %f %f\n",
						            lightSphere->getTransform().getX(),
						            lightSphere->getTransform().getY(),
						            lightSphere->getTransform().getZ());
						auto transformMatrix = lightSphere->getTransform().getTransformMatrix();

						light.value()->setTransformMatrix(transformMatrix);
						std::printf("Setting light position for instanceCustomIndex: %d\n",
						            light.value()->instanceCustomIndex);
						raytracingScene->setTransformMatrixForInstance(
						    light.value()->instanceCustomIndex + 0, transformMatrix);
					}
				}
			}

			raytracingScene->recreateAccelerationStructures(raytracingInfo, fullRebuild);

			updateAccelerationStructureDescriptorSet(
			    logicalDevice, *raytracingScene, raytracingInfo);

			uiData.recreateAccelerationStructures.reset();
			resetFrameCountRequested = true;
		}

		tracer::updateRaytraceBuffer(
		    logicalDevice, vmaAllocator, raytracingInfo, resetFrameCountRequested);
		resetFrameCountRequested = false;
	}

	updateUniformBuffer(currentFrame);

	uiData.raytracingDataConstants.cameraDir = camera.transform.getForward();

	// if (raytracingSupported)
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex, uiData);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

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
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value();

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
	// auto fragShaderCode = readFile("shaders/frag.spv");
	auto blitfragShaderCode = readFile("shaders/frag_blit.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	// VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
	VkShaderModule blitfragShaderModule = createShaderModule(blitfragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_VERTEX_BIT,
	    .module = vertShaderModule,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	// VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{
	//     .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	//     .pNext = NULL,
	//     .flags = 0,
	//     .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	//     .module = fragShaderModule,
	//     .pName = "main",
	//     .pSpecializationInfo = NULL,
	// };

	VkPipelineShaderStageCreateInfo blitfragShaderStageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	    .module = blitfragShaderModule,
	    .pName = "main",
	    .pSpecializationInfo = NULL,
	};

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
	    vertShaderStageCreateInfo,
	    // fragShaderStageCreateInfo,
	    blitfragShaderStageCreateInfo,
	};

	// auto bindingDescription = tracer::Vertex::getBindingDescription();
	// auto attributeDescriptions = tracer::Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = NULL;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = VK_NULL_HANDLE;

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
	    .cullMode = VK_CULL_MODE_FRONT_BIT,
	    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

	std::vector<VkDescriptorSetLayout> layouts = {
	    raytracingImageDescriptorSetLayoutHandle,
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = NULL,
	    .flags = 0,
	    .setLayoutCount = static_cast<uint32_t>(layouts.size()),
	    .pSetLayouts = layouts.data(),
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

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = NULL;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = NULL;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	// pipelineInfo.subpass = 0;
	// pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	// pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
	        logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline)
	    != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline");
	}

	deletionQueue.push_function([=, this]()
	                            { vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr); });

	// vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, blitfragShaderModule, nullptr);
}

void Renderer::createRenderPass()
{
	VkAttachmentDescription colorAttachment{
	    .flags = 0,
	    .format = window.getSwapChainImageFormat(),
	    .samples = VK_SAMPLE_COUNT_1_BIT,
	    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
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

	VK_CHECK_RESULT(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass));

	deletionQueue.push_function([=, this]()
	                            { vkDestroyRenderPass(logicalDevice, renderPass, nullptr); });
}

void Renderer::createRaytracingRenderpassAndFramebuffer()
{
	// // Setting the image layout for both color and depth
	{
		VkCommandPool commandPool1 = VK_NULL_HANDLE;
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.queueFamilyIndex = raytracingInfo.queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool1) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}

		VkCommandBuffer commandBuffer1 = VK_NULL_HANDLE;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer1) != VK_SUCCESS)
		{
			throw std::runtime_error("Renderer::createRaytracingRenderpassAndFramebuffer - failed "
			                         "to allocate command buffers!");
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = 0;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer1, &beginInfo));

		// addImageMemoryBarrier(commandBuffer,
		//                       VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		//                       VK_ACCESS_2_NONE,
		//                       VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		//                       VK_ACCESS_TRANSFER_READ_BIT,
		//                       VK_IMAGE_LAYOUT_UNDEFINED,
		//                       VK_IMAGE_LAYOUT_GENERAL,
		//                       subresourceRange,
		//                       raytracingInfo.rayTraceImageHandle);

		auto subresourceRange = VkImageSubresourceRange{
		    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		    .baseMipLevel = 0,
		    .levelCount = 1,
		    .baseArrayLayer = 0,
		    .layerCount = 1,
		};
		addImageMemoryBarrier(commandBuffer1,
		                      VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		                      VK_ACCESS_2_NONE,
		                      VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		                      VK_ACCESS_2_TRANSFER_WRITE_BIT,
		                      VK_IMAGE_LAYOUT_UNDEFINED,
		                      VK_IMAGE_LAYOUT_GENERAL,
		                      subresourceRange,
		                      raytracingInfo.rayTraceImageHandle);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer1));

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &commandBuffer1;

		VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submit, nullptr));

		VK_CHECK_RESULT(vkQueueWaitIdle(graphicsQueue));

		vkFreeCommandBuffers(logicalDevice, commandPool1, 1, &commandBuffer1);
		vkDestroyCommandPool(logicalDevice, commandPool1, nullptr);
	}

	// Creating a renderpass for the offscreen
	if (raytracingRenderPass == VK_NULL_HANDLE)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = raytracingImageColorFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = VK_NULL_HANDLE;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(
		    logicalDevice, &renderPassCreateInfo, nullptr, &raytracingRenderPass));
	}

	// Creating the frame buffer for offscreen
	std::vector<VkImageView> attachments = {};

	// destroy old framebuffer
	vkDestroyFramebuffer(logicalDevice, raytracingFramebuffer, nullptr);

	// create new framebuffer
	auto extent = window.getSwapChainExtent();
	VkFramebufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = raytracingRenderPass;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &raytracingInfo.rayTraceImageViewHandle;
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	createInfo.layers = 1;
	VK_CHECK_RESULT(
	    vkCreateFramebuffer(logicalDevice, &createInfo, nullptr, &raytracingFramebuffer));
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                   uint32_t imageIndex,
                                   [[maybe_unused]] ui::UIData& uiData)
{
	auto swapChainExtent = window.getSwapChainExtent();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	auto subresourceRange = VkImageSubresourceRange{
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = 1,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	VkClearColorValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
	VkClearValue clearValue{};
	clearValue.color = clearColor;

	if (raytracingSupported)
	{
		tracer::rt::recordRaytracingCommandBuffer(commandBuffer, swapChainExtent, raytracingInfo);
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValue;

	// Ensure the image is using the correct layout
	addImageMemoryBarrier(commandBuffer,
	                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                      0,
	                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                      VK_IMAGE_LAYOUT_UNDEFINED,
	                      VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	                      subresourceRange,
	                      swapChainImages[imageIndex]);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.x = 0.0f;
	viewport.y = 1;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// write raytracing image to swapchain image
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	vkCmdBindDescriptorSets(commandBuffer,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        pipelineLayout,
	                        0,
	                        static_cast<uint32_t>(descriptorSetHandleList.size()),
	                        descriptorSetHandleList.data(),
	                        0,
	                        NULL);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	renderImguiFrame(commandBuffer, uiData);

	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
}
}; // namespace tracer
