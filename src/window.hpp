#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "src/types.hpp"

namespace ltracer
{

class Window
{
  public:
	Window()
	{
		//
	}

	Window(uint32_t width, uint32_t height) : initialWidth(width), initialHeight(height)
	{
		//
	}

	~Window() = default;
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) noexcept = delete;
	Window& operator=(Window&&) noexcept = delete;

	/// Returns the GLFW window
	GLFWwindow*& getGLFWWindow()
	{
		return glfwWindow;
	}

	/// Returns the vulkan surface
	VkSurfaceKHR& getVkSurface()
	{
		return vulkanSurface;
	}

	VkFormat getSwapChainImageFormat() const
	{
		return swapChainImageFormat;
	}

	VkExtent2D getSwapChainExtent() const
	{
		return swapChainExtent;
	}

	auto& getSwapChain()
	{
		return swapChain;
	}

	auto& getMouseCursorCaptureEnabled() const
	{
		return cursorCaptureEnabled;
	}

	/// Creates the window using GLFW
	void initWindow(GLFWframebuffersizefun framebufferResizeCallback)
	{
		// init glfw
		glfwInit();
		// Don't create OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (FULLSCREEN_ENABLED)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

			int window_width = mode->width;
			int window_height = mode->height;
			glfwWindow = glfwCreateWindow(
			    window_width, window_height, "Vulkan", glfwGetPrimaryMonitor(), nullptr);
		}
		else
		{
			// Disable resizing the window for now
			glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

			glfwWindow = glfwCreateWindow(static_cast<int>(initialWidth),
			                              static_cast<int>(initialHeight),
			                              "Vulkan",
			                              nullptr,
			                              nullptr);
		}
		glfwSetFramebufferSizeCallback(glfwWindow, framebufferResizeCallback);

		setMouseCursorCapturedEnabled(cursorCaptureEnabled);
	}

	void setMouseCursorCapturedEnabled(const bool cursorCaptureEnabled)
	{
		this->cursorCaptureEnabled = cursorCaptureEnabled;
		if (this->cursorCaptureEnabled)
		{
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	void setWindowUserPointer(void* ptr)
	{
		glfwSetWindowUserPointer(glfwWindow, ptr);
	}

	/// Returns whether the window should close (e.g. user pressed the close
	/// button)
	bool shouldClose() const
	{
		return glfwWindowShouldClose(glfwWindow);
	}

	void createVulkanSurface(VkInstance vulkanInstance)
	{
		if (glfwCreateWindowSurface(vulkanInstance, glfwWindow, nullptr, &vulkanSurface)
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface");
		}
	}

	void getFramebufferSize(int* width, int* height) const
	{
		glfwGetFramebufferSize(glfwWindow, width, height);
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// uint32_t max has a special meaning, it tells us to set the correct
		// resolution
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width = 0, height = 0;
			getFramebufferSize(&width, &height);
			VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
			actualExtent.width = std::clamp(actualExtent.width,
			                                capabilities.minImageExtent.width,
			                                capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height,
			                                 capabilities.minImageExtent.height,
			                                 capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void createSwapChain(VkPhysicalDevice physicalDevice,
	                     VkDevice logicalDevice,
	                     VkExtent2D swapExtent,
	                     SwapChainSupportDetails swapChainSupport)
	{

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);

		// debug_print("Minimum image count required for swap chain: %d\n",
		//             swapChainSupport.capabilities.minImageCount);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		// debug_print("Using image count for swap chain: %d\n", imageCount);

		// maxImageCount of 0 means there is no limit
		if (swapChainSupport.capabilities.maxImageCount > 0
		    && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
			std::cerr << "Exceeded max count for swap chain. Setting to max limit "
			             "instead: "
			          << imageCount << '\n';
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = vulkanSurface;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.minImageCount = imageCount;

		ltracer::QueueFamilyIndices indices = findQueueFamilies(physicalDevice, vulkanSurface);
		uint32_t queueFamilyIndices[] = {
		    indices.graphicsFamily.value(),
		    indices.presentFamily.value(),
		};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// store these values for later use
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = swapExtent;
	}

	void recreateSwapChain(VkPhysicalDevice& physicalDevice,
	                       VkDevice& logicalDevice,
	                       VkExtent2D& swapExtent,
	                       ltracer::SwapChainSupportDetails& swapChainSupport)
	{

		cleanupSwapChain(logicalDevice);
		createSwapChain(physicalDevice, logicalDevice, swapExtent, swapChainSupport);
	}

	void cleanupSwapChain(VkDevice logicalDevice)
	{
		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	VkSurfaceFormatKHR
	chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{

		// try to find the good SRGB format with 8-bit RGB colors
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB
			    && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		// if the good format cannot be found, just use the first one
		return availableFormats[0];
	}

	VkPresentModeKHR
	chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{

		// Possible modes:
		// does not wait for vsync
		// VK_PRESENT_MODE_IMMEDIATE_KHR

		// wait for vsync
		// VK_PRESENT_MODE_MAILBOX_KHR
		// VK_PRESENT_MODE_FIFO_KHR
		// VK_PRESENT_MODE_FIFO_RELAXED_KHR
		for (const auto& availablePresentMode : availablePresentModes)
		{
			// TODO: This is only for testing, please use the correct mode
			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) return availablePresentMode;

			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

  private:
	const bool FULLSCREEN_ENABLED = false;

	uint32_t initialWidth = 800;
	uint32_t initialHeight = 600;

	// window
	GLFWwindow* glfwWindow = nullptr;

	VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

#ifdef NDEBUG
	bool cursorCaptureEnabled = true;
#else
	bool cursorCaptureEnabled = false;
#endif
};

} // namespace ltracer
