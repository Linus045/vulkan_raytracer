#pragma once

#include <vulkan/vulkan_core.h>

namespace ltracer
{

// forward declarations
class Window;
class Camera;
class Renderer;
struct SwapChainSupportDetails;

struct CustomUserData
{
	bool& vulkan_initialized;
	Window& window;
	Camera& camera;
	Renderer& renderer;
	SwapChainSupportDetails& swapChainSupportDetails;

	VkDevice& logicalDevice;
	VkPhysicalDevice& physicalDevice;
};

} // namespace ltracer
