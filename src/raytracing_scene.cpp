
#include "raytracing_scene.hpp"
#include "renderer.hpp"
#include "visualizations.hpp"
#include <random>

namespace tracer
{
namespace rt
{

static glm::vec3 getRandomOffset(const int min, const int max)
{
	std::random_device rd;
	std::uniform_int_distribution<> dist(min, max);
	return glm::vec3(dist(rd), dist(rd), dist(rd));
}

/// Loads the scene with the given index. A manual rebuild of the acceleration
/// structure is still required
void RaytracingScene::loadScene([[maybe_unused]] const Renderer& renderer,
                                RaytracingScene& raytracingScene,
                                const int sceneNr)
{
	if (sceneNr <= 0 || sceneNr > SCENE_COUNT)
	{
		std::printf("Scene %d not implemented\n", sceneNr);
		return;
	}

	raytracingScene.getWorldObjectSpheres().clear();
	raytracingScene.getWorldObjectTetrahedrons().clear();
	raytracingScene.getWorldObjectBezierTriangles().clear();
	raytracingScene.getWorldObjectRectangularBezierSurfaces2x2().clear();
	raytracingScene.getSlicingPlanes().clear();

	// first sphere represents light
	raytracingScene.addObjectSphere(
	    renderer.getRaytracingDataConstants().globalLightPosition, 0.1f, ColorIdx::t_yellow);

	// we always wanna create one slicing plane
	raytracingScene.addSlicingPlane(SlicingPlane{
	    glm::vec3(0.7, 0, 0),
	    glm::vec3(-1, 0, 0),

	});

	// TODO: support multiple slicing planes
	// raytracingScene.addSlicingPlane(SlicingPlane{
	//     glm::vec3(0, 5, 0),
	//     glm::vec3(0, 1, 0),
	// });

	if (sceneNr == 1)
	{
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f),
		    glm::vec3(2.0f, 0.0f, 0.0f),
		    glm::vec3(2.0f, 2.0f, 2.0f),
		    glm::vec3(0.0f, 0.0f, 2.0f),

		    glm::vec3(1.0f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 1.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 1.0f),

		    glm::vec3(1.0f, 1.0f, 0.0f),
		    glm::vec3(1.0f, 0.0f, 1.0f),
		    glm::vec3(0.0f, 1.0f, 1.0f),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
	}
	else if (sceneNr == 2)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(8.0f, 0, 10.0f);
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 2.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset,

		    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset,

		    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset,
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		tracer::rt::visualizeTetrahedron<Tetrahedron2, 10>(raytracingScene, tetrahedron2);
	}
	else if (sceneNr == 3)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0, 0.0f);
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0, 0, 0) * scalar + offset,
		    glm::vec3(4, 0, 2) * scalar + offset,
		    glm::vec3(7, 0, 9) * scalar + offset,
		    glm::vec3(0, 0, 4) * scalar + offset,

		    glm::vec3(2, 0, 1) * scalar + offset,
		    glm::vec3(6, 3, 2) * scalar + offset,
		    glm::vec3(0, 0, 2) * scalar + offset,

		    glm::vec3(7, 2, 1) * scalar + offset,
		    glm::vec3(4, 0, 3) * scalar + offset,
		    glm::vec3(5, 2, 1) * scalar + offset,
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
	}
	else if (sceneNr == 4)
	{
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(2.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 2.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 2.0f),

			    glm::vec3(1.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 1.0f),

			    glm::vec3(1.0f, 1.0f, 0.0f),
			    glm::vec3(1.0f, 0.0f, 1.0f),
			    glm::vec3(0.0f, 1.0f, 1.0f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		}
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, -2.0f),
			    glm::vec3(0.0f, 2.0f, 0.0f),
			    glm::vec3(2.0f, 0.0f, 0.0f),

			    glm::vec3(0.0f, 0.0f, -1.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(1.0f, 0.0f, 0.0f),

			    glm::vec3(0.0f, 1.0f, -1.0f),
			    glm::vec3(1.0f, 0.0f, -1.0f),
			    glm::vec3(1.0f, 1.0f, 0.0f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		}
	}
	else if (sceneNr == 5)
	{
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(2.0f, 0.0f, 0.0f),
			    glm::vec3(2.0f, 2.0f, 2.0f),
			    glm::vec3(0.0f, 0.0f, 2.0f),

			    glm::vec3(1.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 1.0f),

			    glm::vec3(1.0f, 1.0f, 0.0f),
			    glm::vec3(1.0f, 0.0f, 1.0f),
			    glm::vec3(0.0f, 1.0f, 1.0f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		}
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, -2.0f),
			    glm::vec3(2.0f, 2.0f, 2.0f),
			    glm::vec3(2.0f, 0.0f, 0.0f),

			    glm::vec3(0.0f, 0.0f, -1.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(1.0f, 0.0f, 0.0f),

			    glm::vec3(0.0f, 1.0f, -1.0f),
			    glm::vec3(1.0f, 0.0f, -1.0f),
			    glm::vec3(1.0f, 1.0f, 0.0f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2,
			                                                         {true, true, true, true});
		}
	}
	else if (sceneNr == 6)
	{
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron3(std::to_array({
		    glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 2), glm::vec3(0, 0, 3),
		    glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 1, 2), glm::vec3(0, 2, 0),
		    glm::vec3(0, 2, 1), glm::vec3(0, 3, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 1),
		    glm::vec3(1, 0, 2), glm::vec3(1, 1, 0), glm::vec3(1, 1, 1), glm::vec3(1, 2, 0),
		    glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 1, 0), glm::vec3(3, 0, 0),
		}));

		for (auto& point : tetrahedron2.controlPoints)
		{
			if (point.x == 1 && point.y == 1 && point.z == 1)
			{
				raytracingScene.addObjectSphere(point, 0.08f, ColorIdx::t_red);
			}
			else
			{
				raytracingScene.addObjectSphere(point, 0.04f, ColorIdx::t_black);
			}
		}
	}
	else if (sceneNr == 7)
	{

		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0, 0.0f);
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(-0, 0),
		    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(-0, 2),
		    glm::vec3(2.0f, 2.0f, 2.0f) * scalar + offset + getRandomOffset(-0, 2),
		    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset + getRandomOffset(-0, 2),

		    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(-0, 0),
		    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(-0, 0),
		    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(-0, 0),

		    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(-0, 0),
		    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(-0, 0),
		    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset + getRandomOffset(-0, 0),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		// Visualize control points
		for (auto& point : tetrahedron2.controlPoints)
		{
			raytracingScene.addObjectSphere(point, 0.04f, ColorIdx::t_pink);
		}
	}
	else if (sceneNr == 8)
	{
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, 0.0f),
			    glm::vec3(2.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 2.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 2.0f),

			    glm::vec3(1.0f, 0.0f, 0.0f),
			    glm::vec3(0.0f, 1.0f, 0.0f),
			    glm::vec3(0.0f, 0.0f, 1.0f),

			    glm::vec3(1.0f, 1.0f, 0.0f),
			    glm::vec3(1.0f, 0.0f, 1.0f),
			    glm::vec3(0.0f, 1.0f, 1.0f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		}
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, -0.01f),
			    glm::vec3(0.0f, 0.0f, -2.0f),
			    glm::vec3(0.0f, 2.0f, -0.01f),
			    glm::vec3(2.0f, 0.0f, -0.01f),

			    glm::vec3(0.0f, 0.0f, -1.0f),
			    glm::vec3(0.0f, 1.0f, -0.01f),
			    glm::vec3(1.0f, 0.0f, -0.01f),

			    glm::vec3(0.00, 1.0f, -1.0f),
			    glm::vec3(1.0f, 0.0f, -1.0f),
			    glm::vec3(1.0f, 1.0f, -0.01f),
			}));
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		}
	}
	else
	{
		std::printf("Scene %d not implemented\n", sceneNr);
	}
}

} // namespace rt
} // namespace tracer
