#pragma once

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <vulkan/vk_enum_string_helper.h>

// forward declarations
struct RaytracingDataConstants;

namespace ltracer
{

// forward declarations
class Camera;
class Window;
struct DeletionQueue;

namespace ui
{

// TODO: consider storing the raw data values instead of pointers/references and
// updating the struct regularly
struct UIData
{
	bool configurationChanged = false;
	const Camera& camera;
	const bool& raytracingSupported;
	const VkPhysicalDeviceProperties& physicalDeviceProperties;
	RaytracingDataConstants& raytracingDataConstants;
	bool mainPanelCollapsed = true;
	const uint32_t& frameCount;

	UIData(const Camera& camera,
	       const bool& raytracingSupported,
	       const VkPhysicalDeviceProperties& physicalDeviceProperties,
	       RaytracingDataConstants& raytracingDataConstants,
	       const uint32_t& frameCount)
	    : camera(camera), raytracingSupported(raytracingSupported),
	      physicalDeviceProperties(physicalDeviceProperties),
	      raytracingDataConstants(raytracingDataConstants), frameCount(frameCount)
	{
	}
};

static VkDescriptorPool imguiPool;

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

void renderGPUProperties(const UIData& uiData);

void renderErrors(const UIData& uiData);

void renderRaytracingOptions(UIData& uiData);

void renderHelpInfo(const ltracer::ui::UIData& uiData);

void renderRaytracingProperties(const ltracer::ui::UIData& uiData);

void renderMainPanel(UIData& uiData);

void beginFrame();

void endFrame();

} // namespace ui
} // namespace ltracer
