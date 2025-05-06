#include <cstdio>
#include <utility>
#include <format>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float3.hpp>

#include "renderer.hpp"
#include "window.hpp"
#include "ui.hpp"
#include "common_types.h"
#include "renderer.hpp"
#include "raytracing_scene.hpp"
#include "input.hpp"
#include "visualizations.hpp"

namespace tracer
{
namespace rt
{

/// NOTE: this is not working generally for every degree so don't use it, it was just for
/// debugging anyway
// static void shootRay(Renderer& renderer, const Camera& camera, ui::UIData& uiData)
// {
// 	glm::vec2 initialGuess = vec2(0.3, 0.3);

// 	auto& raytracingScene = renderer.getRaytracingScene();
// 	glm::vec3 intersectionPoint = glm::vec3(0);

// 	Ray ray{
// 	    .origin = camera.transform.getPos(),
// 	    .direction = camera.transform.getForward(),
// 	};

// 	// TODO: test out other definition for normals
// 	// Visualize planes
// 	glm::vec3 v = (glm::abs(ray.direction.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
// 	                                                  : glm::vec3(1.0f, 0.0f, 0.0f);
// 	glm::vec3 n1 = normalize(cross(ray.direction, v));
// 	glm::vec3 n2 = normalize(cross(ray.direction, n1));

// 	for (size_t side = 0; side < 4; side++)
// 	{
// 		// clear the spheres before we test a side so only corresponding spheres for that side
// 		// remain
// 		raytracingScene.getWorldObjectSpheres().clear();
// 		if (tracer::rt::newtonsMethodTriangle2(
// 		        raytracingScene,
// 		        renderer.getRaytracingDataConstants(),
// 		        intersectionPoint,
// 		        initialGuess,
// 		        ray.origin,
// 		        raytracingScene.getWorldObjectBezierTriangles<BezierTriangle2>()[side]
// 		            .getGeometry()
// 		            .getData(),
// 		        n1,
// 		        n2))
// 		{
// 			raytracingScene.addObjectSphere(intersectionPoint, 0.2f, ColorIdx::t_red);
// 			break;
// 		}
// 	}
// 	uiData.recreateAccelerationStructures.requestRecreate(true);
// };

static void visualizeRayPlanes(Renderer& renderer, const Camera& camera, ui::UIData& uiData)
{
	auto& raytracingScene = renderer.getCurrentRaytracingScene();

	Ray ray{
	    .origin = camera.transform.getPos(),
	    .direction = camera.transform.getForward(),
	};
	glm::vec3 n1;
	glm::vec3 n2;

	float dx = ray.direction.x;
	float dy = ray.direction.y;
	float dz = ray.direction.z;
	if (glm::abs(dx) > glm::abs(dy) && glm::abs(dx) > glm::abs(dz))
	{
		n1 = normalize(vec3(dy, -dx, 0));
	}
	else
	{
		n1 = normalize(vec3(0, dz, -dy));
	}
	n2 = normalize(cross(ray.direction, n1));

	tracer::rt::visualizePlane(raytracingScene, n1, camera.transform.getPos(), 1, 1);
	tracer::rt::visualizePlane(raytracingScene, n2, camera.transform.getPos(), 1, 1);

	uiData.recreateAccelerationStructures.requestRecreate(true);
};

static void visualizeSlicingPlanes(Renderer& renderer, ui::UIData& uiData)
{
	auto& raytracingScene = renderer.getCurrentRaytracingScene();
	auto& slicingPlane = raytracingScene.getSlicingPlanes()[0];

	tracer::rt::visualizePlane(
	    raytracingScene, slicingPlane.normal, slicingPlane.planeOrigin, 2.5, 2.5);

	uiData.recreateAccelerationStructures.requestRecreate(true);
};

static void loadScene(Renderer& renderer, ui::UIData& uiData, const int sceneNr)
{
	auto& raytracingScene = renderer.getCurrentRaytracingScene();
	auto& sceneConfig = tracer::SceneConfig::fromUIData(uiData);

	std::printf("Loading scene %d\n", sceneNr);
	RaytracingScene::loadScene(renderer, raytracingScene, sceneConfig, sceneNr);

	uiData.recreateAccelerationStructures.requestRecreate(true);
};

void registerButtonFunctions(Window& window, Renderer& renderer, Camera& camera, ui::UIData& uiData)
{

	auto takeScreenshot = [&]()
	{
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
		auto timestamp = oss.str();

		float frametime = uiData.frameTimeMilliseconds;
		int num_start_guesses = uiData.raytracingDataConstants.newtonGuessesAmount;
		int width = window.getWidth();
		int height = window.getHeight();
		std::filesystem::create_directories("screenshots");
		std::filesystem::path path = std::format("screenshots/screenshot_{}_{}x{}_{}G_{}ms.ppm",
		                                         timestamp,
		                                         width,
		                                         height,
		                                         num_start_guesses,
		                                         frametime);
		renderer.saveFrameToFile(path);

		std::cout << "Screenshot saved to disk: " << std::filesystem::absolute(path) << std::endl;
	};
	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "[B] Take Screenshot",
	    .tooltip = "Saves the current raytraicng image to a .ppm file (without the UI) in the "
	               "directory ./screenshots/",
	    .callback = takeScreenshot,
	});
	registerKeyListener(window,
	                    GLFW_KEY_B,
	                    tracer::KeyTriggerMode::KeyDown,
	                    tracer::KeyListeningMode::UI_AND_FLYING_CAMERA,
	                    takeScreenshot);

	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "Set Window Resolution 1920x1080",
	    .tooltip = "Sets the window Resolution to 1920x1080",
	    .callback = [&]() { window.setPreferredSize(1920, 1080); },
	});
	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "Set Window Resolution 1280x720",
	    .tooltip = "Sets the window Resolution to 1280x720",
	    .callback = [&]() { window.setPreferredSize(1080, 720); },
	});

	auto openUI = [&]() { uiData.mainPanelCollapsed = !uiData.mainPanelCollapsed; };
	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "[C] [C]ollapse/Uncollapse UI",
	    .tooltip = "Collapes or uncollapses the main menu",
	    .callback = openUI,
	});
	registerKeyListener(window,
	                    GLFW_KEY_C,
	                    tracer::KeyTriggerMode::KeyDown,
	                    tracer::KeyListeningMode::UI_AND_FLYING_CAMERA,
	                    openUI);

	// uiData.buttonCallbacks.push_back(ui::ButtonData{
	//      .label = "[R] [Debug] CPU Raytrace",
	//      .tooltip = "Shoots a ray into the scene and prints the calculations to the console. Only
	//      "
	//                 "used for debugging.",
	//      .callback = [&]() { shootRay(renderer, camera, uiData); },
	//  });
	//  registerKeyListener(window,
	//                      GLFW_KEY_R,
	//                      tracer::KeyTriggerMode::KeyDown,
	//                      tracer::KeyListeningMode::FLYING_CAMERA,
	//                      [&]() { shootRay(renderer, camera, uiData); });

	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "[T] Visualize Ray Planes",
	    .tooltip
	    = "Visualizes the crosshair ray as two planes, represented by spheres. For debugging only.",
	    .callback = [&]() { visualizeRayPlanes(renderer, camera, uiData); },
	});
	registerKeyListener(window,
	                    GLFW_KEY_T,
	                    tracer::KeyTriggerMode::KeyDown,
	                    tracer::KeyListeningMode::FLYING_CAMERA,
	                    [&]() { visualizeRayPlanes(renderer, camera, uiData); });

	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "Visualize Slicing Planes",
	    .tooltip = "Visualizes the slicing plane with spheres. For debugging only.",
	    .callback = [&]() { visualizeSlicingPlanes(renderer, uiData); },
	});

	auto moveCameraHome = [&]() { camera.resetPositionAndOrientation(); };
	uiData.buttonCallbacks.push_back(ui::ButtonData{
	    .label = "[H] move camera [H]ome",
	    .tooltip = "Resets the camera to its original position and orientation",
	    .callback = moveCameraHome,
	});
	registerKeyListener(window,
	                    GLFW_KEY_H,
	                    tracer::KeyTriggerMode::KeyDown,
	                    tracer::KeyListeningMode::FLYING_CAMERA,
	                    moveCameraHome);

	auto sceneCount = RaytracingScene::getSceneCount();
	for (int sceneNr = 1; sceneNr <= sceneCount; sceneNr++)
	{
		auto sceneName = RaytracingScene::getSceneName(sceneNr);
		auto label = std::format("[{}] Load Scene: {}", sceneNr, sceneName);
		uiData.buttonCallbacks.push_back(ui::ButtonData{
		    .label = label,
		    .tooltip = "",
		    .callback = [&, sceneNr]() { loadScene(renderer, uiData, sceneNr); },
		});
		registerKeyListener(window,
		                    GLFW_KEY_0 + sceneNr,
		                    tracer::KeyTriggerMode::KeyDown,
		                    tracer::KeyListeningMode::FLYING_CAMERA,
		                    [&, sceneNr]() { loadScene(renderer, uiData, sceneNr); });
	}
}
} // namespace rt
} // namespace tracer
