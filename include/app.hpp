#pragma once

#include <memory>

#include "custom_user_data.hpp"
#include "deletion_queue.hpp"
#include "ui.hpp"
#include "renderer.hpp"

class Application
{
  public:
	Application() = default;
	~Application() = default;

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	Application(Application&&) noexcept = delete;
	Application& operator=(Application&&) = delete;

	void run();

  private:
	void createWindow();
	void createRenderer();
	bool checkValidationLayerSupport();
	void initInputHandlers();
	void printSelectedGPU();
	void initVulkan();
	void createLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDeviceToCheck,
	                                 const std::vector<const char*> requiredDeviceExtensions);
	[[nodiscard]] bool pickPhysicalDevice(const std::vector<const char*> requiredDeviceExtensions);

	static void resizeFramebuffer(VkPhysicalDevice physicalDevice,
	                              VkDevice logicalDevice,
	                              ltracer::Renderer& renderer,
	                              ltracer::Window& window,
	                              ltracer::Camera& camera,
	                              ltracer::SwapChainSupportDetails& swapChainSupportDetails);

	static ltracer::SwapChainSupportDetails
	updateSwapChainSupportDetails(VkPhysicalDevice physicalDevice, ltracer::Window& window)
	{
		return querySwapChainSupport(physicalDevice, window);
	}

	static ltracer::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice,
	                                                              ltracer::Window& window);

	bool isDeviceSuitable(VkPhysicalDevice physicalDeviceToCheck,
	                      const std::vector<const char*> requiredDeviceExtensions);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	static VkResult
	createDebugUtilsMessengerEXT(VkInstance instance,
	                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                             const VkAllocationCallbacks* pAllocator,
	                             VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	                                          const VkDebugUtilsMessengerEXT debugMessenger,
	                                          const VkAllocationCallbacks* pAllocator);
	void mainLoop();
	void cleanupApp();
	void createInstanceForVulkan();
	std::vector<const char*> getRequiredInstanceExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL
	debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	              [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	              [[maybe_unused]] void* pUserData);
	static void framebufferResizeCallback(GLFWwindow* window,
	                                      [[maybe_unused]] int width,
	                                      [[maybe_unused]] int height);

  private:
	// stores the data displayed in the ui
	std::unique_ptr<ltracer::ui::UIData> uiData;

	// stores the data needed for the window callback events, e.g. mouse move, keyboard input, etc.
	std::unique_ptr<ltracer::CustomUserData> customUserData;

	// the deletion queue used for the deinitialization of vulkan objects on exit
	ltracer::DeletionQueue mainDeletionQueue;

	ltracer::SwapChainSupportDetails swapChainSupportDetails;

	// whether the GPU supports ray tracing, if false, only the ui is renderer
	bool raytracingSupported = false;

	// whether vulkan has been initialized, to make sure window events don't trigger beforehand,
	// e.g. window resize
	bool vulkan_initialized = false;

	// vulkan instance
	VkInstance vulkanInstance;

	// properties of the selected GPU (info is displayed via the UI)
	VkPhysicalDeviceProperties physicalDeviceProperties;

	ltracer::Window window;
	std::unique_ptr<ltracer::Renderer> renderer;
	ltracer::Camera camera;

	// Handle for the debug message callback
	VkDebugUtilsMessengerEXT debugMessenger;

	// physical device handle
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// Logical device to interact with
	VkDevice logicalDevice = VK_NULL_HANDLE;

	// TODO: these can probably be moved into the Renderer class
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;

#ifdef NDEBUG
	const bool enableValidationLayer = false;
	const bool enableDebugShaderPrintf = false;
#else
	const bool enableValidationLayer = true;
	const bool enableDebugShaderPrintf = true;
#endif

	const std::vector<const char*> validationLayers = {
	    "VK_LAYER_KHRONOS_validation",
	};

	// used to select a GPU that supports ray tracing
	const std::vector<const char*> deviceExtensionsForRaytracing = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,

	    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,

	    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	    // VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
	    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	    // VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    // needed for ray tracing capabilities
	    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
	    // VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
	    // VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
	};

	// if no device can be found that supports ray tracing, search for a device to display to
	// the screen (to show messages, errors etc.) (when raytracingSupported is false)
	const std::vector<const char*> deviceExtensionsForDisplay = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	};
};
