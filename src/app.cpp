#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL

#include "app.hpp"
#include "logger.hpp"
#include "camera.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "ui.hpp"
#include "types.hpp"
#include "custom_user_data.hpp"
#include "input.hpp"
#include "raytracing_scene.hpp"
#include "visualizations.hpp"

// #include "tiny_obj_loader.h"

#define TERM_COLOR_RED "\033[31m"
#define TERM_COLOR_RESET "\033[0m"

void Application::run()
{
	// loadModels();

	createWindow();

	initVulkan();

	createRenderer();

	uiData = std::make_unique<ltracer::ui::UIData>(camera,
	                                               window,
	                                               raytracingSupported,
	                                               physicalDeviceProperties,
	                                               renderer->getRaytracingDataConstants(),
	                                               renderer->getFrameCount(),
	                                               renderer->getBLASInstancesCount(),
	                                               renderer->getRaytracingScene().getSlicingPlanes()

	);

	customUserData = std::make_unique<ltracer::CustomUserData>(vulkan_initialized,
	                                                           window,
	                                                           camera,
	                                                           *renderer,
	                                                           *uiData,
	                                                           swapChainSupportDetails,
	                                                           logicalDevice,
	                                                           physicalDevice);

	window.setWindowUserPointer(customUserData.get());
	initInputHandlers();

	if (raytracingSupported)
	{
		setupScene();
	}

	mainLoop();

	cleanupApp();
}

// std::shared_ptr<std::vector<ltracer::WorldObject>> worldObjects
//     = std::make_shared<std::vector<ltracer::WorldObject>>();

void Application::createWindow()
{
	window.initWindow(&framebufferResizeCallback);
}

void Application::createRenderer()
{
	int width, height;
	window.getFramebufferSize(&width, &height);
	renderer = std::make_unique<ltracer::Renderer>(physicalDevice,
	                                               logicalDevice,
	                                               mainDeletionQueue,
	                                               window,
	                                               graphicsQueue,
	                                               presentQueue,
	                                               transferQueue,
	                                               raytracingSupported);

	window.createSwapChain(
	    physicalDevice,
	    logicalDevice,
	    VkExtent2D{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
	    swapChainSupportDetails);

	renderer->initRenderer(vulkanInstance);
}

bool Application::checkValidationLayerSupport()
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

void Application::setupScene()
{
	ltracer::rt::RaytracingScene& raytracingScene = renderer->getRaytracingScene();

	for (int x = 0; x <= 0; x++)
	{
		for (int y = 0; y <= 0; y++)
		{
			float scalar = 3.0f;
			glm::vec3 offset
			    = glm::vec3(4.0f * static_cast<float>(x), 0, 4.0f * static_cast<float>(y));
			[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset,
			    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset,
			    glm::vec3(1.0f, 2.0f + static_cast<float>(x), 2.0f) * scalar + offset,
			    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset,

			    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset,
			    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset,
			    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset,

			    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset,
			    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset,
			    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset,
			}));

			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2, 1);
			ltracer::rt::visualizeTetrahedron2(raytracingScene, tetrahedron2);
		}
	}

	raytracingScene.addSlicingPlane(SlicingPlane{
	    glm::vec3(1.7, 0, 0),
	    glm::vec3(1, 0, 0),
	});

	raytracingScene.addSlicingPlane(SlicingPlane{
	    glm::vec3(0, 2, 0),
	    glm::vec3(0, 1, 0),
	});

	// =========================================================================
	// Bottom and Top Level Acceleration Structure
	raytracingScene.recreateAccelerationStructures(renderer->getRaytracingInfo(), true);

	// =========================================================================
	// Update Descriptor Set
	ltracer::rt::updateAccelerationStructureDescriptorSet(
	    logicalDevice, raytracingScene, renderer->getRaytracingInfo());

	// TODO: move this to a more appropriate place
	if (renderer->getRaytracingScene().getWorldObjectTetrahedrons().size() > 0)
	{
		auto& tetrahedron = renderer->getRaytracingScene().getWorldObjectTetrahedrons()[0];
		for (size_t i = 0; i < uiData->positions.size(); i++)
		{
			uiData->positions[i] = tetrahedron.getGeometry().getData().controlPoints[i];
		}
	}
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
// 			vertices.emplace_back(glm::vec3(vert.Position.X, vert.Position.Y,
// vert.Position.Z), 			                      color, glm::vec3(vert.Normal.X,
// vert.Normal.Y, vert.Normal.Z));
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
// 		                         + std::filesystem::absolute(modelPath).string() + "'
// file\n");
// 	}
// }

void Application::initInputHandlers()
{
	glfwSetKeyCallback(window.getGLFWWindow(), &ltracer::handleInputCallback);
	glfwSetMouseButtonCallback(window.getGLFWWindow(), &ltracer::handleMouseInputCallback);
	glfwSetCursorPosCallback(window.getGLFWWindow(), &ltracer::handleMouseMovementCallback);
	glfwSetScrollCallback(window.getGLFWWindow(), &ltracer::handleMouseScrollCallback);

	// additional callbacks only needed by Imgui
	glfwSetWindowFocusCallback(window.getGLFWWindow(), ImGui_ImplGlfw_WindowFocusCallback);
	glfwSetCursorEnterCallback(window.getGLFWWindow(), ImGui_ImplGlfw_CursorEnterCallback);
	glfwSetCharCallback(window.getGLFWWindow(), ImGui_ImplGlfw_CharCallback);
	glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
}

void Application::printSelectedGPU()
{
	std::cout << "Using device: " << physicalDeviceProperties.deviceName << std::endl;
}

void Application::initVulkan()
{
	createInstanceForVulkan();
	if (enableValidationLayer) setupDebugMessenger();
	window.createVulkanSurface(vulkanInstance);

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

	swapChainSupportDetails = updateSwapChainSupportDetails(physicalDevice, window);

	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	if (deviceFound)
		printSelectedGPU();
	else
		throw std::runtime_error("failed to find a suitable GPU!");

	assert(deviceExtensions != NULL);
	createLogicalDevice(*deviceExtensions);

	vulkan_initialized = true;
}

void Application::createLogicalDevice(const std::vector<const char*>& requiredDeviceExtensions)
{
	// grab the required queue families
	ltracer::QueueFamilyIndices indices
	    = ltracer::findQueueFamilies(physicalDevice, window.getVkSurface());

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

	// Features that are always needed:
	VkPhysicalDeviceScalarBlockLayoutFeatures deviceScalarBlockLayoutFeature = {};
	deviceScalarBlockLayoutFeature.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
	deviceScalarBlockLayoutFeature.scalarBlockLayout = VK_TRUE;

	VkPhysicalDeviceSynchronization2Features synchronizationFeatures2{};
	synchronizationFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	synchronizationFeatures2.synchronization2 = true;
	synchronizationFeatures2.pNext = &deviceScalarBlockLayoutFeature;

	// Features only needed when ray tracing is supported
	VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures{};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures{};
	VkPhysicalDeviceFeatures deviceFeatures{};
	VkPhysicalDeviceDescriptorIndexingFeatures deviceDescriptorFeature = {};
	if (raytracingSupported)
	{
		physicalDeviceBufferDeviceAddressFeatures.sType
		    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
		physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
		physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
		physicalDeviceBufferDeviceAddressFeatures.pNext = &synchronizationFeatures2;

		physicalDeviceAccelerationStructureFeatures.sType
		    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		physicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		physicalDeviceAccelerationStructureFeatures.accelerationStructureCaptureReplay = VK_FALSE;
		physicalDeviceAccelerationStructureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
		physicalDeviceAccelerationStructureFeatures.accelerationStructureHostCommands = VK_FALSE;
		physicalDeviceAccelerationStructureFeatures
		    .descriptorBindingAccelerationStructureUpdateAfterBind
		    = VK_FALSE;
		physicalDeviceAccelerationStructureFeatures.pNext
		    = &physicalDeviceBufferDeviceAddressFeatures;

		physicalDeviceRayTracingPipelineFeatures.sType
		    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay
		    = VK_FALSE;
		physicalDeviceRayTracingPipelineFeatures
		    .rayTracingPipelineShaderGroupHandleCaptureReplayMixed
		    = VK_FALSE;
		physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
		physicalDeviceRayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_FALSE;
		physicalDeviceRayTracingPipelineFeatures.pNext
		    = &physicalDeviceAccelerationStructureFeatures;

		deviceDescriptorFeature.sType
		    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		// allow null in descriptor set
		deviceDescriptorFeature.descriptorBindingPartiallyBound = true;
		deviceDescriptorFeature.pNext = &physicalDeviceRayTracingPipelineFeatures;

		// Needs to be enabled for Raytracing
		createInfo.pEnabledFeatures = &deviceFeatures;

		// link chain of pNext pointers
		createInfo.pNext = &deviceDescriptorFeature;
	}
	else
	{
		createInfo.pNext = &synchronizationFeatures2;
	}

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
	debug_print("Graphics queue: %p\n", static_cast<void*>(graphicsQueue));

	if (indices.presentFamily.has_value())
		vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
	debug_print("Present queue: %p\n", static_cast<void*>(presentQueue));

	if (indices.transferFamily.has_value())
		vkGetDeviceQueue(logicalDevice, indices.transferFamily.value(), 0, &transferQueue);
	debug_print("Transfer queue: %p\n", static_cast<void*>(transferQueue));
}

bool Application::checkDeviceExtensionSupport(
    VkPhysicalDevice physicalDeviceToCheck, const std::vector<const char*> requiredDeviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDeviceToCheck, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
	    physicalDeviceToCheck, nullptr, &extensionCount, availableExtensions.data());

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDeviceToCheck, &properties);

	// std::cout << "----------------------------\n";
	// std::cout << "Device: " << properties.deviceName << "\n";
	// std::cout << "Driver Version: " << properties.driverVersion << "\n";
	// std::cout << "Api Version:" << properties.apiVersion << "\n";
	// std::cout << "Available Extensions on device: \n";
	// for (const auto& extension : availableExtensions)
	// {
	// 	std::cout << "Name: " << extension.extensionName
	// 	          << " Version: " << extension.specVersion << '\n';
	// }
	// std::cout << std::endl;

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

[[nodiscard]] bool
Application::pickPhysicalDevice(const std::vector<const char*> requiredDeviceExtensions)
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

void Application::resizeFramebuffer(VkPhysicalDevice physicalDevice,
                                    VkDevice logicalDevice,
                                    ltracer::Renderer& renderer,
                                    ltracer::Window& window,
                                    ltracer::Camera& camera,
                                    ltracer::SwapChainSupportDetails& swapChainSupportDetails)
{
	VkExtent2D extent = window.chooseSwapExtent(swapChainSupportDetails.capabilities);
	if (extent.height > 0 && extent.width > 0)
	{
		vkDeviceWaitIdle(logicalDevice);
		renderer.cleanupFramebufferAndImageViews();
		window.recreateSwapChain(physicalDevice, logicalDevice, extent, swapChainSupportDetails);
		renderer.createImageViews();
		renderer.createFramebuffers();

		renderer.recreateRaytracingImageAndImageView();
		camera.updateScreenSize(extent.width, extent.height);
	}
}

void Application::framebufferResizeCallback(GLFWwindow* window,
                                            [[maybe_unused]] int width,
                                            [[maybe_unused]] int height)
{
	ltracer::CustomUserData& userData
	    = *reinterpret_cast<ltracer::CustomUserData*>(glfwGetWindowUserPointer(window));
	if (!userData.vulkan_initialized) return;

	userData.swapChainSupportDetails
	    = updateSwapChainSupportDetails(userData.physicalDevice, userData.window);

	resizeFramebuffer(userData.physicalDevice,
	                  userData.logicalDevice,
	                  userData.renderer,
	                  userData.window,
	                  userData.camera,
	                  userData.swapChainSupportDetails);
}

ltracer::SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice physicalDevice,
                                                                    ltracer::Window& window)
{
	ltracer::SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	    physicalDevice, window.getVkSurface(), &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(
	    physicalDevice, window.getVkSurface(), &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
		    physicalDevice, window.getVkSurface(), &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
	    physicalDevice, window.getVkSurface(), &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
		    physicalDevice, window.getVkSurface(), &presentModeCount, details.presentModes.data());
	}

	return details;
}

bool Application::isDeviceSuitable(VkPhysicalDevice physicalDeviceToCheck,
                                   const std::vector<const char*> requiredDeviceExtensions)
{
	ltracer::QueueFamilyIndices indices
	    = ltracer::findQueueFamilies(physicalDeviceToCheck, window.getVkSurface());

	bool extensionsSupported
	    = checkDeviceExtensionSupport(physicalDeviceToCheck, requiredDeviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		ltracer::SwapChainSupportDetails swapChainSupport
		    = querySwapChainSupport(physicalDeviceToCheck, window);
		swapChainAdequate
		    = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// std::cout << "Indices.isComplete()" << indices.isComplete() <<
	// std::endl; std::cout << "extensionsSupported" << extensionsSupported <<
	// std::endl; std::cout << "isComplete" << swapChainAdequate << sd::endl;
	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void Application::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

void Application::setupDebugMessenger()
{
	if (!enableValidationLayer) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	// We can pass in custom data, e.g. the HelloTriangleApplication object
	// this data pointer will then be provided inside the callback
	createInfo.pUserData = nullptr;

	if (createDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger)
	    != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VkResult
Application::createDebugUtilsMessengerEXT(VkInstance instance,
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

void Application::DestroyDebugUtilsMessengerEXT(VkInstance instance,
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

void Application::mainLoop()
{
	double delta = 0;
	double currentFrame = 0;
	double lastFrame = 0;

	while (!window.shouldClose())
	{
		glfwPollEvents();

		currentFrame = glfwGetTime();
		delta = glfwGetTime() - lastFrame;
		lastFrame = currentFrame;

		ltracer::updateMovement(*customUserData, delta);

		// static auto startTime = std::chrono::high_resolution_clock::now();

		// auto currentTime = std::chrono::high_resolution_clock::now();
		// float time = std::chrono::duration<float,
		// std::chrono::seconds::period>(
		//                  currentTime - startTime)
		//                  .count();

		renderer->updateViewProjectionMatrix(camera.getViewMatrix(), camera.getProjectionMatrix());

		// std::cout << "Frame count: " << renderer->getFrameCount() << std::endl;
		// if (renderer->getFrameCount() >= 10)
		// {
		// 	std::cout << "Reached limit for frames, exiting\n";
		// 	glfwSetWindowShouldClose(window.getGLFWWindow(), true);
		// }

		renderer->drawFrame(camera, delta, *uiData);

		if (uiData->configurationChanged || camera.isCameraMoved())
		{
			camera.resetCameraMoved();
			renderer->requestResetFrameCount();
		}

		if (renderer->swapChainOutdated)
		{
			VkExtent2D extent = window.chooseSwapExtent(swapChainSupportDetails.capabilities);

			if (extent.height > 0 && extent.width > 0)
			{
				vkDeviceWaitIdle(logicalDevice);
				renderer->cleanupFramebufferAndImageViews();
				window.recreateSwapChain(
				    physicalDevice, logicalDevice, extent, swapChainSupportDetails);
				renderer->createImageViews();
				renderer->createFramebuffers();

				renderer->recreateRaytracingImageAndImageView();

				camera.updateScreenSize(extent.width, extent.height);
			}
		}
		renderer->swapChainOutdated = false;
	}
	vkDeviceWaitIdle(logicalDevice);
}

void Application::cleanupApp()
{
	// TODO: When abstracting stuff, implement it as a deletion queue

	vkDeviceWaitIdle(logicalDevice);

	renderer->cleanupRenderer();

	window.cleanupSwapChain(logicalDevice);
	mainDeletionQueue.flush();

	vkDestroyDevice(logicalDevice, nullptr);

	if (enableValidationLayer)
	{
		DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(vulkanInstance, window.getVkSurface(), nullptr);
	vkDestroyInstance(vulkanInstance, nullptr);

	glfwDestroyWindow(window.getGLFWWindow());

	glfwTerminate();
}

void Application::createInstanceForVulkan()
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

	auto glfwExtensions = getRequiredInstanceExtensions();

	// debug_print("%s\n", "Used extensions:");
	// for (const auto &extension : glfwExtensions) {
	//   debug_print("\t %s\n", extension);
	// }

	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	VkValidationFeatureEnableEXT validationFeatureEnable[]
	    = {VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};

	VkValidationFeaturesEXT features{};
	features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;

	// debugMessengerCreation initialization is put outside of if statement to
	// make sure its valid until the vkCreateInstance call below
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreation;
	if (enableValidationLayer)
	{
		populateDebugMessengerCreateInfo(debugMessengerCreation);

		features.disabledValidationFeatureCount = 0;
		features.pDisabledValidationFeatures = nullptr;

		if (enableDebugShaderPrintf)
		{
			features.enabledValidationFeatureCount = 1;
			features.pEnabledValidationFeatures = validationFeatureEnable;
		}
		else
		{
			features.enabledValidationFeatureCount = 0;
			features.pEnabledValidationFeatures = nullptr;
		}

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

std::vector<const char*> Application::getRequiredInstanceExtensions()
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

VKAPI_ATTR VkBool32 VKAPI_CALL
Application::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                           [[maybe_unused]] void* pUserData)
{
	auto errMsg = std::format("Validation layer ({}): {}\n",
	                          string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity),
	                          pCallbackData->pMessage);
	std::fprintf(stderr, "%s", errMsg.c_str());
	if (messageSeverity
	    >= VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		throw new std::runtime_error(errMsg);
	return VK_FALSE;
}
