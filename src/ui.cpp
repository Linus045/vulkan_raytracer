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

namespace tracer
{

namespace ui
{

static VkDescriptorPool imguiPool;

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

bool renderCameraProperties(UIData& uiData)
{
	bool valueChanged = false;
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

		float fovy_degree = uiData.camera.getFOVY();
		valueChanged
		    = ImGui::SliderFloat(
		          "Camera FOV", &fovy_degree, 0.1f, 180.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp)
		      || valueChanged;
		uiData.camera.setFOVY(fovy_degree);
	}

	return valueChanged;
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

	if (ImGui::CollapsingHeader("Raytracing - Environment & Light"))
	{
		ImGui::SeparatorText("Values");
		{
			bool lightPositionChanged
			    = ImGui::SliderFloat3("Global Light Position",
			                          &uiData.raytracingDataConstants.globalLightPosition.x,
			                          -30.0f,
			                          30.0f,
			                          "%.1f",
			                          0);
			if (lightPositionChanged)
			{
				uiData.recreateAccelerationStructures.requestRecreate(false);
			}

			valueChanged = lightPositionChanged || valueChanged;

			valueChanged = ImGui::SliderFloat("Global Light Intensity",
			                                  &uiData.raytracingDataConstants.globalLightIntensity,
			                                  0.0f,
			                                  1.0f,
			                                  "%.4f",
			                                  0)
			               || valueChanged;
			valueChanged
			    = ImGui::SliderFloat("Environment Light Intensity",
			                         &uiData.raytracingDataConstants.environmentLightIntensity,
			                         0.0f,
			                         1.0f,
			                         "%.2f",
			                         0)
			      || valueChanged;

			ImGui::Spacing();
		}

		ImGui::SeparatorText("Colors");
		{
			valueChanged = ImGui::ColorEdit3("Global Light Color",
			                                 &uiData.raytracingDataConstants.globalLightColor.x,
			                                 0)
			               || valueChanged;

			valueChanged = ImGui::ColorEdit3("Environment Light Color",
			                                 &uiData.raytracingDataConstants.environmentColor.x,
			                                 0)
			               || valueChanged;
		}
	}

	if (ImGui::CollapsingHeader("Raytracing - Configuration"))
	{
		ImGui::SeparatorText("Debug");
		{
			// passing a boolean in a storage buffer directly didn't work probably due to alignment
			// issues, so we just send 0 or 1 instead
			bool debugShowAABBs = uiData.raytracingDataConstants.debugShowAABBs > 0;
			valueChanged = ImGui::Checkbox(
			                   "Debug: Show Axis-Aligned Bounding-Boxes (ignores slicing planes)",
			                   &debugShowAABBs)
			               || valueChanged;
			TOOLTIP("Renders the AABB of each object instead of the actual object");
			uiData.raytracingDataConstants.debugShowAABBs
			    = static_cast<float>(debugShowAABBs ? 1 : 0);

			bool debugPrintCrosshairRay = uiData.raytracingDataConstants.debugPrintCrosshairRay > 0;
			valueChanged = ImGui::Checkbox("Debug: Print Crosshair Ray calculations",
			                               &debugPrintCrosshairRay)
			               || valueChanged;
			TOOLTIP("Prints the ray calculations for the crosshair ray in the console. "
			        "This is only used for debugging purposes.");
			uiData.raytracingDataConstants.debugPrintCrosshairRay
			    = static_cast<float>(debugPrintCrosshairRay ? 1 : 0);

			bool debugSlicingPlanes = uiData.raytracingDataConstants.debugSlicingPlanes > 0;
			valueChanged = ImGui::Checkbox("Debug: Show Slicing Planes", &debugSlicingPlanes)
			               || valueChanged;
			TOOLTIP("Highlights the slicing plane on the object in pink");
			uiData.raytracingDataConstants.debugSlicingPlanes
			    = static_cast<float>(debugSlicingPlanes ? 1 : 0);

			bool debugShowSubdivisions
			    = uiData.raytracingDataConstants.debugHighlightObjectEdges > 0;
			valueChanged
			    = ImGui::Checkbox("Debug: Show Edges", &debugShowSubdivisions) || valueChanged;
			TOOLTIP("Highlights the edges of the objects");
			uiData.raytracingDataConstants.debugHighlightObjectEdges
			    = static_cast<float>(debugShowSubdivisions ? 1 : 0);

			bool debugFastRenderMode = uiData.raytracingDataConstants.debugFastRenderMode > 0;
			valueChanged
			    = ImGui::Checkbox(
			          "Debug: Enable fast render mode (decreases tolerace for frames < 10)",
			          &debugFastRenderMode)
			      || valueChanged;
			TOOLTIP("Decreases the tolerance for the first 10 frames. "
			        "Allows quicker camera movements if the scene takes a long time to render (low "
			        "FPS). This is only intended for debugging purposes.");
			uiData.raytracingDataConstants.debugFastRenderMode
			    = static_cast<float>(debugFastRenderMode ? 1 : 0);

			valueChanged = ImGui::SliderFloat("Newton Method Tolerance F Value",
			                                  &uiData.raytracingDataConstants.newtonErrorFTolerance,
			                                  1e-8f,
			                                  15.0f,
			                                  "%.8f",
			                                  ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;
			TOOLTIP(
			    "Tolerance for the Newton method. "
			    "If the error is below this value, the ray is considered to have hit an object.");

			bool newtonErrorFIgnoreIncrease
			    = uiData.raytracingDataConstants.newtonErrorFIgnoreIncrease > 0;
			valueChanged
			    = ImGui::Checkbox("Newton ErrorF Ignore increases", &newtonErrorFIgnoreIncrease)
			      || valueChanged;
			TOOLTIP("If the error increases, the ray is considered to miss the object. This option "
			        "disables this behavior, allowing the error to increase during the "
			        "Newton-Method search.");
			uiData.raytracingDataConstants.newtonErrorFIgnoreIncrease
			    = static_cast<float>(newtonErrorFIgnoreIncrease ? 1 : 0);

			bool newtonErrorFHitBelowTolerance
			    = uiData.raytracingDataConstants.newtonErrorFHitBelowTolerance > 0;
			valueChanged = ImGui::Checkbox("Newton ErrorF Below Tolerance counts as hit ",
			                               &newtonErrorFHitBelowTolerance)
			               || valueChanged;
			TOOLTIP(
			    "If the error is below this value, the ray is considered to have hit an object.");
			uiData.raytracingDataConstants.newtonErrorFHitBelowTolerance
			    = static_cast<float>(newtonErrorFHitBelowTolerance ? 1 : 0);
			valueChanged = ImGui::SliderInt("Number of initial guesses for Newton-Method",
			                                &uiData.raytracingDataConstants.newtonGuessesAmount,
			                                0,
			                                6,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;
			TOOLTIP("Number of initial guesses for the Newton method to try."
			        "The Newton-Method is fully executed for each guess and the best/closest "
			        "intersection point is used");

			valueChanged = ImGui::SliderInt("Max Newton-Iterations",
			                                &uiData.raytracingDataConstants.newtonMaxIterations,
			                                0,
			                                MAX_NEWTON_ITERATIONS,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;
			TOOLTIP("Max iterations for the Newton method. If the method does not converge within "
			        "this number of iterations, the ray is considered to miss the object.");

			valueChanged = ImGui::SliderInt("Recursive Rays per Pixel",
			                                &uiData.raytracingDataConstants.recursiveRaysPerPixel,
			                                0,
			                                20,
			                                "%d",
			                                ImGuiSliderFlags_AlwaysClamp)
			               || valueChanged;
			TOOLTIP(
			    "WARNING: This options is useless in the "
			    "current form because there is no randomness, leave it at 1."
			    "Each ray is deterministic and there is no randomness in the ray tracing "
			    "process e.g. global illumnination or reflection/refraction is not implemented.");
		}
	}
	uiData.configurationChanged = uiData.configurationChanged || valueChanged;
}

void renderButtons(const tracer::ui::UIData& uiData)
{
	for (const auto& button : uiData.buttonCallbacks)
	{
		if (ImGui::Button(button.label.c_str()))
		{
			button.callback();
		}
		if (!button.tooltip.empty())
		{
			TOOLTIP(button.tooltip.c_str());
		}
	}
}

void renderHelpInfo(const tracer::ui::UIData& uiData)
{
	ImGui::SeparatorText("Control:");
	ImGui::Text("Press Q to quit");
	ImGui::Text("W/A/S/D to move");
	ImGui::Text("Scroll up/down to increase/decrease movement speed");
	ImGui::Text("Arrow keys to rotate");
	ImGui::Text("Esc|G to [G]rab/release mouse cursor");
	ImGui::Text("C to [C]ollapse/Expand menu");

	ImGui::Separator();
	ImGui::Text("Movement speed: %f", uiData.camera.getMovementSpeed());
}

void renderRaytracingProperties(const tracer::ui::UIData& uiData)
{
	ImGui::SeparatorText("Raytracing Properties:");
	ImGui::Text("Frame Count: %d", uiData.frameCount);
	ImGui::Text("Estimated frame time: %.4fms", uiData.frameTimeMilliseconds);

	// if (uiData.raytracingSupported)
	{
		ImGui::Text("BLAS Instances Count: %ld", uiData.blasInstancesCount);
	}
}

void renderPositionSliders(tracer::ui::UIData& uiData)
{
	if (ImGui::CollapsingHeader("Raytracing - Positions"))
	{

		bool valueChanged = false;
		for (size_t i = 0; i < uiData.positions.size(); i++)
		{
			valueChanged = ImGui::SliderFloat3(("ControlPoint " + std::to_string(i)).c_str(),
			                                   &uiData.positions[i].x,
			                                   -10.0,
			                                   10.0,
			                                   "%.2f")
			               || valueChanged;
		}
		if (valueChanged)
		{
			uiData.recreateAccelerationStructures.requestRecreate(false);
		}
	}
}

void renderSlicingPlaneSliders(UIData& uiData)
{
	if (ImGui::CollapsingHeader("Raytracing - Slicing Planes"))
	{
		// if (uiData.raytracingSupported)
		{
			{
				bool valueChanged = false;
				bool enableSlicingPlanes = uiData.raytracingDataConstants.enableSlicingPlanes > 0;
				valueChanged = ImGui::Checkbox("Debug: Enable Slicing Planes", &enableSlicingPlanes)
				               || valueChanged;
				uiData.raytracingDataConstants.enableSlicingPlanes
				    = static_cast<float>(enableSlicingPlanes ? 1 : 0);

				uiData.configurationChanged = uiData.configurationChanged || valueChanged;
			}

			{
				bool valueChanged = false;
				for (size_t i = 0; i < uiData.slicingPlanes.size(); i++)
				{
					ImGui::LabelText("Slicingplane:", "%ld", i);
					valueChanged = ImGui::SliderFloat3(
					                   (std::string("Position#") + std::to_string(i)).c_str(),
					                   &uiData.slicingPlanes[i].planeOrigin.x,
					                   -10.0,
					                   10.0,
					                   "%.2f")
					               || valueChanged;
					valueChanged
					    = ImGui::SliderFloat3((std::string("Normal#") + std::to_string(i)).c_str(),
					                          &uiData.slicingPlanes[i].normal.x,
					                          -1.0,
					                          1.0,
					                          "%.2f")
					      || valueChanged;
				}
				if (valueChanged)
				{
					uiData.recreateAccelerationStructures.requestRecreate(false);
				}
			}
		}
	}
}

void renderSceneOptions(UIData& uiData)
{
	bool sceneReloadNeeded = false;
	bool valueChanged = false;
	if (ImGui::CollapsingHeader("Raytracing - Scene"))
	{

		bool debugVisualizeControlPoints
		    = uiData.raytracingDataConstants.debugVisualizeControlPoints > 0;
		sceneReloadNeeded
		    = ImGui::Checkbox("Debug: Visualize Control Points", &debugVisualizeControlPoints)
		      || sceneReloadNeeded;
		TOOLTIP("Whether to render the bezier tetrahedron's control points or not");
		uiData.raytracingDataConstants.debugVisualizeControlPoints
		    = static_cast<float>(debugVisualizeControlPoints ? 1 : 0);

		bool debugVisualizeSampledSurface
		    = uiData.raytracingDataConstants.debugVisualizeSampledSurface > 0;
		sceneReloadNeeded
		    = ImGui::Checkbox("Debug: Visualize sampled surface", &debugVisualizeSampledSurface)
		      || sceneReloadNeeded;
		TOOLTIP(
		    "Whether to render the sides of the bezier tetrahedrons with sampled points or not.");
		uiData.raytracingDataConstants.debugVisualizeSampledSurface
		    = static_cast<float>(debugVisualizeSampledSurface ? 1 : 0);

		bool debugVisualizeSampledVolume
		    = uiData.raytracingDataConstants.debugVisualizeSampledVolume > 0;
		sceneReloadNeeded
		    = ImGui::Checkbox("Debug: Visualize sampled Volume", &debugVisualizeSampledVolume)
		      || sceneReloadNeeded;
		TOOLTIP("Whether to render the tetrahedrons volume as sampled points or not (disable "
		        "triangle sides is to see)");
		uiData.raytracingDataConstants.debugVisualizeSampledVolume
		    = static_cast<float>(debugVisualizeSampledVolume ? 1 : 0);

		bool renderSideTriangle = uiData.raytracingDataConstants.renderSideTriangle > 0;
		valueChanged = ImGui::Checkbox("Render Triangles", &renderSideTriangle) || valueChanged;
		TOOLTIP("Whether to render the sides of the bezier tetrahedrons or not.");
		uiData.raytracingDataConstants.renderSideTriangle
		    = static_cast<float>(renderSideTriangle ? 1 : 0);
	}

	uiData.configurationChanged = uiData.configurationChanged || valueChanged;

	if (sceneReloadNeeded)
	{
		uiData.sceneReloader.requestSceneReload();
	}
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
	ImGui::SetNextWindowSize(ImVec2(700, 800), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

	ImGuiWindowFlags window_flags = 0;

	// set collapsed state
	ImGui::SetNextWindowCollapsed(uiData.mainPanelCollapsed, ImGuiCond_Always);
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
	renderRaytracingProperties(uiData);
	renderGPUProperties(uiData);
	renderCameraProperties(uiData);

	// TODO: display the control points in the ui and make them editable
	// renderPositionSliders(uiData);

	ImGui::SeparatorText("Configuration");
	renderRaytracingOptions(uiData);
	renderSlicingPlaneSliders(uiData);
	renderSceneOptions(uiData);

	ImGui::SeparatorText("Buttons");
	renderButtons(uiData);

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
} // namespace tracer
