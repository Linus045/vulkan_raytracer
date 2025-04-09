#pragma once

#include <cassert>
#include <cstring>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"

#include "camera.hpp"
#include "deletion_queue.hpp"
#include "raytracing.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "window.hpp"

namespace tracer
{

class Renderer
{
  public:
	bool swapChainOutdated = true;

	Renderer(VkPhysicalDevice physicalDevice,
	         VkDevice logicalDevice,
	         tracer::DeletionQueue& deletionQueue,
	         tracer::Window& window,
	         VkQueue graphicsQueue,
	         VkQueue presentQueue,
	         VkQueue transferQueue,
	         const bool raytracingSupported)
	    : deletionQueue(deletionQueue), window(window), physicalDevice(physicalDevice),
	      raytracingSupported(raytracingSupported), logicalDevice(logicalDevice),
	      graphicsQueue(graphicsQueue), presentQueue(presentQueue), transferQueue(transferQueue) //
	{
	}

	~Renderer() = default;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void initRenderer(VkInstance& vulkanInstance);

	inline void cleanupFramebufferAndImageViews()
	{
		for (auto framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
		}

		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}
	}

	inline void cleanupRenderer()
	{
		// if (raytracingSupported)
		{
			raytracingScene->cleanup(raytracingInfo.graphicsQueueHandle);

			tracer::freeRaytraceImageAndImageView(logicalDevice,
			                                      raytracingInfo.rayTraceImageHandle,
			                                      raytracingInfo.rayTraceImageViewHandle,
			                                      raytracingInfo.rayTraceImageDeviceMemoryHandle);
		}

		cleanupFramebufferAndImageViews();

		vkDestroyFramebuffer(logicalDevice, raytracingFramebuffer, nullptr);
	}

	/// Initializes the renderer and creates the necessary Vulkan objects
	inline void recreateRaytracingImageAndImageView()
	{
		if (raytracingSupported)
		{
			tracer::recreateRaytracingImageBuffer(
			    physicalDevice, logicalDevice, window.getSwapChainExtent(), raytracingInfo);
		}
	}

	inline std::vector<VkImageView>& getSwapChainImageViews()
	{
		return swapChainImageViews;
	}

	inline std::vector<VkFramebuffer>& getSwapChainFramebuffers()
	{
		return swapChainFramebuffers;
	}

	inline const uint32_t& getFrameCount() const
	{
		return raytracingInfo.uniformStructure.frameCount;
	}

	inline const size_t& getBLASInstancesCount() const
	{
		return raytracingScene->getBLASInstancesCount();
	}

	void createImageViews();

	void createFramebuffers();

	void createRaytracingRenderpassAndFramebuffer();

	inline void renderImguiFrame(const VkCommandBuffer commandBuffer, tracer::ui::UIData& uiData)
	{
		tracer::ui::beginFrame();
		tracer::ui::renderMainPanel(uiData);
		tracer::ui::renderCrosshair(uiData);
		tracer::ui::endFrame();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	void updateUniformBuffer([[maybe_unused]] uint32_t currentImage)
	{
		// for (tracer::WorldObject &obj : *worldObjects) {
		//   tracer::UniformBufferObject ubo{};
		//   ubo.modelMatrix = obj.getModelMatrix();
		//   // logMat4("MODEL MATRIX for Frame:" + std::to_string(currentImage) +
		//   //             " and object:" + std::to_string(objectIdx),
		//   //         ubo.modelMatrix);

		//   memcpy(obj.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
		// }
		raytracingInfo.uniformStructure.viewProj = projectionMatrix * viewMatrix;
		raytracingInfo.uniformStructure.viewInverse = glm::inverse(viewMatrix);
		raytracingInfo.uniformStructure.projInverse = glm::inverse(projectionMatrix);
	}

	inline void requestResetFrameCount()
	{
		resetFrameCountRequested = true;
	}

	/// Draws the frame and updates the surface
	void drawFrame(Camera& camera, [[maybe_unused]] double delta, ui::UIData& uiData);

	inline void updateViewProjectionMatrix(const glm::mat4 view, const glm::mat4 proj)
	{
		viewMatrix = view;
		projectionMatrix = proj;
		updateSharedInfoBuffer();
	}

	inline const RaytracingInfo& getRaytracingInfo() const
	{
		return raytracingInfo;
	}

	inline RaytracingInfo& getRaytracingInfo()
	{
		return raytracingInfo;
	}

	inline const RaytracingDataConstants& getRaytracingDataConstants() const
	{
		return raytracingInfo.raytracingConstants;
	}

	inline RaytracingDataConstants& getRaytracingDataConstants()
	{
		return raytracingInfo.raytracingConstants;
	}

	inline const rt::RaytracingScene& getRaytracingScene() const
	{
		return *raytracingScene;
	}

	inline rt::RaytracingScene& getRaytracingScene()
	{
		return *raytracingScene;
	}

  private:
	void createSyncObjects();

	void createCommandBuffers();

	void createCommandPools();

	VkShaderModule createShaderModule(const std::vector<char>& code);

	void createGraphicsPipeline();

	void createRenderPass();

	void
	recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, ui::UIData& uiData);

	inline void updateSharedInfoBuffer()
	{
		// tracer::SharedInfo sharedInfo;
		// sharedInfo.view = viewMatrix;
		// sharedInfo.proj = projectionMatrix;
		// memcpy(sharedInfoBufferMemoryMapped, &sharedInfo,
		//        sizeof(tracer::SharedInfo));
	}

  private:
	// How many frames can be recorded at the same time
	const int MAX_FRAMES_IN_FLIGHT = 3;

	DeletionQueue& deletionQueue;
	RaytracingInfo raytracingInfo = {};

	Window& window;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	// physical device handle
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	const bool raytracingSupported = false;

	std::unique_ptr<rt::RaytracingScene> raytracingScene;

	// Logical device to interact with
	VkDevice logicalDevice;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	// TODO: maybe utilize this queue for transfer operations
	[[maybe_unused]] VkQueue transferQueue = VK_NULL_HANDLE;

	bool resetFrameCountRequested = false;
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

	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	VkRenderPass raytracingRenderPass = VK_NULL_HANDLE;
	VkFramebuffer raytracingFramebuffer = VK_NULL_HANDLE;

	// format used for raytracing image
	VkFormat raytracingImageColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	// std::shared_ptr<std::vector<tracer::WorldObject>> worldObjects;
};

} // namespace tracer
