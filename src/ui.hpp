#pragma once

#include <array>
#include <string>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include "src/camera.hpp"
#include "src/window.hpp"
#include "src/custom_user_data.hpp"

namespace ltracer
{

namespace ui
{

// TODO: consider storing the raw data values instead of pointers/references and
// updating the struct regularly
struct UIData
{
	const Camera& camera;
	const bool& raytracingSupported;
	const VkPhysicalDeviceProperties& physicalDeviceProperties;
};

struct UIStatus
{
	bool mainPanelOpen = true;
};

static UIStatus uiStatus;
static VkDescriptorPool imguiPool;

inline void initImgui(VkInstance vulkanInstance,
                      VkDevice logicalDevice,
                      VkPhysicalDevice physicalDevice,
                      Window& window,
                      VkRenderPass renderPass,
                      VkQueue graphicsQueue,
                      DeletionQueue& deletionQueue)
{
	// oversized but whatever
	VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
	                                     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
	                                     {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
	                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
	                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
	                                     {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(logicalDevice, &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
	{
		throw std::runtime_error("init_imgui - vkCreateDescriptorPool");
	}

	deletionQueue.push_function([=]() { vkDestroyDescriptorPool(logicalDevice, imguiPool, NULL); });

	// initialize the core structures of imgui
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window.getGLFWWindow(), false);

	ImGui_ImplVulkan_InitInfo init_info = {};

	init_info.Instance = vulkanInstance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = logicalDevice;

	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = renderPass;

	ImGui_ImplVulkan_Init(&init_info);

	// Disable saving the settings to imgui.ini file
	ImGui::GetIO().ConfigDebugIniSettings = false;
	ImGui::GetIO().IniFilename = NULL;

	// execute a gpu command to upload imgui font textures
	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	deletionQueue.push_function([]() { ImGui_ImplVulkan_Shutdown(); });
}

inline void renderCameraProperties(const UIData& uiData)
{
	if (ImGui::CollapsingHeader("Camera"))
	{
		auto cameraPosition = glm::to_string(uiData.camera.transform.position);
		ImGui::Text("Position: %s", cameraPosition.c_str());

		auto cameraLookDirection = glm::to_string(uiData.camera.transform.getForward());
		ImGui::Text("Look Direction: %s", cameraLookDirection.c_str());
		ImGui::Text("Camera yaw (degrees): %f", glm::degrees(uiData.camera.getYawRadians()));
		ImGui::Text("Camera pitch (degrees): %f", glm::degrees(uiData.camera.getPitchRadians()));

		auto cameraRotationRadians
		    = glm::to_string(glm::eulerAngles(uiData.camera.transform.rotation));
		auto cameraRotationDegrees
		    = glm::to_string(glm::degrees(glm::eulerAngles(uiData.camera.transform.rotation)));
		ImGui::Text("Camera rotation (radians)(pitch,yaw,roll): %s", cameraRotationRadians.c_str());
		ImGui::Text("Camera rotation (degrees)(pitch,yaw,roll): %s", cameraRotationDegrees.c_str());
	}
}

// inline std::string getPipelineUUIDAsString(const UIData& uiData)
// {
// 	auto pipelineCacheUUID = std::string();
// 	std::for_each(std::begin(uiData.physicalDeviceProperties.pipelineCacheUUID),
// 	              std::end(uiData.physicalDeviceProperties.pipelineCacheUUID),
// 	              [&pipelineCacheUUID](const uint8_t n)
// 	              { pipelineCacheUUID.append(std::format("{:x}", n)); });
// 	return pipelineCacheUUID;
// }

inline void renderGPUProperties(const UIData& uiData)
{
	if (ImGui::CollapsingHeader("PhysicalDevice - GPU - Properties"))
	{
		ImGui::Text("Hardware ray tracing supported: %s",
		            uiData.raytracingSupported ? "Yes" : "No");
		ImGui::Text("Api version: %d", uiData.physicalDeviceProperties.apiVersion);
		ImGui::Text("Driver version: %d", uiData.physicalDeviceProperties.driverVersion);
		ImGui::Text("Vendor ID: %d", uiData.physicalDeviceProperties.vendorID);
		ImGui::Text("Device ID: %d", uiData.physicalDeviceProperties.deviceID);
		ImGui::Text("Device Type: %s",
		            string_VkPhysicalDeviceType(uiData.physicalDeviceProperties.deviceType));
		ImGui::Text("Device name: %s", uiData.physicalDeviceProperties.deviceName);

		// not yet sure if these properties are useful
		// auto pipelineCacheUUID = getPipelineUUIDAsString(uiData);
		// ImGui::Text("Pipeline cache UUID: %s", pipelineCacheUUID.c_str());

		// contains limits of the device, might be useful for debugging
		// ImGui::Text("Limits: %s", uiData.physicalDeviceProperties.limits);

		// sparse properties
		// ImGui::Text("Sparse properties: %s",
		// uiData.physicalDeviceProperties.sparseProperties);
	}
}

inline void renderErrors(const UIData& uiData)
{
	ImGui::SeparatorText("ERRORS:");
	if (!uiData.raytracingSupported)
	{
		ImGui::Text("Hardware ray tracing not supported!");
	}
}

inline void renderMainPanel(const UIData& uiData)
{
	// ImGui::ShowDemoWindow();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 100),
	                        ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags window_flags = 0;
	if (!ImGui::Begin("Status", &uiStatus.mainPanelOpen, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::SeparatorText("Control:");
	ImGui::Text("Press Q to quit");
	ImGui::Text("W/A/S/D to move");
	ImGui::Text("Scroll up/down to increase/decrease movement speed");
	ImGui::Text("Arrow keys to rotate");
	ImGui::Text("Esc|G to [G]rab/release mouse cursor");

	ImGui::Separator();
	ImGui::Text("Movement speed: %f", uiData.camera.getMovementSpeed());

	ImGui::SeparatorText("Data:");
	renderGPUProperties(uiData);
	renderCameraProperties(uiData);
	renderErrors(uiData);
	ImGui::End();
}

inline void beginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

inline void endFrame()
{
	ImGui::EndFrame();
	ImGui::Render();
}

} // namespace ui
} // namespace ltracer
