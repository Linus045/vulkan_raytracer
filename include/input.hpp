#pragma once

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <imgui.h>

#include "window.hpp"

namespace ltracer
{

enum class KeyListeningMode
{
	UI_ONLY,             // only trigger when UI is active i.e. mouse is not captured
	FLYING_CAMERA,       // only trigger when camera is in flying mode i.e. mouse is captured
	UI_AND_FLYING_CAMERA // trigger in both modes
};

// trigger on keyup, keydown or repeat
enum class KeyTriggerMode
{
	Repeat,  // trigger every frame the key is pressed/hold down
	KeyDown, // trigger only once when the key is pressed
	KeyUp    // trigger only once when the key is released
};

struct KeyListener
{
	// trigger on keyup, keydown or repeat
	KeyTriggerMode triggerMode;

	// trigger when UI is active, flying camera is active or both
	KeyListeningMode listeningMode;
	std::function<void()> callback;
};

using GLFWKEY = int;
using GLFW_KEY_STATE = bool;

struct CustomUserData;

void handleMouseInputCallback(GLFWwindow* window, int button, int action, int mods);

void handleMouseMovementCallback(GLFWwindow* window, double xpos, double ypos);

void updateMovement(CustomUserData& userData, double delta);

void registerKeyListener(Window& window,
                         GLFWKEY key,
                         KeyTriggerMode triggerMode,
                         KeyListeningMode listeningMode,
                         std::function<void()> callback);

void handleInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void handleMouseScrollCallback(GLFWwindow* window, [[maybe_unused]] double xOffset, double yOffset);

} // namespace ltracer
