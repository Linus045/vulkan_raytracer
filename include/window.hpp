#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "types.hpp"
#include "vk_utils.hpp"
#include "logger.hpp"

namespace tracer
{

// manages the window creation and swapchain stuff
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

	inline int getWidth() const
	{
		auto width = 0, _height = 0;
		glfwGetWindowSize(glfwWindow, &width, &_height);
		return width;
	}

	inline int getHeight() const
	{
		auto _width = 0, height = 0;
		glfwGetWindowSize(glfwWindow, &_width, &height);
		return height;
	}

	inline int getFramebufferWidth() const
	{
		auto width = 0, _height = 0;
		glfwGetFramebufferSize(glfwWindow, &width, &_height);
		return width;
	}

	inline int getFramebufferHeight() const
	{
		auto _width = 0, height = 0;
		glfwGetFramebufferSize(glfwWindow, &_width, &height);
		return height;
	}

	/// Returns the GLFW window
	inline GLFWwindow*& getGLFWWindow()
	{
		return glfwWindow;
	}

	/// Returns the vulkan surface
	inline VkSurfaceKHR& getVkSurface()
	{
		return vulkanSurface;
	}

	inline VkFormat getSwapChainImageFormat() const
	{
		return swapChainImageFormat;
	}

	inline VkExtent2D getSwapChainExtent() const
	{
		return swapChainExtent;
	}

	inline auto& getSwapChain()
	{
		return swapChain;
	}

	inline auto& getMouseCursorCaptureEnabled() const
	{
		return cursorCaptureEnabled;
	}

	/// Creates the window using GLFW
	inline void initWindow(GLFWframebuffersizefun framebufferResizeCallback)
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

	inline void setPreferredSize(int width, int height)
	{
		preferredWidth = width;
		preferredHeight = height;
		checkPreferredSizeChange = true;
	}
	// since we have to call glfwSetWindowSize() from the main thread, we can't simply change the
	// size in the button callback
	inline void checkPreferredWindowSize()
	{
		if (checkPreferredSizeChange)
		{
			checkPreferredSizeChange = false;
			// std::printf("Preferred Size: %dx%d, Window Size: %dx%d, Framebuffer Size %dx%d\n",
			//             preferredWidth,
			//             preferredHeight,
			//             getWidth(),
			//             getHeight(),
			//             getFramebufferWidth(),
			//             getFramebufferHeight());
			if (getFramebufferWidth() != preferredWidth
			    || getFramebufferHeight() != preferredHeight)
			{
				glfwSetWindowSize(glfwWindow, preferredWidth, preferredHeight);
				// glfwSetWindowMonitor(
				//     glfwWindow, nullptr, 0, 0, preferredWidth, preferredHeight, GLFW_DONT_CARE);
			}
		}
	}

	inline void setMouseCursorCapturedEnabled(const bool captureMouseCursor)
	{
		cursorCaptureEnabled = captureMouseCursor;
		if (cursorCaptureEnabled)
		{
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	inline void setWindowUserPointer(void* ptr)
	{
		glfwSetWindowUserPointer(glfwWindow, ptr);
	}

	/// Returns whether the window should close (e.g. user pressed the close
	/// button)
	inline bool shouldClose() const
	{
		return glfwWindowShouldClose(glfwWindow);
	}

	inline void createVulkanSurface(VkInstance vulkanInstance)
	{
		if (glfwCreateWindowSurface(vulkanInstance, glfwWindow, nullptr, &vulkanSurface)
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface");
		}
	}

	inline void getFramebufferSize(int* width, int* height) const
	{
		glfwGetFramebufferSize(glfwWindow, width, height);
	}

	inline VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

	inline void createSwapChain(VkPhysicalDevice physicalDevice,
	                            VkDevice logicalDevice,
	                            VkExtent2D swapExtent,
	                            SwapChainSupportDetails swapChainSupport)
	{

		VkSurfaceFormatKHR surfaceFormat
		    = chooseSwapSurfaceFormat(physicalDevice, swapChainSupport.formats);
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
		createInfo.imageUsage
		    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.minImageCount = imageCount;

		tracer::QueueFamilyIndices indices = findQueueFamilies(physicalDevice, vulkanSurface);
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

	inline void recreateSwapChain(VkPhysicalDevice& physicalDevice,
	                              VkDevice& logicalDevice,
	                              VkExtent2D& swapExtent,
	                              tracer::SwapChainSupportDetails& swapChainSupport)
	{

		cleanupSwapChain(logicalDevice);
		createSwapChain(physicalDevice, logicalDevice, swapExtent, swapChainSupport);
	}

	inline void cleanupSwapChain(VkDevice logicalDevice)
	{
		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	inline VkSurfaceFormatKHR
	chooseSwapSurfaceFormat(VkPhysicalDevice physicalDevice,
	                        const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// try to find the good SRGB format with 8-bit RGB colors
		for (const auto& availableFormat : availableFormats)
		{
			VkPhysicalDeviceImageFormatInfo2 formatInfo = {};
			formatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
			formatInfo.format = availableFormat.format;
			formatInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			                   | VK_IMAGE_USAGE_STORAGE_BIT;
			formatInfo.type = VK_IMAGE_TYPE_2D;
			formatInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			formatInfo.flags = 0;

			VkImageFormatProperties2 properties{};
			properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;

			VkResult result = vkGetPhysicalDeviceImageFormatProperties2(
			    physicalDevice, &formatInfo, &properties);

			if (result != VK_SUCCESS)
			{
				debug_printFmt("Format unavailable - format: "
				               "%s with color space: %s\n",
				               string_VkFormat(availableFormat.format),
				               string_VkColorSpaceKHR(availableFormat.colorSpace));
				continue;
			}

			if (result == VK_SUCCESS && availableFormat.format == VK_FORMAT_B8G8R8_SRGB
			    && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				debug_printFmt("SwapSurface Format - using format: "
				               "%s with color space: %s\n",
				               string_VkFormat(availableFormat.format),
				               string_VkColorSpaceKHR(availableFormat.colorSpace));
				return availableFormat;
			}

			if (result == VK_SUCCESS && availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			    && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				debug_printFmt("SwapSurface Format - using format: "
				               "%s with color space: %s\n",
				               string_VkFormat(availableFormat.format),
				               string_VkColorSpaceKHR(availableFormat.colorSpace));
				return availableFormat;
			}
		}

		for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
		{
			VkPhysicalDeviceImageFormatInfo2 formatInfo = {};
			formatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
			formatInfo.format = availableFormat.format;
			formatInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			                   | VK_IMAGE_USAGE_STORAGE_BIT;
			formatInfo.type = VK_IMAGE_TYPE_2D;
			formatInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			formatInfo.flags = 0;

			VkImageFormatProperties2 properties{};
			properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;

			VkResult result = vkGetPhysicalDeviceImageFormatProperties2(
			    physicalDevice, &formatInfo, &properties);

			if (result == VK_SUCCESS)
			{
				debug_printFmt(
				    "SwapSurface Format - Choosing first valid format: %s with color space: %s\n",
				    string_VkFormat(availableFormat.format),
				    string_VkColorSpaceKHR(availableFormat.colorSpace));

				// if the good format cannot be found, just use the first one
				return availableFormat;
			}
		}

		throw std::runtime_error("failed to find suitable surface format!");
	}

	inline VkPresentModeKHR
	chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{

		// see
		// https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html?language=en#_Toc445674479
		// Possible modes:
		// does not wait for vsync
		// VK_PRESENT_MODE_IMMEDIATE_KHR

		// waits for vsync
		// VK_PRESENT_MODE_MAILBOX_KHR
		// VK_PRESENT_MODE_FIFO_KHR
		// VK_PRESENT_MODE_FIFO_RELAXED_KHR

		// prefer mailbox
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				debug_print("SwapPresentMode - using mailbox mode\n");
				return availablePresentMode;
			}
		}

		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				debug_print("SwapPresentMode - using immediate mode\n");
				return availablePresentMode;
			}
		}

		// fallback to fifo (always available)
		debug_print("SwapPresentMode - using fifo mode\n");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

  private:
	const bool FULLSCREEN_ENABLED = false;

	uint32_t initialWidth = 1920;
	uint32_t initialHeight = 1080;

	int preferredWidth = static_cast<int>(initialWidth);
	int preferredHeight = static_cast<int>(initialHeight);
	bool checkPreferredSizeChange = false;

	// window
	GLFWwindow* glfwWindow = nullptr;

	VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = {0, 0};

	bool cursorCaptureEnabled = false;
};

} // namespace tracer
