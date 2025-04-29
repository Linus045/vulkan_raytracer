#pragma once

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <cstdint>
#include <imgui.h>

#include <glm/ext/vector_float3.hpp>

#include <vulkan/vk_enum_string_helper.h>
#include "blas.hpp"
#include "common_types.h"

// forward declarations
struct RaytracingDataConstants;

namespace tracer
{

// forward declarations
class Camera;
class Window;
struct DeletionQueue;

namespace ui
{

#define TOOLTIP(text)                                                                              \
	if (ImGui::IsItemHovered())                                                                    \
	{                                                                                              \
		ImGui::SetTooltip("%s", text);                                                             \
	}

struct ButtonData
{
	std::string label;
	std::string tooltip;
	std::function<void()> callback;
};

// TODO: consider storing the raw data values instead of pointers/references and
// updating the struct regularly
struct UIData
{
	bool configurationChanged = false;
	Camera& camera;
	const Window& window;
	const bool& raytracingSupported;
	const VkPhysicalDeviceProperties& physicalDeviceProperties;
	RaytracingDataConstants& raytracingDataConstants;
	bool mainPanelCollapsed = true;
	const uint32_t& frameCount;
	const size_t& blasInstancesCount;
	std::vector<SlicingPlane>& slicingPlanes;
	std::vector<tracer::rt::SceneObject>& sceneObjects;

	float frameTimeMilliseconds = 0.0f;

	std::vector<glm::vec3> positions = std::vector<glm::vec3>(10);

	// pair - first value: recreateNeeded? second value: full rebuild?
	struct recreateAccelerationStructure
	{
		/// @brief whether the acceleration structures need to be recreated and
		/// if so, whether a full rebuild is needed
		void requestRecreate(const bool _fullRebuild)
		{
			recreate = true;

			// if something already requested a fullRebuild, don't override it
			fullRebuild = fullRebuild || _fullRebuild;
		}

		void reset()
		{
			recreate = false;
			fullRebuild = false;
		}

		bool isRecreateNeeded() const
		{
			return recreate;
		}

		bool isFullRebuildNeeded() const
		{
			return fullRebuild;
		}

	  private:
		bool recreate = false;
		bool fullRebuild = false;

	} recreateAccelerationStructures;

	struct SceneReloader
	{
		bool isReloadRequested() const
		{
			return sceneReloadRequested;
		}

		void requestSceneReload()
		{
			sceneReloadRequested = true;
		}

		void resetSceneReloadRequest()
		{
			sceneReloadRequested = false;
		}

	  private:
		bool sceneReloadRequested = false;
	} sceneReloader;

	std::vector<ButtonData> buttonCallbacks;

	UIData(Camera& camera,
	       const Window& window,
	       const bool& raytracingSupported,
	       const VkPhysicalDeviceProperties& physicalDeviceProperties,
	       RaytracingDataConstants& raytracingDataConstants,
	       const uint32_t& frameCount,
	       const size_t& blasInstancesCount,
	       std::vector<SlicingPlane>& slicingPlanes,
	       std::vector<tracer::rt::SceneObject>& sceneObjects)
	    : camera(camera), window(window), raytracingSupported(raytracingSupported),
	      physicalDeviceProperties(physicalDeviceProperties),
	      raytracingDataConstants(raytracingDataConstants), frameCount(frameCount),
	      blasInstancesCount(blasInstancesCount), slicingPlanes(slicingPlanes),
	      sceneObjects(sceneObjects)
	{
	}
};

void initImgui(VkInstance vulkanInstance,
               VkDevice logicalDevice,
               VkPhysicalDevice physicalDevice,
               Window& window,
               VkRenderPass renderPass,
               VkQueue graphicsQueue,
               DeletionQueue& deletionQueue);

void renderCameraProperties(const UIData& uiData);

// inline std::string getPipelineUUIDAsString(const UIData& uiData)
// {
// 	auto pipelineCacheUUID = std::string();
// 	std::for_each(std::begin(uiData.physicalDeviceProperties.pipelineCacheUUID),
// 	              std::end(uiData.physicalDeviceProperties.pipelineCacheUUID),
// 	              [&pipelineCacheUUID](const uint8_t n)
// 	              { pipelineCacheUUID.append(std::format("{:x}", n)); });
// 	return pipelineCacheUUID;
// }

void renderBLASObjectInfo(const UIData& uiData);

void renderGPUProperties(const UIData& uiData);

void renderErrors(const UIData& uiData);

void renderRaytracingOptions(UIData& uiData);

void renderHelpInfo(const tracer::ui::UIData& uiData);

void renderButtons(const tracer::ui::UIData& uiData);

void renderRaytracingProperties(const tracer::ui::UIData& uiData);

void renderCrosshair(const UIData& uiData);

void renderMainPanel(UIData& uiData);

void beginFrame();

void endFrame();

} // namespace ui
} // namespace tracer
