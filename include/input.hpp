#pragma once

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

#include <imgui.h>
#include "backends/imgui_impl_glfw.h"

#include "custom_user_data.hpp"
#include "camera.hpp"
#include "window.hpp"

namespace ltracer
{

inline void handleMouseInputCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

inline void handleMouseMovementCallback(GLFWwindow* window, double xpos, double ypos)
{
	CustomUserData& userData = *reinterpret_cast<CustomUserData*>(glfwGetWindowUserPointer(window));

	// TODO: Add mouse rotation via a virtual trackball e.g.
	// https://computergraphics.stackexchange.com/questions/151/how-to-implement-a-trackball-in-opengl
	if (userData.window.getMouseCursorCaptureEnabled())
	{
		// TODO: move these into a more fitting space
		// TODO: move this into the UI
		double mouse_sensitivity = 0.1;

		double deltaX = xpos - userData.lastMouseX;
		double deltaY = ypos - userData.lastMouseY;

		// TODO: for some reason sometimes after capturing the mouse cursor (pressing G) the delta
		// values are huge which results in weird camera snapping
		// so a workaround is to ignore the first mouse movement event
		// I suspect this is due to the mouse cursor being moved by glfw when switching the cursor
		// input mode
		if (userData.ignoreFirstMouseMovement)
		{
			userData.ignoreFirstMouseMovement = false;
			std::cout << "Ignoring first mouse movement" << std::endl;
			std::cout << "Delta X: " << deltaX << " Delta Y: " << deltaY << std::endl;
		}
		else
		{
			userData.camera.rotateYawY(static_cast<float>(-deltaX * mouse_sensitivity));
			userData.camera.rotatePitchX(static_cast<float>(-deltaY * mouse_sensitivity));
		}
	}
	else
	{
		ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
	}

	// always store the last mouse position which prevents the snapping problems a bit
	userData.lastMouseX = static_cast<float>(xpos);
	userData.lastMouseY = static_cast<float>(ypos);

	// std::cout << "Mouse position: " << xpos << ", " << ypos << std::endl;
}

inline void updateMovement(CustomUserData& userData, double delta)
{
	if (userData.keyStateMap[GLFW_KEY_W])
	{
		userData.camera.translate(userData.camera.transform.getForward()
		                          * userData.camera.getMovementSpeed() * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_S])
	{
		userData.camera.translate(userData.camera.transform.getForward()
		                          * -userData.camera.getMovementSpeed() * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_A])
	{
		userData.camera.translate(userData.camera.transform.getRight()
		                          * -userData.camera.getMovementSpeed() * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_D])
	{
		userData.camera.translate(userData.camera.transform.getRight()
		                          * userData.camera.getMovementSpeed() * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_SPACE])
	{
		userData.camera.translate(userData.camera.globalUp * userData.camera.getMovementSpeed()
		                          * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_LEFT_SHIFT])
	{
		userData.camera.translate(userData.camera.globalUp * -userData.camera.getMovementSpeed()
		                          * delta);
	}

	if (userData.keyStateMap[GLFW_KEY_LEFT])
	{
		userData.camera.rotateYawY(userData.camera.getRotationSpeed() * static_cast<float>(delta));
	}

	if (userData.keyStateMap[GLFW_KEY_RIGHT])
	{
		userData.camera.rotateYawY(-userData.camera.getRotationSpeed() * static_cast<float>(delta));
	}

	if (userData.keyStateMap[GLFW_KEY_UP])
	{
		userData.camera.rotatePitchX(userData.camera.getRotationSpeed()
		                             * static_cast<float>(delta));
	}

	if (userData.keyStateMap[GLFW_KEY_DOWN])
	{
		userData.camera.rotatePitchX(-userData.camera.getRotationSpeed()
		                             * static_cast<float>(delta));
	}
}

inline void handleInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	CustomUserData& userData = *reinterpret_cast<CustomUserData*>(glfwGetWindowUserPointer(window));

	// always detect Q and ESC/G key
	if (action == GLFW_PRESS && key == GLFW_KEY_Q)
	{
		glfwSetWindowShouldClose(userData.window.getGLFWWindow(), true);
	}
	else if (action == GLFW_PRESS && (key == GLFW_KEY_G || key == GLFW_KEY_ESCAPE))
	{
		userData.window.setMouseCursorCapturedEnabled(
		    !userData.window.getMouseCursorCaptureEnabled());
		userData.ignoreFirstMouseMovement = true;

		std::cout << "Cursor capture enabled: "
		          << (userData.window.getMouseCursorCaptureEnabled() ? "True" : "False")
		          << std::endl;
	}

	if (!userData.window.getMouseCursorCaptureEnabled())
	{
		ImGui_ImplGlfw_KeyCallback(userData.window.getGLFWWindow(), key, scancode, action, mods);
	}
	else if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		// only update the key state map when not using Imgui UI
		userData.keyStateMap[key] = true;
	}

	// always reset the key state map when the key is released to prevent issues with a key being
	// 'stuck' in the down position
	if (action == GLFW_RELEASE)
	{
		userData.keyStateMap[key] = false;
	}
}

inline void
handleMouseScrollCallback(GLFWwindow* window, [[maybe_unused]] double xOffset, double yOffset)
{
	CustomUserData& userData = *reinterpret_cast<CustomUserData*>(glfwGetWindowUserPointer(window));

	if (!userData.window.getMouseCursorCaptureEnabled())
	{
		ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
	}
	else
	{
		float change = static_cast<float>(glm::sign(yOffset) * 1.0f);

		// if movement speed is less than 1.0f, slow down even more
		if (userData.camera.getMovementSpeed() + change * 0.01f <= 0.1f)
		{
			change *= 0.01f;
		}
		else if (userData.camera.getMovementSpeed() + change * 0.1f <= 1.0f)
		{
			change *= 0.1f;
		}

		userData.camera.setMovementSpeed(userData.camera.getMovementSpeed() + change);
	}
}

} // namespace ltracer
