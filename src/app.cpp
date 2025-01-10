#include "src/deletion_queue.hpp"
#include "src/ui.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL

#include "src/camera.hpp"
#include "src/logger.hpp"
#include "src/renderer.hpp"
#include "src/window.hpp"

#include "tiny_obj_loader.h"

#define TERM_COLOR_RED "\033[31m"
#define TERM_COLOR_RESET "\033[0m"

class HelloTriangleApplication
{

	void createRenderer(const ltracer::ui::UIData& uiData)
	{
		ltracer::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
		int width, height;
		window->getFramebufferSize(&width, &height);
		renderer = std::make_shared<ltracer::Renderer>(physicalDevice,
		                                               logicalDevice,
		                                               mainDeletionQueue,
		                                               window,
		                                               graphicsQueue,
		                                               presentQueue,
		                                               transferQueue,
		                                               raytracingSupported,
		                                               uiData);

		window->createSwapChain(
		    physicalDevice,
		    logicalDevice,
		    VkExtent2D{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
		    swapChainSupport);

		renderer->initRenderer(vulkanInstance, camera, width, height, swapChainSupport);
	}

  public:
	void run()
	{
		// loadModels();

		camera = std::make_shared<ltracer::Camera>();

		const ltracer::ui::UIData uiData = {
		    .camera = camera,
		    .raytracingSupported = raytracingSupported,
		    .physicalDeviceProperties = physicalDeviceProperties,
		};

		createWindow();

		initInput(window);
		initVulkan();

		createRenderer(uiData);

		mainLoop();

		cleanupApp();
	}

	HelloTriangleApplication()
	{
		//
	}

	~HelloTriangleApplication()
	{
	}

	void createWindow()
	{
		window = std::make_shared<ltracer::Window>();
		window->initWindow(framebufferResizeCallback);
		window->setWindowUserPointer(this);
	}

	HelloTriangleApplication(const HelloTriangleApplication&) = delete;
	HelloTriangleApplication(const HelloTriangleApplication&&) = delete;

	HelloTriangleApplication& operator=(const HelloTriangleApplication&) = delete;

  private:
	ltracer::DeletionQueue mainDeletionQueue;

	bool raytracingSupported = false;
	bool vulkan_initialized = false;
	// vulkan instance
	VkInstance vulkanInstance;
	VkPhysicalDeviceProperties physicalDeviceProperties;

	std::shared_ptr<ltracer::Window> window;
	std::shared_ptr<ltracer::Renderer> renderer;
	std::shared_ptr<ltracer::Camera> camera;

	VkDebugUtilsMessengerEXT debugMessenger;

	// physical device handle
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	// Logical device to interact with
	VkDevice logicalDevice;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;

#ifdef NDEBUG
	const bool enableValidationLayer = false;
#else
	const bool enableValidationLayer = true;
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

	// if no device can be found that supports ray tracing, search for a device to display to the
	// screen (to show messages, errors etc.)
	const std::vector<const char*> deviceExtensionsForDisplay = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
	};

	// std::shared_ptr<std::vector<ltracer::WorldObject>> worldObjects
	//     = std::make_shared<std::vector<ltracer::WorldObject>>();

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			for (auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound) return false;
		}

		return true;
	}

	// void loadModels()
	// {
	// 	std::filesystem::path floor = "3d-models/floor.obj";
	// 	loadModel(floor, glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec3{0, 0, -1.0f});

	// 	// std::filesystem::path cubePath = "../3d-models/cube.obj";
	// 	// auto cube = loadModel(cubePath);
	// 	// logMat4("modelMatrix:", cube.getModelMatrix());

	// 	std::filesystem::path arrowPath = "3d-models/up_arrow.obj";
	// 	auto arrow = loadModel(arrowPath);

	// 	// std::cout << "Loaded " << (*meshObjects).size() << " models\n";
	// }

	// ltracer::WorldObject& loadModel(std::filesystem::path modelPath,
	//                                 glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f},
	//                                 glm::vec3 position = glm::vec3(0, 0, 0))
	// {
	// 	objl::Loader modelLoader;
	// 	bool loadout = modelLoader.LoadFile(modelPath.string());

	// 	if (loadout)
	// 	{
	// 		std::vector<ltracer::Vertex> vertices;
	// 		vertices.reserve(modelLoader.LoadedMeshes[0].Vertices.size());
	// 		std::vector<unsigned int> indices;
	// 		indices.reserve(modelLoader.LoadedMeshes[0].Indices.size());

	// 		for (auto& vert : modelLoader.LoadedMeshes[0].Vertices)
	// 		{
	// 			vertices.emplace_back(glm::vec3(vert.Position.X, vert.Position.Y, vert.Position.Z),
	// 			                      color,
	// 			                      glm::vec3(vert.Normal.X, vert.Normal.Y, vert.Normal.Z));
	// 		}

	// 		for (auto& index : modelLoader.LoadedMeshes[0].Indices)
	// 		{
	// 			indices.emplace_back(index);
	// 		}

	// 		return (*worldObjects).emplace_back(vertices, indices, position);
	// 	}
	// 	else
	// 	{
	// 		std::cerr << TERM_COLOR_RED << "ERROR: Failed to load file '"
	// 		          << std::filesystem::absolute(modelPath) << "' file\n"
	// 		          << TERM_COLOR_RESET;
	// 		throw std::runtime_error(std::string("ERROR: Failed to load file '")
	// 		                         + std::filesystem::absolute(modelPath).string() + "' file\n");
	// 	}
	// }

	void initInput(std::shared_ptr<ltracer::Window> window)
	{
		glfwSetKeyCallback(window->getGLFWWindow(), &HelloTriangleApplication::handleInputCallback);

		glfwSetMouseButtonCallback(window->getGLFWWindow(),
		                           &HelloTriangleApplication::handleMouseInputCallback);
	}

	static void handleMouseInputCallback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	}

	static void handleInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{

		auto ptr = glfwGetWindowUserPointer(window);

		HelloTriangleApplication* ref1 = (HelloTriangleApplication*)ptr;
		auto& ref = *ref1;
		// TODO: don't apply this directly here but rather on the draw call
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
		{
			if (key == GLFW_KEY_W)
			{
				ref.camera->translate(ref.camera->transform.getForward() * 0.3f);
			}
			else if (key == GLFW_KEY_S)
			{
				ref.camera->translate(ref.camera->transform.getForward() * -0.3f);
			}
			else if (key == GLFW_KEY_A)
			{
				ref.camera->translate(ref.camera->transform.getRight() * -0.3f);
			}
			else if (key == GLFW_KEY_D)
			{
				ref.camera->translate(ref.camera->transform.getRight() * 0.3f);
			}
			else if (key == GLFW_KEY_SPACE)
			{
				ref.camera->translate(ref.camera->globalUp * 0.3f);
			}
			else if (key == GLFW_KEY_LEFT_SHIFT)
			{
				ref.camera->translate(ref.camera->globalUp * -0.3f);
			}
			else if (key == GLFW_KEY_LEFT)
			{
				ref.camera->rotateYawY(10.0f);
			}
			else if (key == GLFW_KEY_RIGHT)
			{
				ref.camera->rotateYawY(-10.0f);
			}
			else if (key == GLFW_KEY_UP)
			{
				ref.camera->rotatePitchX(10.0f);
			}
			else if (key == GLFW_KEY_DOWN)
			{
				ref.camera->rotatePitchX(-10.0f);
			}
			else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
			{
				glfwSetWindowShouldClose(ref.window->getGLFWWindow(), true);
			}
			else
			{
				ImGui_ImplGlfw_KeyCallback(
				    ref.window->getGLFWWindow(), key, scancode, action, mods);
			}

			// TODO: Add mouse rotation via a virtual trackball e.g.
			// https://computergraphics.stackexchange.com/questions/151/how-to-implement-a-trackball-in-opengl
		}
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{

		auto& app = *reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));

		if (!app.vulkan_initialized) return;

		ltracer::SwapChainSupportDetails swapChainSupport
		    = app.querySwapChainSupport(app.physicalDevice);
		VkExtent2D extent = app.window->chooseSwapExtent(swapChainSupport.capabilities);
		if (extent.height > 0 && extent.width > 0)
		{

			vkDeviceWaitIdle(app.logicalDevice);
			app.renderer->cleanupFramebufferAndImageViews();
			app.window->recreateSwapChain(
			    app.physicalDevice, app.logicalDevice, extent, swapChainSupport);
			app.renderer->createImageViews(app.logicalDevice);
			app.renderer->createFramebuffers(app.logicalDevice, app.window);

			ltracer::QueueFamilyIndices indices
			    = ltracer::findQueueFamilies(app.physicalDevice, app.window->getVkSurface());

			app.renderer->recreateRaytracingImageAndImageView(indices);
			app.camera->updateScreenSize(extent.width, extent.height);
		}
	}

	void printSelectedGPU()
	{
		std::cout << "Using device: " << physicalDeviceProperties.deviceName << std::endl;
	}

	void initVulkan()
	{
		createInstanceForVulkan();
		if (enableValidationLayer) setupDebugMessenger();
		window->createVulkanSurface(vulkanInstance);

		bool deviceFound = false;
		const std::vector<const char*>* deviceExtensions = NULL;
		if (pickPhysicalDevice(deviceExtensionsForRaytracing))
		{
			deviceFound = true;
			raytracingSupported = true;
			deviceExtensions = &deviceExtensionsForRaytracing;
		}
		else if (pickPhysicalDevice(deviceExtensionsForDisplay))
		{
			deviceFound = true;
			raytracingSupported = false;
			deviceExtensions = &deviceExtensionsForDisplay;
		}

		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		if (deviceFound)
			printSelectedGPU();
		else
			throw std::runtime_error("failed to find a suitable GPU!");

		assert(deviceExtensions != NULL);
		createLogicalDevice(*deviceExtensions, raytracingSupported);

		vulkan_initialized = true;
	}

	void createLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions,
	                         const bool raytracingSupported)
	{
		// grab the required queue families
		ltracer::QueueFamilyIndices indices
		    = ltracer::findQueueFamilies(physicalDevice, window->getVkSurface());

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

		// filter the unique families, some families can use the same queue
		std::set<uint32_t> uniqueQueueFamilies = {};

		if (indices.graphicsFamily.has_value())
			uniqueQueueFamilies.insert(indices.graphicsFamily.value());
		if (indices.presentFamily.has_value())
			uniqueQueueFamilies.insert(indices.presentFamily.value());
		if (indices.transferFamily.has_value())
			uniqueQueueFamilies.insert(indices.transferFamily.value());

		// add the unique families into a list
		for (auto const queueFamily : uniqueQueueFamilies)
		{
			float queuePriority = 1.0f;

			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// =========================================================================
		// Physical Device Features
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR
		    physicalDeviceAccelerationStructureFeatures{};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures{};
		VkPhysicalDeviceFeatures deviceFeatures{};
		VkPhysicalDeviceDescriptorIndexingFeatures deviceDescriptorFeature = {};

		VkPhysicalDeviceSynchronization2Features synchronizationFeatures2{};
		synchronizationFeatures2.sType
		    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		synchronizationFeatures2.synchronization2 = true;

		if (raytracingSupported)
		{
			physicalDeviceBufferDeviceAddressFeatures.sType
			    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
			physicalDeviceBufferDeviceAddressFeatures.pNext = NULL;
			physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
			physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
			physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;

			physicalDeviceAccelerationStructureFeatures.sType
			    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			physicalDeviceAccelerationStructureFeatures.pNext
			    = &physicalDeviceBufferDeviceAddressFeatures;
			physicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
			physicalDeviceAccelerationStructureFeatures.accelerationStructureCaptureReplay
			    = VK_FALSE;
			physicalDeviceAccelerationStructureFeatures.accelerationStructureIndirectBuild
			    = VK_FALSE;
			physicalDeviceAccelerationStructureFeatures.accelerationStructureHostCommands
			    = VK_FALSE;
			physicalDeviceAccelerationStructureFeatures
			    .descriptorBindingAccelerationStructureUpdateAfterBind
			    = VK_FALSE;

			physicalDeviceRayTracingPipelineFeatures.sType
			    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
			physicalDeviceRayTracingPipelineFeatures.pNext
			    = &physicalDeviceAccelerationStructureFeatures;
			physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
			physicalDeviceRayTracingPipelineFeatures
			    .rayTracingPipelineShaderGroupHandleCaptureReplay
			    = VK_FALSE;
			physicalDeviceRayTracingPipelineFeatures
			    .rayTracingPipelineShaderGroupHandleCaptureReplayMixed
			    = VK_FALSE;
			physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
			physicalDeviceRayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_FALSE;

			deviceDescriptorFeature.sType
			    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
			// allow null in descriptor set
			deviceDescriptorFeature.descriptorBindingPartiallyBound = true;
			deviceDescriptorFeature.pNext = &physicalDeviceRayTracingPipelineFeatures;

			// link chain of pNext pointers
			synchronizationFeatures2.pNext = &deviceDescriptorFeature;

			// Needs to be enabled for Raytracing
			deviceFeatures.geometryShader = VK_TRUE;
			createInfo.pEnabledFeatures = &deviceFeatures;
		}

		createInfo.pNext = &synchronizationFeatures2;

		if (enableValidationLayer)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		// create the logical device
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		// get the handle to the graphics queue
		if (indices.graphicsFamily.has_value())
			vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
		debug_print("Graphics queue: %p\n", graphicsQueue);

		if (indices.presentFamily.has_value())
			vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
		debug_print("Present queue: %p\n", presentQueue);

		if (indices.transferFamily.has_value())
			vkGetDeviceQueue(logicalDevice, indices.transferFamily.value(), 0, &transferQueue);
		debug_print("Transfer queue: %p\n", transferQueue);
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice,
	                                 const std::vector<const char*> requiredDeviceExtensions)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(
		    physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		std::cout << "----------------------------\n";
		std::cout << "Device: " << properties.deviceName << "\n";
		std::cout << "Driver Version: " << properties.driverVersion << "\n";
		std::cout << "Api Version:" << properties.apiVersion << "\n";
		std::cout << "Available Extensions on device: \n";
		for (const auto& extension : availableExtensions)
		{
			// std::cout << "Name: " << extension.extensionName
			//           << " Version: " << extension.specVersion << '\n';
		}
		std::cout << std::endl;

		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(),
		                                         requiredDeviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		if (requiredExtensions.empty())
		{
			return true;
		}
		else
		{
			for (auto& extension : requiredExtensions)
			{
				std::printf("Device is missing Extension: %s\n", extension.c_str());
			}
			std::printf("Trying next device...\n");
		}
		return false;
	}

	[[nodiscard]] bool pickPhysicalDevice(const std::vector<const char*> requiredDeviceExtensions)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::cout << "Found " << deviceCount << " GPU devices\n";

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{

			// VkPhysicalDeviceProperties properties;
			// vkGetPhysicalDeviceProperties(logicalDevice, &properties);
			// std::cout << "Found device: " << properties.deviceName << std::endl;
			// if (strcmp(properties.deviceName, "NVIDIA GeForce GTX 1060") == 0) {
			//   std::cout << "skipping " << properties.deviceName << std::endl;
			//   continue;
			// }

			if (isDeviceSuitable(device, requiredDeviceExtensions))
			{
				physicalDevice = device;
				break;
			}
		}

		bool deviceFound = physicalDevice != VK_NULL_HANDLE;
		return deviceFound;
	}

	ltracer::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice)
	{
		ltracer::SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		    physicalDevice, window->getVkSurface(), &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(
		    physicalDevice, window->getVkSurface(), &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(
			    physicalDevice, window->getVkSurface(), &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(
		    physicalDevice, window->getVkSurface(), &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
			                                          window->getVkSurface(),
			                                          &presentModeCount,
			                                          details.presentModes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice physicalDevice,
	                      const std::vector<const char*> requiredDeviceExtensions)
	{
		ltracer::QueueFamilyIndices indices
		    = ltracer::findQueueFamilies(physicalDevice, window->getVkSurface());

		bool extensionsSupported
		    = checkDeviceExtensionSupport(physicalDevice, requiredDeviceExtensions);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			ltracer::SwapChainSupportDetails swapChainSupport
			    = querySwapChainSupport(physicalDevice);
			swapChainAdequate
			    = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// std::cout << "Indices.isComplete()" << indices.isComplete() <<
		// std::endl; std::cout << "extensionsSupported" << extensionsSupported <<
		// std::endl; std::cout << "isComplete" << swapChainAdequate << sd::endl;
		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{

		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		                             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayer) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		// We can pass in custom data, e.g. the HelloTriangleApplication object
		// this data pointer will then be provided inside the callback
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger)
		    != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	static VkResult
	CreateDebugUtilsMessengerEXT(VkInstance instance,
	                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                             const VkAllocationCallbacks* pAllocator,
	                             VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		    instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	                                          const VkDebugUtilsMessengerEXT debugMessenger,
	                                          const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		    instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	void mainLoop()
	{
		double delta = 0;
		double currentFrame = 0;
		double lastFrame = 0;

		while (!window->shouldClose())
		{
			glfwPollEvents();

			currentFrame = glfwGetTime();
			delta = glfwGetTime() - lastFrame;
			lastFrame = currentFrame;

			// static auto startTime = std::chrono::high_resolution_clock::now();

			// auto currentTime = std::chrono::high_resolution_clock::now();
			// float time = std::chrono::duration<float,
			// std::chrono::seconds::period>(
			//                  currentTime - startTime)
			//                  .count();

			renderer->updateViewProjectionMatrix(camera->getViewMatrix(),
			                                     camera->getProjectionMatrix());

			renderer->drawFrame(camera, delta);
			if (renderer->swapChainOutdated)
			{
				ltracer::SwapChainSupportDetails swapChainSupport
				    = querySwapChainSupport(physicalDevice);
				VkExtent2D extent = window->chooseSwapExtent(swapChainSupport.capabilities);

				if (extent.height > 0 && extent.width > 0)
				{
					vkDeviceWaitIdle(logicalDevice);
					renderer->cleanupFramebufferAndImageViews();
					window->recreateSwapChain(
					    physicalDevice, logicalDevice, extent, swapChainSupport);
					renderer->createImageViews(logicalDevice);
					renderer->createFramebuffers(logicalDevice, window);

					ltracer::QueueFamilyIndices indices
					    = ltracer::findQueueFamilies(physicalDevice, window->getVkSurface());
					renderer->recreateRaytracingImageAndImageView(indices);

					camera->updateScreenSize(extent.width, extent.height);
				}
			}
			renderer->swapChainOutdated = false;
		}
		vkDeviceWaitIdle(logicalDevice);
	}

	void cleanupApp()
	{
		// TODO: When abstracting stuff, implement it as a deletion queue

		vkDeviceWaitIdle(logicalDevice);

		renderer->cleanupRenderer();

		window->cleanupSwapChain(logicalDevice);
		mainDeletionQueue.flush();

		vkDestroyDevice(logicalDevice, nullptr);

		if (enableValidationLayer)
		{
			DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(vulkanInstance, window->getVkSurface(), nullptr);
		vkDestroyInstance(vulkanInstance, nullptr);

		glfwDestroyWindow(window->getGLFWWindow());

		glfwTerminate();
	}

	void createInstanceForVulkan()
	{

		if (enableValidationLayer && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto glfwExtensions = getRequiredINSTANCEExtensions();

		// debug_print("%s\n", "Used extensions:");
		// for (const auto &extension : glfwExtensions) {
		//   debug_print("\t %s\n", extension);
		// }

		createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();

		VkValidationFeatureEnableEXT validationFeatureEnable[]
		    = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
		VkValidationFeaturesEXT features{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};

		// debugMessengerCreation initialization is put outside of if statement to
		// make sure its valid until the vkCreateInstance call below
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreation;
		if (enableValidationLayer)
		{
			populateDebugMessengerCreateInfo(debugMessengerCreation);

			features.disabledValidationFeatureCount = 0;
			features.enabledValidationFeatureCount = 1;
			features.pDisabledValidationFeatures = nullptr;
			features.pEnabledValidationFeatures = validationFeatureEnable;
			features.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreation;

			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.pNext = &features;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	std::vector<const char*> getRequiredINSTANCEExtensions()
	{
		uint32_t glfwExtensionsCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
		if (enableValidationLayer)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL
	debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	              VkDebugUtilsMessageTypeFlagsEXT messageType,
	              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	              void* pUserData)
	{
		auto errMsg = std::format("Validation layer ({}): {}\n",
		                          string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
		                          pCallbackData->pMessage);
		std::fprintf(stderr, "%s", errMsg.c_str());
		if (messageSeverity >= VkDebugUtilsMessageSeverityFlagBitsEXT::
		        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			throw new std::runtime_error(errMsg);
		return VK_FALSE;
	}
};

int main()
{

#ifdef NDEBUG
	std::cout << "Running NOT in debug mode" << '\n';
#else
	std::cout << "Running in debug mode" << '\n';
#endif

	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
