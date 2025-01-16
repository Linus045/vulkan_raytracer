#pragma once

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "backends/imgui_impl_glfw.h"

#include "src/custom_user_data.hpp"
#include "src/camera.hpp"
#include "src/window.hpp"
#include "src/renderer.hpp"

namespace ltracer
{

inline void handleMouseInputCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

inline void handleMouseMovementCallback(GLFWwindow* window, double xpos, double ypos)
{
	CustomUserData& userData = *reinterpret_cast<CustomUserData*>(glfwGetWindowUserPointer(window));

	if (userData.window.getMouseCursorCaptureEnabled())
	{
		// TODO: move these into a more fitting space
		static auto lastMouseX = 0.0;
		static auto lastMouseY = 0.0;
		double mouse_sensitivity = 0.1;

		double deltaX = xpos - lastMouseX;
		double deltaY = ypos - lastMouseY;

		userData.camera.rotateYawY(static_cast<float>(-deltaX * mouse_sensitivity));
		userData.camera.rotatePitchX(static_cast<float>(-deltaY * mouse_sensitivity));

		lastMouseX = xpos;
		lastMouseY = ypos;
	}
	else
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
	}
}

inline void handleInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	CustomUserData& userData = *reinterpret_cast<CustomUserData*>(glfwGetWindowUserPointer(window));

	// TODO: maybe use an array to store which key is held down so we can handle multiple keys
	// at the same time e.g. update on key down and up event respectively
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		if (key == GLFW_KEY_W)
		{
			userData.camera.translate(userData.camera.transform.getForward() * 0.3f);
		}
		else if (key == GLFW_KEY_S)
		{
			userData.camera.translate(userData.camera.transform.getForward() * -0.3f);
		}
		else if (key == GLFW_KEY_A)
		{
			userData.camera.translate(userData.camera.transform.getRight() * -0.3f);
		}
		else if (key == GLFW_KEY_D)
		{
			userData.camera.translate(userData.camera.transform.getRight() * 0.3f);
		}
		else if (key == GLFW_KEY_SPACE)
		{
			userData.camera.translate(userData.camera.globalUp * 0.3f);
		}
		else if (key == GLFW_KEY_LEFT_SHIFT)
		{
			userData.camera.translate(userData.camera.globalUp * -0.3f);
		}
		else if (key == GLFW_KEY_LEFT)
		{
			userData.camera.rotateYawY(10.0f);
		}
		else if (key == GLFW_KEY_RIGHT)
		{
			userData.camera.rotateYawY(-10.0f);
		}
		else if (key == GLFW_KEY_UP)
		{
			userData.camera.rotatePitchX(10.0f);
		}
		else if (key == GLFW_KEY_DOWN)
		{
			userData.camera.rotatePitchX(-10.0f);
		}
		else if (key == GLFW_KEY_Q)
		{
			glfwSetWindowShouldClose(userData.window.getGLFWWindow(), true);
		}
		else if (key == GLFW_KEY_G || key == GLFW_KEY_ESCAPE)
		{
			userData.window.setMouseCursorCapturedEnabled(
			    !userData.window.getMouseCursorCaptureEnabled());
			std::cout << "Cursor capture enabled: "
			          << (userData.window.getMouseCursorCaptureEnabled() ? "True" : "False")
			          << std::endl;
		}
		else
		{
			ImGui_ImplGlfw_KeyCallback(
			    userData.window.getGLFWWindow(), key, scancode, action, mods);
		}

		// TODO: Add mouse rotation via a virtual trackball e.g.
		// https://computergraphics.stackexchange.com/questions/151/how-to-implement-a-trackball-in-opengl
	}
}

inline void framebufferResizeCallback(GLFWwindow* window,
                                      [[maybe_unused]] int width,
                                      [[maybe_unused]] int height)
{

	ltracer::CustomUserData& userData
	    = *reinterpret_cast<ltracer::CustomUserData*>(glfwGetWindowUserPointer(window));

	if (!userData.vulkan_initialized) return;

	ltracer::SwapChainSupportDetails swapChainSupport = userData.swapChainSupportDetails;
	VkExtent2D extent = userData.window.chooseSwapExtent(swapChainSupport.capabilities);
	if (extent.height > 0 && extent.width > 0)
	{

		vkDeviceWaitIdle(userData.logicalDevice);
		userData.renderer.cleanupFramebufferAndImageViews();
		userData.window.recreateSwapChain(
		    userData.physicalDevice, userData.logicalDevice, extent, swapChainSupport);
		userData.renderer.createImageViews(userData.logicalDevice);
		userData.renderer.createFramebuffers(userData.logicalDevice, userData.window);

		ltracer::QueueFamilyIndices indices
		    = ltracer::findQueueFamilies(userData.physicalDevice, userData.window.getVkSurface());

		userData.renderer.recreateRaytracingImageAndImageView(indices);
		userData.camera.updateScreenSize(extent.width, extent.height);
	}
}
} // namespace ltracer
