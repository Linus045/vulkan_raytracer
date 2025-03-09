#include <array>
#include <imgui.h>
#include <string>

#include "ui.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

#include "common_types.h"
#include "camera.hpp"
#include "deletion_queue.hpp"
#include "window.hpp"

namespace ltracer
{

namespace ui
{

void initImgui(VkInstance vulkanInstance,
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
	pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
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

void renderCameraProperties(const UIData& uiData)
{
	if (ImGui::CollapsingHeader("Camera"))
	{
		auto cameraPosition = glm::to_string(uiData.camera.transform.getPos());
		ImGui::Text("Position: %s", cameraPosition.c_str());

		auto cameraLookDirection = glm::to_string(uiData.camera.transform.getForward());
		ImGui::Text("Look Direction: %s", cameraLookDirection.c_str());
		ImGui::Text("Camera yaw (degrees): %f", glm::degrees(uiData.camera.getYawRadians()));
		ImGui::Text("Camera pitch (degrees): %f", glm::degrees(uiData.camera.getPitchRadians()));

		auto cameraRotationRadians
		    = glm::to_string(glm::eulerAngles(uiData.camera.transform.getRotation()));
		auto cameraRotationDegrees
		    = glm::to_string(glm::degrees(glm::eulerAngles(uiData.camera.transform.getRotation())));
		ImGui::Text("Camera rotation (radians)(pitch,yaw,roll): %s", cameraRotationRadians.c_str());
		ImGui::Text("Camera rotation (degrees)(pitch,yaw,roll): %s", cameraRotationDegrees.c_str());
	}
}

void renderGPUProperties(const UIData& uiData)
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

void renderErrors(const UIData& uiData)
{
	ImGui::SeparatorText("ERRORS:");
	if (!uiData.raytracingSupported)
	{
		ImGui::Text("Hardware ray tracing not supported!");
	}
}

void renderRaytracingOptions(UIData& uiData)
{
	bool valueChanged = false;
	if (ImGui::CollapsingHeader("Raytracing - Configuration"))
	{
		ImGui::SeparatorText("Debug");
		{
			// passing a boolean in a storage buffer directly didn't work probably due to alignment
			// issues, so we just send 0 or 1 instead
			bool debugShowAABBs = uiData.raytracingDataConstants.debugShowAABBs > 0;
			valueChanged
			    = ImGui::Checkbox("Debug Axis-Aligned Bounding-Boxes (ignores slicing planes)",
			                      &debugShowAABBs)
			      || valueChanged;
			uiData.raytracingDataConstants.debugShowAABBs
			    = static_cast<float>(debugShowAABBs ? 1 : 0);

			valueChanged = ImGui::SliderFloat("Newton Method Tolerance X Value",
			                                  &uiData.raytracingDataConstants.newtonErrorXTolerance,
			                                  1e-8f,
			                                  15.0f,
			                                  "%.8f",
			                                  ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;

			valueChanged = ImGui::SliderFloat("Newton Method Tolerance F Value",
			                                  &uiData.raytracingDataConstants.newtonErrorFTolerance,
			                                  1e-8f,
			                                  15.0f,
			                                  "%.8f",
			                                  ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;

			bool newtonErrorFIgnoreIncrease
			    = uiData.raytracingDataConstants.newtonErrorFIgnoreIncrease > 0;
			valueChanged
			    = ImGui::Checkbox("Newton ErrorF Ignore increases", &newtonErrorFIgnoreIncrease)
			      || valueChanged;
			uiData.raytracingDataConstants.newtonErrorFIgnoreIncrease
			    = static_cast<float>(newtonErrorFIgnoreIncrease ? 1 : 0);

			bool newtonErrorFHitBelowTolerance
			    = uiData.raytracingDataConstants.newtonErrorFHitBelowTolerance > 0;
			valueChanged = ImGui::Checkbox("Newton ErrorF Below Tolerance counts as hit ",
			                               &newtonErrorFHitBelowTolerance)
			               || valueChanged;
			uiData.raytracingDataConstants.newtonErrorFHitBelowTolerance
			    = static_cast<float>(newtonErrorFHitBelowTolerance ? 1 : 0);

			bool newtonErrorXIgnoreIncrease
			    = uiData.raytracingDataConstants.newtonErrorXIgnoreIncrease > 0;
			valueChanged
			    = ImGui::Checkbox("Newton ErrorX Ignore increases", &newtonErrorXIgnoreIncrease)
			      || valueChanged;
			uiData.raytracingDataConstants.newtonErrorXIgnoreIncrease
			    = static_cast<float>(newtonErrorXIgnoreIncrease ? 1 : 0);

			bool newtonErrorXHitBelowTolerance
			    = uiData.raytracingDataConstants.newtonErrorXHitBelowTolerance > 0;
			valueChanged = ImGui::Checkbox("Newton ErrorX Below Tolerance counts as hit ",
			                               &newtonErrorXHitBelowTolerance)
			               || valueChanged;
			uiData.raytracingDataConstants.newtonErrorXHitBelowTolerance
			    = static_cast<float>(newtonErrorXHitBelowTolerance ? 1 : 0);

			valueChanged = ImGui::SliderInt("Max Newton-Iterations",
			                                &uiData.raytracingDataConstants.newtonMaxIterations,
			                                0,
			                                20,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;

			valueChanged = ImGui::SliderFloat("Some Floating Scalar Value",
			                                  &uiData.raytracingDataConstants.someFloatingScalar,
			                                  1e-8f,
			                                  1.0f,
			                                  "%.8f",
			                                  ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;

			valueChanged = ImGui::SliderInt("Some Scalar Value",
			                                &uiData.raytracingDataConstants.someScalar,
			                                -1,
			                                uiData.raytracingDataConstants.newtonMaxIterations,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;

			bool renderSide1 = uiData.raytracingDataConstants.renderSide1 > 0;
			valueChanged = ImGui::Checkbox("Render Side 1", &renderSide1) || valueChanged;
			uiData.raytracingDataConstants.renderSide1 = static_cast<float>(renderSide1 ? 1 : 0);

			bool renderSide2 = uiData.raytracingDataConstants.renderSide2 > 0;
			valueChanged = ImGui::Checkbox("Render Side 2", &renderSide2) || valueChanged;
			uiData.raytracingDataConstants.renderSide2 = static_cast<float>(renderSide2 ? 1 : 0);

			bool renderSide3 = uiData.raytracingDataConstants.renderSide3 > 0;
			valueChanged = ImGui::Checkbox("Render Side 3", &renderSide3) || valueChanged;
			uiData.raytracingDataConstants.renderSide3 = static_cast<float>(renderSide3 ? 1 : 0);

			bool renderSide4 = uiData.raytracingDataConstants.renderSide4 > 0;
			valueChanged = ImGui::Checkbox("Render Side 4", &renderSide4) || valueChanged;
			uiData.raytracingDataConstants.renderSide4 = static_cast<float>(renderSide4 ? 1 : 0);

			valueChanged = ImGui::SliderInt("Rays per Pixel",
			                                &uiData.raytracingDataConstants.raysPerPixel,
			                                0,
			                                20,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;
		}

		ImGui::SeparatorText("Environment");
		{
			valueChanged = ImGui::ColorPicker3("Environment Color",
			                                   &uiData.raytracingDataConstants.environmentColor.x,
			                                   0)
			               || valueChanged;
		}

		ImGui::SeparatorText("Light");
		{
			valueChanged
			    = ImGui::SliderFloat3("Global Light Position",

			                          &uiData.raytracingDataConstants.globalLightPosition.x,
			                          -1000.0f,
			                          1000.0f,
			                          "%.1f",
			                          0)
			      || valueChanged;
			valueChanged = ImGui::ColorPicker3("Global Light Color",
			                                   &uiData.raytracingDataConstants.globalLightColor.x,
			                                   0)
			               || valueChanged;
		}
	}
	uiData.configurationChanged = valueChanged;
}

void renderHelpInfo(const ltracer::ui::UIData& uiData)
{
	ImGui::SeparatorText("Control:");
	ImGui::Text("Press Q to quit");
	ImGui::Text("W/A/S/D to move");
	ImGui::Text("Scroll up/down to increase/decrease movement speed");
	ImGui::Text("Arrow keys to rotate");
	ImGui::Text("Esc|G to [G]rab/release mouse cursor");

	ImGui::Separator();
	ImGui::Text("Movement speed: %f", uiData.camera.getMovementSpeed());
}

void renderRaytracingProperties(const ltracer::ui::UIData& uiData)
{
	ImGui::SeparatorText("Raytracing Properties:");
	ImGui::Text("Frame Count: %d", uiData.frameCount);
}

void renderPositionSliders(ltracer::ui::UIData& uiData)
{
	bool valueChanged = false;

	if (ImGui::CollapsingHeader("Raytracing - Positions"))
	{

		valueChanged = ImGui::SliderFloat3("Position", &uiData.position.x, -10.0, 10.0, "%.2f")
		               || valueChanged;
	}
	uiData.recreateAccelerationStructures = valueChanged;
}

void renderCrosshair(const UIData& uiData)
{

	auto draw = ImGui::GetBackgroundDrawList();
	auto windowWidth = static_cast<float>(uiData.window.getWidth());
	auto windowHeight = static_cast<float>(uiData.window.getHeight());
	draw->AddCircle(
	    ImVec2(windowWidth / 2.0f, windowHeight / 2.0f), 6, IM_COL32(0, 0, 0, 255), 100, 0.0f);
	draw->AddCircle(
	    ImVec2(windowWidth / 2.0f, windowHeight / 2.0f), 1, IM_COL32(255, 255, 255, 255), 10, 1.0f);
}

void renderMainPanel(UIData& uiData)
{
	// ImGui::ShowDemoWindow();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 100),
	                        ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

	ImGuiWindowFlags window_flags = 0;
	if (!ImGui::Begin("Status", nullptr, window_flags))
	{
		uiData.mainPanelCollapsed = true;
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	uiData.mainPanelCollapsed = false;

	renderHelpInfo(uiData);

	ImGui::SeparatorText("Properties:");
	renderGPUProperties(uiData);
	renderCameraProperties(uiData);
	renderRaytracingProperties(uiData);

	renderPositionSliders(uiData);

	ImGui::SeparatorText("Configuration");
	renderRaytracingOptions(uiData);

	renderErrors(uiData);
	ImGui::End();
}

void beginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void endFrame()
{
	ImGui::EndFrame();
	ImGui::Render();
}

} // namespace ui
} // namespace ltracer
