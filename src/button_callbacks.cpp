#include <cstdio>
#include <utility>

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

namespace ltracer
{
namespace rt
{
static void shootRay(Renderer& renderer, const Camera& camera, ui::UIData& uiData)
{
	glm::vec2 initialGuess = vec2(0.3, 0.3);

	auto& raytracingScene = renderer.getRaytracingScene();
	glm::vec3 intersectionPoint = glm::vec3(0);

	Ray ray{
	    .origin = camera.transform.getPos(),
	    .direction = camera.transform.getForward(),
	};

	// TODO: test out other definition for normals
	// Visualize planes
	glm::vec3 v = (glm::abs(ray.direction.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
	                                                  : glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 n1 = normalize(cross(ray.direction, v));
	glm::vec3 n2 = normalize(cross(ray.direction, n1));

	raytracingScene.getWorldObjectSpheres().clear();
	for (size_t side = 0; side < 4; side++)
	{
		if (ltracer::rt::newtonsMethodTriangle2(
		        raytracingScene,
		        renderer.getRaytracingDataConstants(),
		        intersectionPoint,
		        initialGuess,
		        ray.origin,
		        std::to_array(raytracingScene.getWorldObjectBezierTriangles()[side]
		                          .getGeometry()
		                          .getData()
		                          .controlPoints),
		        n1,
		        n2))
		{
			raytracingScene.addObjectSphere(intersectionPoint, 0.2f, ColorIdx::t_red);
			break;
		}
	}
	uiData.recreateAccelerationStructures.recreate = true;
	uiData.recreateAccelerationStructures.fullRebuild = true;
};

static void visualizeRayPlanes(Renderer& renderer, const Camera& camera, ui::UIData& uiData)
{
	auto& raytracingScene = renderer.getRaytracingScene();

	Ray ray{
	    .origin = camera.transform.getPos(),
	    .direction = camera.transform.getForward(),
	};
	glm::vec3 v = (glm::abs(ray.direction.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
	                                                  : glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 n1 = normalize(cross(ray.direction, v));
	glm::vec3 n2 = normalize(cross(ray.direction, n1));

	raytracingScene.getWorldObjectSpheres().clear();
	ltracer::rt::visualizePlane(raytracingScene, n1, camera.transform.getPos(), 1, 1);
	ltracer::rt::visualizePlane(raytracingScene, n2, camera.transform.getPos(), 1, 1);

	uiData.recreateAccelerationStructures.recreate = true;
	uiData.recreateAccelerationStructures.fullRebuild = true;
};

static void visualizeSlicingPlanes(Renderer& renderer, ui::UIData& uiData)
{
	auto& raytracingScene = renderer.getRaytracingScene();
	auto& slicingPlane = raytracingScene.getSlicingPlanes()[0];

	raytracingScene.getWorldObjectSpheres().clear();
	ltracer::rt::visualizePlane(
	    raytracingScene, slicingPlane.normal, slicingPlane.planeOrigin, 2.5, 2.5);

	uiData.recreateAccelerationStructures.recreate = true;
	uiData.recreateAccelerationStructures.fullRebuild = true;
};

void registerButtonFunctions(Window& window,
                             Renderer& renderer,
                             const Camera& camera,
                             ui::UIData& uiData)
{
	uiData.buttonCallbacks.push_back(
	    std::make_pair("[R] CPU Raytrace", [&]() { shootRay(renderer, camera, uiData); }));
	registerKeyListener(window,
	                    GLFW_KEY_R,
	                    ltracer::KeyTriggerMode::KeyDown,
	                    ltracer::KeyListeningMode::FLYING_CAMERA,
	                    [&]() { shootRay(renderer, camera, uiData); });

	uiData.buttonCallbacks.push_back(std::make_pair(
	    "[T] Visualize Ray Planes", [&]() { visualizeRayPlanes(renderer, camera, uiData); }));
	registerKeyListener(window,
	                    GLFW_KEY_T,
	                    ltracer::KeyTriggerMode::KeyDown,
	                    ltracer::KeyListeningMode::FLYING_CAMERA,
	                    [&]() { visualizeRayPlanes(renderer, camera, uiData); });

	uiData.buttonCallbacks.push_back(std::make_pair(
	    "Visualize Slicing Planes", [&]() { visualizeSlicingPlanes(renderer, uiData); }));
}
} // namespace rt
} // namespace ltracer
