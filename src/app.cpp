#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <format>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

#include "deletion_queue.hpp"
#include "vk_mem_alloc.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float3.hpp>

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
#include "button_callbacks.hpp"
#include "common_types.h"
#include "visualizations.hpp"

#include <OpenVolumeMesh/FileManager/FileManager.hh>
#include "OpenVolumeMesh/Mesh/TetrahedralMesh.hh"
#include <OpenVolumeMesh/Mesh/PolyhedralMesh.hh>
#include <OpenVolumeMesh/Core/Entities.hh>

// #include "tiny_obj_loader.h"

#define TERM_COLOR_RED "\033[31m"
#define TERM_COLOR_RESET "\033[0m"

void Application::run()
{
	// loadModels();

	initWindow(window, framebufferResizeCallback);

	initVulkan();

	vmaAllocator = tracer::createVMAAllocator(physicalDevice, vulkanInstance, logicalDevice);

	raytracingScene = std::make_unique<tracer::rt::RaytracingScene>(
	    physicalDevice, logicalDevice, vmaAllocator);

	renderer = createRenderer(vulkanInstance,
	                          physicalDevice,
	                          logicalDevice,
	                          mainDeletionQueue,
	                          *raytracingScene,
	                          window,
	                          graphicsQueue,
	                          presentQueue,
	                          transferQueue,
	                          raytracingSupported,
	                          vmaAllocator,
	                          swapChainSupportDetails);

	uiData = std::make_unique<tracer::ui::UIData>(camera,
	                                              window,
	                                              raytracingSupported,
	                                              physicalDeviceProperties,
	                                              renderer->getRaytracingDataConstants(),
	                                              renderer->getFrameCount(),
	                                              renderer->getBLASInstancesCount(*raytracingScene),
	                                              raytracingScene->getSlicingPlanes(),
	                                              raytracingScene->getSceneObjects());

	customUserData = std::make_unique<tracer::CustomUserData>(vulkan_initialized,
	                                                          window,
	                                                          camera,
	                                                          *renderer,
	                                                          *uiData,
	                                                          swapChainSupportDetails,
	                                                          logicalDevice,
	                                                          physicalDevice);

	window.setWindowUserPointer(customUserData.get());
	initInputHandlers();

	setupScene();
	if (raytracingSupported)
	{
		tracer::rt::registerButtonFunctions(window, *renderer, camera, *uiData);
	}

	mainLoop();

	cleanupApp();
}

// std::shared_ptr<std::vector<tracer::WorldObject>> worldObjects
//     = std::make_shared<std::vector<tracer::WorldObject>>();

void Application::initWindow(tracer::Window& wnd, GLFWframebuffersizefun framebufferResizeCallback)
{
	wnd.initWindow(framebufferResizeCallback);
}

[[nodiscard]]
std::unique_ptr<tracer::Renderer>
Application::createRenderer(VkInstance vulkanInstance,
                            VkPhysicalDevice physicalDevice,
                            VkDevice logicalDevice,
                            tracer::DeletionQueue mainDeletionQueue,
                            tracer::rt::RaytracingScene& raytracingScene,
                            tracer::Window& window,
                            VkQueue graphicsQueue,
                            VkQueue presentQueue,
                            VkQueue transferQueue,
                            bool raytracingSupported,
                            VmaAllocator vmaAllocator,
                            tracer::SwapChainSupportDetails swapChainSupportDetails)
{
	int width, height;
	window.getFramebufferSize(&width, &height);
	std::unique_ptr<tracer::Renderer> renderer
	    = std::make_unique<tracer::Renderer>(physicalDevice,
	                                         logicalDevice,
	                                         mainDeletionQueue,
	                                         window,
	                                         graphicsQueue,
	                                         presentQueue,
	                                         transferQueue,
	                                         raytracingSupported,
	                                         vmaAllocator);

	window.createSwapChain(
	    physicalDevice,
	    logicalDevice,
	    VkExtent2D{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
	    swapChainSupportDetails);

	renderer->initRenderer(vulkanInstance, raytracingScene);
	return renderer;
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

bool Application::loadOpenVolumeMeshFile(std::filesystem::path path,
                                         tracer::rt::RaytracingScene& raytracingScene,
                                         [[maybe_unused]] tracer::Renderer& renderer,
                                         const tracer::SceneConfig sceneConfig)
{
	static auto filemanager = OpenVolumeMesh::IO::FileManager();
	OpenVolumeMesh::GeometricTetrahedralMeshV3d mesh;
	if (filemanager.readFile(path.string(), mesh))
	{
		std::printf("Mesh loaded\n");
	}
	else
	{
		std::printf("Error: Mesh failed to load!\n");
		return false;
	}

	std::cout << mesh.n_vertices() << " vertices" << std::endl;
	std::cout << mesh.n_cells() << " tetrahedra" << std::endl;

	auto propertyControlPointsOpt
	    = mesh.get_face_property<std::vector<double>>("BezierFaceControlPoints");
	auto propertyBezierDegreeOpt
	    = mesh.get_property<int, OpenVolumeMesh::Entity::Mesh>("BezierDegree");
	auto propertyIsBezierMesh
	    = mesh.get_property<bool, OpenVolumeMesh::Entity::Mesh>("IsBezierMesh");

	if (!propertyIsBezierMesh)
	{
		std::printf("ERROR: Property IsBezierMesh not found\n");
		return false;
	}

	int N = 0;
	if (propertyBezierDegreeOpt)
	{
		N = propertyBezierDegreeOpt.value().data_vector()[0];
		debug_printFmt("Found Tetrahedron degree: %d\n", N);
	}
	else
	{
		std::printf("ERROR: Propery BezierDegree not found\n");
		return false;
	}

	raytracingScene.clearScene();

	// first sphere represents light
	auto sceneObjectLight = raytracingScene.createNamedSceneObject(
	    "light", renderer.getRaytracingDataConstants().globalLightPosition);
	raytracingScene.addObjectSphere(*sceneObjectLight,
	                                renderer.getRaytracingDataConstants().globalLightPosition,
	                                true,
	                                0.2f,
	                                ColorIdx::t_yellow);

	renderer.setCurrentLightSceneObject(sceneObjectLight);
	auto sceneObject = raytracingScene.createNamedSceneObject("model");

	// NOTE: we can't  add elements to sceneObjects out of order, therefore to show the
	// controlpoints, we will add them after adding all bezier triangles to the sceneObject 'model'
	// Although this is every inefficient, its fine since we only do it when loading the model once
	auto allControlPoints = std::vector<glm::vec3>();

	auto allBezierTriangles2 = std::vector<BezierTriangle2>();
	auto allBezierTriangles3 = std::vector<BezierTriangle3>();
	auto allBezierTriangles4 = std::vector<BezierTriangle4>();

	// if we found the control points property
	if (propertyControlPointsOpt)
	{
		auto controlPointsVectors = propertyControlPointsOpt.value();

		// TODO: temporary limit
		// const int triangle_max = 30;
		// int triangles_added = 0;

		// each vector represents one face - data is the control points as 3 doubles
		for (auto it = controlPointsVectors.begin(); it != controlPointsVectors.end(); it++)
		{
			std::vector<glm::vec3> faceControlPoints;

			for (auto controlPoint = it->begin(); controlPoint != it->end();)
			{
				// extract 3 values at a time and convert to vec3
				auto x = *controlPoint;
				controlPoint++;
				auto y = *controlPoint;
				controlPoint++;
				auto z = *controlPoint;
				controlPoint++;

				auto point = glm::vec3(x, y, z);
				faceControlPoints.push_back(point);
			}

			size_t num_control_points = faceControlPoints.size();

			// convert the control points to the correct bezier triangle type
			if (N == 1)
			{
				if (num_control_points != 3)
				{
					std::printf("ERROR: BezierTriangle1 requires 3 control points\n");
					continue;
				}
				// 	auto bezierTriangle = BezierTriangle1{};
				// 	bezierTriangle.controlPoints[0] = faceControlPoints[2];
				// 	bezierTriangle.controlPoints[1] = faceControlPoints[1];
				// 	bezierTriangle.controlPoints[2] = faceControlPoints[0];
				// 	raytracingScene.addObjectBezierTriangle(bezierTriangle);

				// TODO: implement BezierTriangle1
				throw std::runtime_error("BezierTriangle1 not implemented");
			}
			else if (N == 2)
			{
				if (num_control_points != 6)
				{
					std::printf("ERROR: BezierTriangle2 requires 6 control points\n");
					continue;
				}

				auto bezierTriangle = BezierTriangle2{};
				// we need to flip the order here so the lighting calculation is correct - the
				// normal of the face is inverted otherwise - we could also inverse the normal
				// calculation but everything else already uses the current normal direction
				bezierTriangle.controlPoints[0] = faceControlPoints[2];
				bezierTriangle.controlPoints[1] = faceControlPoints[1];
				bezierTriangle.controlPoints[2] = faceControlPoints[0];

				bezierTriangle.controlPoints[3] = faceControlPoints[4];
				bezierTriangle.controlPoints[4] = faceControlPoints[3];

				bezierTriangle.controlPoints[5] = faceControlPoints[5];

				auto aabb = tracer::AABB::fromBezierTriangle(bezierTriangle);
				bezierTriangle.aabb = Aabb{aabb.min, aabb.max};
				raytracingScene.addObjectBezierTriangle(*sceneObject, bezierTriangle, false);
				allBezierTriangles2.push_back(bezierTriangle);
			}
			else if (N == 3)
			{
				if (num_control_points != 10)
				{
					std::printf("ERROR: BezierTriangle3 requires 10 control points\n");
					continue;
				}

				auto bezierTriangle = BezierTriangle3{};
				// we need to flip the order here so the lighting calculation is correct - the
				// normal of the face is inverted otherwise - we could also inverse the normal
				// calculation but everything else already uses the current normal direction
				bezierTriangle.controlPoints[0] = faceControlPoints[3];
				bezierTriangle.controlPoints[1] = faceControlPoints[2];
				bezierTriangle.controlPoints[2] = faceControlPoints[1];
				bezierTriangle.controlPoints[3] = faceControlPoints[0];

				bezierTriangle.controlPoints[4] = faceControlPoints[6];
				bezierTriangle.controlPoints[5] = faceControlPoints[5];
				bezierTriangle.controlPoints[6] = faceControlPoints[4];

				bezierTriangle.controlPoints[7] = faceControlPoints[8];
				bezierTriangle.controlPoints[8] = faceControlPoints[7];

				bezierTriangle.controlPoints[9] = faceControlPoints[9];

				auto aabb = tracer::AABB::fromBezierTriangle(bezierTriangle);
				bezierTriangle.aabb = Aabb{aabb.min, aabb.max};
				raytracingScene.addObjectBezierTriangle(*sceneObject, bezierTriangle, false);
				allBezierTriangles3.push_back(bezierTriangle);
			}
			else if (N == 4)
			{
				if (num_control_points != 15)
				{
					std::printf("ERROR: BezierTriangle3 requires 15 control points\n");
					continue;
				}
				auto bezierTriangle = BezierTriangle4{};
				// we need to flip the order here so the lighting calculation is correct - the
				// normal of the face is inverted otherwise - we could also inverse the normal
				// calculation but everything else already uses the current normal direction
				bezierTriangle.controlPoints[0] = faceControlPoints[4];
				bezierTriangle.controlPoints[1] = faceControlPoints[3];
				bezierTriangle.controlPoints[2] = faceControlPoints[2];
				bezierTriangle.controlPoints[3] = faceControlPoints[1];
				bezierTriangle.controlPoints[4] = faceControlPoints[0];

				bezierTriangle.controlPoints[5] = faceControlPoints[8];
				bezierTriangle.controlPoints[6] = faceControlPoints[7];
				bezierTriangle.controlPoints[7] = faceControlPoints[6];
				bezierTriangle.controlPoints[8] = faceControlPoints[5];

				bezierTriangle.controlPoints[9] = faceControlPoints[11];
				bezierTriangle.controlPoints[10] = faceControlPoints[10];
				bezierTriangle.controlPoints[11] = faceControlPoints[9];

				bezierTriangle.controlPoints[12] = faceControlPoints[13];
				bezierTriangle.controlPoints[13] = faceControlPoints[12];

				bezierTriangle.controlPoints[14] = faceControlPoints[14];

				auto aabb = tracer::AABB::fromBezierTriangle(bezierTriangle);
				bezierTriangle.aabb = Aabb{aabb.min, aabb.max};
				raytracingScene.addObjectBezierTriangle(*sceneObject, bezierTriangle, false);
				allBezierTriangles4.push_back(bezierTriangle);
			}
			else
			{
				std::printf("ERROR: BezierTriangle not implemented for degree %d\n", N);
			}

			if (sceneConfig.visualizeControlPoints)
			{
				for (auto& point : faceControlPoints)
				{
					allControlPoints.push_back(point);
					// printf("Control point: (%f, %f, %f)\n", point.x, point.y, point.z);
				}
			}

			// triangles_added++;
			// if (triangles_added > triangle_max) break;
		}
	}

	if (sceneConfig.visualizeControlPoints)
	{
		auto sceneObjectControlPoints = raytracingScene.createSceneObject();
		for (auto& point : allControlPoints)
		{
			raytracingScene.addObjectSphere(
			    *sceneObjectControlPoints, point, false, 0.2f, ColorIdx::t_orange);
		}
	}

	auto sceneObjectControlPoints = raytracingScene.createSceneObject();
	for (auto& bezierTriangle : allBezierTriangles2)
	{
		tracer::rt::visualizeTriangleSide(*sceneObjectControlPoints,
		                                  raytracingScene,
		                                  bezierTriangle,
		                                  sceneConfig.visualizeSampledSurface);
	}

	for (auto& bezierTriangle : allBezierTriangles3)
	{
		tracer::rt::visualizeTriangleSide(*sceneObjectControlPoints,
		                                  raytracingScene,
		                                  bezierTriangle,
		                                  sceneConfig.visualizeSampledSurface);
	}

	for (auto& bezierTriangle : allBezierTriangles4)
	{
		tracer::rt::visualizeTriangleSide(*sceneObjectControlPoints,
		                                  raytracingScene,
		                                  bezierTriangle,
		                                  sceneConfig.visualizeSampledSurface);
	}

	// auto cells = mesh.cells();
	// for (auto cellItr = cells.first; cellItr != cells.second; cellItr++)
	// {
	// 	auto faces = mesh.cell_faces(*cellItr);
	// 	for (auto facePtr = faces.first; facePtr != faces.second; facePtr++)
	// 	{
	// 		auto face = mesh.face(*facePtr);

	// 		auto halfedges = face.halfedges();
	// 		for (auto& halfedgePtr : halfedges)
	// 		{
	// 			auto halfedge = mesh.halfedge(halfedgePtr);
	// 			auto vertPtrFrom = halfedge.from_vertex();
	// 			auto vertPtrTo = halfedge.to_vertex();
	// 			auto vert1 = mesh.vertex(vertPtrFrom);
	// 			auto vert2 = mesh.vertex(vertPtrTo);

	// 			auto v1 = glm::vec3(vert1[0], vert1[1], vert1[2]);
	// 			auto v2 = glm::vec3(vert2[0], vert2[1], vert2[2]);
	// raytracingScene.addObjectSphere(v1, 0.2f, ColorIdx::t_red);
	// raytracingScene.addObjectSphere(v2, 0.2f, ColorIdx::t_yellow);

	// float step = 0.1f;
	// auto dir = v2 - v1;
	// for (float a = 0.0; a <= 1.0f + 1e-5f; a += step)
	// {
	// auto v = v1 + dir * a;
	// raytracingScene.addObjectSphere(v, 0.1f, ColorIdx::t_green);
	// }

	// std::printf("face %d:  halfedge %d from: %d: (%f, %f, %f) -> %d: (%f,%f,%f)\n",
	//             facePtr->idx(),
	//             halfedgePtr.idx(),
	//             vertPtrFrom.idx(),
	//             vert1[0],
	//             vert1[1],
	//             vert1[2],
	//             vertPtrTo.idx(),
	//             vert2[0],
	//             vert2[1],
	//             vert2[2]);
	// 		}
	// 	}
	// }
	return true;
}

void Application::handle_dropped_file(GLFWwindow* window, const std::string path)
{
	tracer::CustomUserData& userData
	    = *reinterpret_cast<tracer::CustomUserData*>(glfwGetWindowUserPointer(window));

	auto& renderer = userData.renderer;
	auto sceneConfig = tracer::SceneConfig::fromUIData(userData.uiData);
	auto& raytracingScene = renderer.getCurrentRaytracingScene();

	std::printf("Loading OpenVolumeMesh model from file: %s\n", path.c_str());

	bool loadingSuccessful
	    = Application::loadOpenVolumeMeshFile(path, raytracingScene, renderer, sceneConfig);

	// =========================================================================
	// Rebuild Bottom and Top Level Acceleration Structure
	if (loadingSuccessful && renderer.getRaytracingSupported())
	{
		raytracingScene.recreateAccelerationStructures(renderer.getRaytracingInfo(), true);

		// =========================================================================
		// Update Descriptor Set
		tracer::rt::updateAccelerationStructureDescriptorSet(
		    userData.logicalDevice, raytracingScene, renderer.getRaytracingInfo());
	}
}

void Application::setupScene()
{
	auto sceneConfig = tracer::SceneConfig::fromUIData(*uiData);

	tracer::rt::RaytracingScene::loadScene(
	    *renderer, *raytracingScene, sceneConfig, raytracingScene->getCurrentSceneNr());

	// =========================================================================
	// Rebuild Bottom and Top Level Acceleration Structure
	if (raytracingSupported)
	{
		vkQueueWaitIdle(renderer->getRaytracingInfo().graphicsQueueHandle);
		raytracingScene->recreateAccelerationStructures(renderer->getRaytracingInfo(), true);

		// =========================================================================
		// Update Descriptor Set
		tracer::rt::updateAccelerationStructureDescriptorSet(
		    logicalDevice, *raytracingScene, renderer->getRaytracingInfo());
	}

	// // TODO: move this to a more appropriate place
	// if (renderer->getRaytracingScene().getWorldObjectTetrahedrons().size() > 0)
	// {
	// 	auto& tetrahedron = renderer->getRaytracingScene().getWorldObjectTetrahedrons()[0];
	// 	for (size_t i = 0; i < uiData->positions.size(); i++)
	// 	{
	// 		uiData->positions[i] = tetrahedron.getGeometry().getData().controlPoints[i];
	// 	}
	// }
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

// tracer::WorldObject& loadModel(std::filesystem::path modelPath,
//                                 glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f},
//                                 glm::vec3 position = glm::vec3(0, 0, 0))
// {
// 	objl::Loader modelLoader;
// 	bool loadout = modelLoader.LoadFile(modelPath.string());

// 	if (loadout)
// 	{
// 		std::vector<tracer::Vertex> vertices;
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
	glfwSetKeyCallback(window.getGLFWWindow(), &tracer::handleInputCallback);
	glfwSetMouseButtonCallback(window.getGLFWWindow(), &tracer::handleMouseInputCallback);
	glfwSetCursorPosCallback(window.getGLFWWindow(), &tracer::handleMouseMovementCallback);
	glfwSetScrollCallback(window.getGLFWWindow(), &tracer::handleMouseScrollCallback);

	// additional callbacks only needed by Imgui
	glfwSetWindowFocusCallback(window.getGLFWWindow(), ImGui_ImplGlfw_WindowFocusCallback);
	glfwSetCursorEnterCallback(window.getGLFWWindow(), ImGui_ImplGlfw_CursorEnterCallback);
	glfwSetCharCallback(window.getGLFWWindow(), ImGui_ImplGlfw_CharCallback);
	glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

	// TODO: move lambda into input.cpp file
	glfwSetDropCallback(window.getGLFWWindow(),
	                    [](GLFWwindow* window, int count, const char** path)
	                    {
		                    if (count > 1)
		                    {
			                    std::printf(
			                        "ERROR: Only one file can be loaded. Loading first file\n");
		                    }

		                    if (count > 0)
		                    {
			                    std::string pathStr = path[0];
			                    Application::handle_dropped_file(window, pathStr);
		                    }
	                    });
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
	else
	{
		throw std::runtime_error("Application::initVulkan - failed to find a suitable GPU!");
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
	tracer::QueueFamilyIndices indices
	    = tracer::findQueueFamilies(physicalDevice, window.getVkSurface());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// query the features of the device
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	// filter the unique families, some families can use the same queue
	std::set<uint32_t> uniqueQueueFamilies = {};

	if (indices.graphicsFamily.has_value())
		uniqueQueueFamilies.insert(indices.graphicsFamily.value());
	if (indices.presentFamily.has_value())
		uniqueQueueFamilies.insert(indices.presentFamily.value());
	// if (indices.transferFamily.has_value())
	// 	uniqueQueueFamilies.insert(indices.transferFamily.value());

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

	// features vulkan enabled by default that are enabled here explicitly
	deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
	deviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
	deviceFeatures.shaderInt64 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeature = {};
	timelineSemaphoreFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	timelineSemaphoreFeature.timelineSemaphore = VK_TRUE;
	timelineSemaphoreFeature.pNext = nullptr;

	VkPhysicalDeviceVulkanMemoryModelFeatures memoryModelFeature = {};
	memoryModelFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES;
	memoryModelFeature.vulkanMemoryModelDeviceScope = VK_TRUE;
	memoryModelFeature.vulkanMemoryModel = VK_TRUE;
	memoryModelFeature.pNext = &timelineSemaphoreFeature;

	VkPhysicalDevice8BitStorageFeatures storage8BitFeature = {};
	storage8BitFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
	storage8BitFeature.storageBuffer8BitAccess = VK_TRUE;
	storage8BitFeature.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	storage8BitFeature.pNext = &memoryModelFeature;

	// Features that are always needed:
	VkPhysicalDeviceScalarBlockLayoutFeatures deviceScalarBlockLayoutFeature = {};
	deviceScalarBlockLayoutFeature.sType
	    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
	deviceScalarBlockLayoutFeature.scalarBlockLayout = VK_TRUE;
	deviceScalarBlockLayoutFeature.pNext = &storage8BitFeature;

	VkPhysicalDeviceSynchronization2Features synchronizationFeatures2{};
	synchronizationFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	synchronizationFeatures2.synchronization2 = true;
	synchronizationFeatures2.pNext = &deviceScalarBlockLayoutFeature;

	// Features only needed when ray tracing is supported
	VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures{};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures{};
	VkPhysicalDeviceDescriptorIndexingFeatures deviceDescriptorFeature = {};
	if (raytracingSupported)
	{
		// Needs to be enabled for Raytracing
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

	createInfo.pEnabledFeatures = &deviceFeatures;

	// create the logical device
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	// get the handle to the graphics queue
	if (indices.graphicsFamily.has_value())
		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
	debug_printFmt("Graphics queue: %p\n", static_cast<void*>(graphicsQueue));

	if (indices.presentFamily.has_value())
		vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
	debug_printFmt("Present queue: %p\n", static_cast<void*>(presentQueue));

	// if (indices.transferFamily.has_value())
	// 	vkGetDeviceQueue(logicalDevice, indices.transferFamily.value(), 0, &transferQueue);
	// debug_print("Transfer queue: %p\n", static_cast<void*>(transferQueue));
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

#ifndef NDEBUG
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device, &properties);
		std::cout << "Checking Device: " << properties.deviceName << std::endl;
#endif

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
                                    tracer::Renderer& renderer,
                                    tracer::Window& window,
                                    tracer::Camera& camera,
                                    tracer::SwapChainSupportDetails& swapChainSupportDetails,
                                    const bool raytracingSupported)
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
		renderer.createRaytracingRenderpassAndFramebuffer();
		renderer.updateRaytracingDescriptorSet();

		camera.updateScreenSize(extent.width, extent.height);

		if (raytracingSupported)
		{
			tracer::rt::updateAccelerationStructureDescriptorSet(
			    logicalDevice, renderer.getCurrentRaytracingScene(), renderer.getRaytracingInfo());
		}

		// reset frame count so the window gets refreshed properly
		resetFrameCount(renderer.getRaytracingInfo());
	}
}

void Application::framebufferResizeCallback(GLFWwindow* window,
                                            [[maybe_unused]] int width,
                                            [[maybe_unused]] int height)
{
	tracer::CustomUserData& userData
	    = *reinterpret_cast<tracer::CustomUserData*>(glfwGetWindowUserPointer(window));
	if (!userData.vulkan_initialized) return;

	userData.swapChainSupportDetails
	    = updateSwapChainSupportDetails(userData.physicalDevice, userData.window);

	resizeFramebuffer(userData.physicalDevice,
	                  userData.logicalDevice,
	                  userData.renderer,
	                  userData.window,
	                  userData.camera,
	                  userData.swapChainSupportDetails,
	                  userData.uiData.raytracingSupported);
}

tracer::SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice physicalDevice,
                                                                   tracer::Window& window)
{
	tracer::SwapChainSupportDetails details;

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
	tracer::QueueFamilyIndices indices
	    = tracer::findQueueFamilies(physicalDeviceToCheck, window.getVkSurface());

	bool extensionsSupported
	    = checkDeviceExtensionSupport(physicalDeviceToCheck, requiredDeviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		tracer::SwapChainSupportDetails swapChainSupport
		    = querySwapChainSupport(physicalDeviceToCheck, window);
		swapChainAdequate
		    = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	debug_printFmt(
	    "Application::isDeviceSuitable - hasValue: graphicsFamily: %d presentFamily: %d\n",
	    indices.graphicsFamily.has_value(),
	    indices.presentFamily.has_value());
	debug_printFmt("Indices.isComplete(): %d extensionsSupported: %d swapChainAdequate: %d\n",
	               indices.isComplete(),
	               extensionsSupported,
	               swapChainAdequate);

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

		window.checkPreferredWindowSize();

		currentFrame = glfwGetTime();
		delta = glfwGetTime() - lastFrame;
		lastFrame = currentFrame;

		tracer::updateMovement(*customUserData, delta);

		renderer->updateViewProjectionMatrix(camera.getViewMatrix(), camera.getProjectionMatrix());

		// std::cout << "Frame count: " << renderer->getFrameCount() << std::endl;
		// if (renderer->getFrameCount() >= 10)
		// {
		// 	std::cout << "Reached limit for frames, exiting\n";
		// 	glfwSetWindowShouldClose(window.getGLFWWindow(), true);
		// }

		auto startTime = std::chrono::high_resolution_clock::now();

		renderer->drawFrame(camera, delta, *uiData);

		float frameTimeDelta = std::chrono::duration<float, std::chrono::milliseconds::period>(
		                           std::chrono::high_resolution_clock::now() - startTime)
		                           .count();
		uiData->frameTimeMilliseconds = frameTimeDelta;

		if (uiData->configurationChanged)
		{
			renderer->requestResetFrameCount();
			uiData->configurationChanged = false;
		}

		if (camera.isCameraMoved())
		{
			renderer->requestResetFrameCount();
			camera.resetCameraMoved();
		}

		if (uiData->sceneReloader.isReloadRequested())
		{
			setupScene();

			renderer->requestResetFrameCount();
			uiData->sceneReloader.resetSceneReloadRequest();
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
				renderer->createRaytracingRenderpassAndFramebuffer();

				renderer->updateRaytracingDescriptorSet();

				if (raytracingSupported)
				{
					tracer::rt::updateAccelerationStructureDescriptorSet(
					    logicalDevice, *raytracingScene, renderer->getRaytracingInfo());
				}

				camera.updateScreenSize(extent.width, extent.height);

				// reset frame count so the window gets refreshed properly
				resetFrameCount(renderer->getRaytracingInfo());
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

	raytracingScene->cleanup(graphicsQueue);
	renderer->cleanupRenderer();

	window.cleanupSwapChain(logicalDevice);
	mainDeletionQueue.flush();

	vmaDestroyAllocator(vmaAllocator);
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

	VkValidationFeatureEnableEXT validationFeatureEnable[] = {
	    VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
	    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	};

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
			features.enabledValidationFeatureCount = 3;
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
		throw std::runtime_error(errMsg);
	return VK_FALSE;
}
