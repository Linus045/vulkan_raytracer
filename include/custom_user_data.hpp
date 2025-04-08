#pragma once

#include <map>
#include <vulkan/vulkan_core.h>

#include "input.hpp"

namespace tracer
{

// forward declarations
namespace ui
{
struct UIData;
} // namespace ui

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
	ui::UIData& uiData;
	SwapChainSupportDetails& swapChainSupportDetails;

	VkDevice& logicalDevice;
	VkPhysicalDevice& physicalDevice;

	// current key state for each key
	std::map<GLFWKEY, GLFW_KEY_STATE> keyStateMap;

	// key callbacks
	std::map<GLFWKEY, KeyListener> keyListeners;

	float lastMouseX = 0.0;
	float lastMouseY = 0.0;
	// used to prevent weird cursor movement issues when switching cursor input mode, see
	// handleMouseMovementCallback in input.cpp
	bool ignoreFirstMouseMovement = false;
};

} // namespace tracer
