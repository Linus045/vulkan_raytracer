#pragma once

#include <map>
#include <vulkan/vulkan_core.h>

namespace ltracer
{

// forward declarations
namespace ui
{
	struct UIData;
}

class Window;
class Camera;
class Renderer;
struct SwapChainSupportDetails;

using GLFWKEY = int;
using GLFW_KEY_STATE = bool;

struct CustomUserData
{
	bool& vulkan_initialized;
	Window& window;
	Camera& camera;
	Renderer& renderer;
	ui::UIData& uiData;
	SwapChainSupportDetails& swapChainSupportDetails;

	VkDevice& logicalDevice;
	VkPhysicalDevice& physicalDevice;

	std::map<GLFWKEY, GLFW_KEY_STATE> keyStateMap;
};

} // namespace ltracer
