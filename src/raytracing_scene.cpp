
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

void RaytracingScene::clearScene()
{
	getWorldObjectSpheres().clear();
	// raytracingScene.getWorldObjectTetrahedrons().clear();
	getWorldObjectBezierTriangles<BezierTriangle2>().clear();
	getWorldObjectBezierTriangles<BezierTriangle3>().clear();
	getWorldObjectBezierTriangles<BezierTriangle4>().clear();
	getWorldObjectRectangularBezierSurfaces2x2().clear();

	/// we add the slicing plane once in the RaytracingScene constructor instead of adding it
	/// per scene
	// raytracingScene.getSlicingPlanes().clear();
}

/// Loads the scene with the given index. A manual rebuild of the acceleration
/// structure is still required
void RaytracingScene::loadScene([[maybe_unused]] const Renderer& renderer,
                                RaytracingScene& raytracingScene,
                                const SceneConfig sceneConfig,
                                const int sceneNr)
{
	if (sceneNr <= 0 || sceneNr > SCENE_COUNT)
	{
		std::printf("Scene %d not implemented\n", sceneNr);
		return;
	}

	raytracingScene.clearScene();
	raytracingScene.currentSceneNr = sceneNr;

	// first sphere represents light
	raytracingScene.addObjectSphere(
	    renderer.getRaytracingDataConstants().globalLightPosition, 0.2f, ColorIdx::t_yellow);

	// TODO: support multiple slicing planes
	// raytracingScene.addSlicingPlane(SlicingPlane{
	//     glm::vec3(0, 5, 0),
	//     glm::vec3(0, 1, 0),
	// });

	if (sceneNr == 1)
	{
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f),
		    glm::vec3(0.0f, 0.0f, 1.0f),
		    glm::vec3(0.0f, 0.0f, 2.0f),
		    glm::vec3(0.0f, 1.0f, 0.0f),
		    glm::vec3(0.0f, 1.0f, 1.0f),
		    glm::vec3(2.0f, 2.0f, 2.0f),
		    glm::vec3(1.0f, 0.0f, 0.0f),
		    glm::vec3(1.0f, 0.0f, 1.0f),
		    glm::vec3(1.0f, 1.0f, 0.0f),
		    glm::vec3(2.0f, 0.0f, 0.0f),
		}));
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 2)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0, 0.0f);
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset,
		    glm::vec3(0.0f, 2.0f, 0.0f) * scalar + offset,
		    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset,
		    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(3.0f, 1.5f, 0.0f) * scalar + offset,
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 3)
	{
		auto point02 = glm::vec3(0.0f, 0.0f, 0.0f);
		auto point34 = glm::vec3(0.0f, 1.0f, 0.0f);
		auto point55 = glm::vec3(0.0f, 2.0f, 0.0f);
		auto point67 = glm::vec3(1.0f, 0.0f, 0.0f);
		auto point88 = glm::vec3(1.0f, 1.0f, 0.0f);
		auto point99 = glm::vec3(2.0f, 0.0f, 0.0f);

		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    point02,
			    glm::vec3(0.0f, 0.0f, 1.0f),
			    glm::vec3(0.0f, 0.0f, 2.0f),
			    point34,
			    glm::vec3(0.0f, 1.0f, 1.0f),
			    point55,
			    point67,
			    glm::vec3(1.0f, 0.0f, 1.0f),
			    point88,
			    point99,
			}));

			// mark left side triangle as inside
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(
			    tetrahedron2, {true, true, true, true}, {false, false, true, false});

			if (sceneConfig.visualizeControlPoints)
			{
				visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
			}
			visualizeTetrahedronSides(raytracingScene,
			                          tetrahedron2,
			                          sceneConfig.visualizeSampledSurface,
			                          sceneConfig.visualizeSampledVolume);
		}
		{
			[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
			    glm::vec3(0.0f, 0.0f, -2.0f),
			    glm::vec3(0.0f, 0.0f, -1.0f),
			    point02,
			    glm::vec3(0.0f, 1.0f, -1.0f),
			    point34,
			    point55,
			    glm::vec3(1.0f, 0.0f, -1.0f),
			    point67,
			    point88,
			    point99,
			}));

			// mark right side triangle as inside
			raytracingScene.addSidesFromTetrahedronAsBezierTriangles(
			    tetrahedron2, {true, true, true, true}, {false, false, false, true});
			if (sceneConfig.visualizeControlPoints)
			{
				visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
			}
			visualizeTetrahedronSides(raytracingScene,
			                          tetrahedron2,
			                          sceneConfig.visualizeSampledSurface,
			                          sceneConfig.visualizeSampledVolume);
		}
	}
	else if (sceneNr == 4)
	{
		[[maybe_unused]] auto tetrahedron3 = tracer::createTetrahedron3(std::to_array({
		    glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 2), glm::vec3(0, 0, 3),
		    glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 1, 2), glm::vec3(0, 2, 0),
		    glm::vec3(0, 2, 1), glm::vec3(0, 3, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 1),
		    glm::vec3(1, 0, 2), glm::vec3(1, 1, 0), glm::vec3(1, 1, 1), glm::vec3(1, 2, 0),
		    glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 1, 0), glm::vec3(3, 0, 0),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron3);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron3);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron3,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 5)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0, 0.0f);
		[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset + getRandomOffset(0, 2),
		    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(0.0f, 2.0f, 0.0f) * scalar + offset + getRandomOffset(0, 2),
		    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
		    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 2),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 6)
	{
		for (float x = 0; x < 10; x++)
		{
			for (float z = 0; z < 10; z++)
			{
				float scalar = 1.0f;
				glm::vec3 offset = glm::vec3(3.0 * x, 0, 3.0f * z);
				[[maybe_unused]] auto tetrahedron2 = tracer::createTetrahedron2(std::to_array({
				    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset + getRandomOffset(0, 2),
				    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(0.0f, 2.0f, 0.0f) * scalar + offset + getRandomOffset(0, 2),
				    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset + getRandomOffset(0, 0),
				    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset + getRandomOffset(0, 2),
				}));

				raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
				// Visualize control points
				// for (auto& point : tetrahedron2.controlPoints)
				// {
				// 	raytracingScene.addObjectSphere(point, 0.04f, ColorIdx::t_pink);
				// }
				if (sceneConfig.visualizeControlPoints)
				{
					visualizeTetrahedronControlPoints(raytracingScene, tetrahedron2);
				}
				visualizeTetrahedronSides(raytracingScene,
				                          tetrahedron2,
				                          sceneConfig.visualizeSampledSurface,
				                          sceneConfig.visualizeSampledVolume);
			}
		}
	}
	else if (sceneNr == 7)
	{
		[[maybe_unused]] auto tetrahedron4 = tracer::createTetrahedron4(std::to_array({
		    glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 2), glm::vec3(0, 0, 3),
		    glm::vec3(0, 0, 4), glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 1, 2),
		    glm::vec3(0, 1, 3), glm::vec3(0, 2, 0), glm::vec3(0, 2, 1), glm::vec3(0, 2, 2),
		    glm::vec3(0, 3, 0), glm::vec3(0, 3, 1), glm::vec3(0, 4, 0), glm::vec3(1, 0, 0),
		    glm::vec3(1, 0, 1), glm::vec3(1, 0, 2), glm::vec3(1, 0, 3), glm::vec3(1, 1, 0),
		    glm::vec3(1, 1, 1), glm::vec3(1, 1, 2), glm::vec3(1, 2, 0), glm::vec3(1, 2, 1),
		    glm::vec3(1, 3, 0), glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 0, 2),
		    glm::vec3(2, 1, 0), glm::vec3(2, 1, 1), glm::vec3(2, 2, 0), glm::vec3(3, 0, 0),
		    glm::vec3(3, 0, 1), glm::vec3(3, 1, 0), glm::vec3(4, 0, 0),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron4);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron4);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron4,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 8)
	{
		[[maybe_unused]] auto tetrahedron4 = tracer::createTetrahedron4(std::to_array({
		    glm::vec3(0, 0, 0),   glm::vec3(0, 0, 1), glm::vec3(0, 0, 2), glm::vec3(0, 0, 3),
		    glm::vec3(0, 0, 4),   glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 1, 2),
		    glm::vec3(0, 1, 3),   glm::vec3(0, 2, 0), glm::vec3(0, 2, 1), glm::vec3(0, 2, 2),
		    glm::vec3(0, 3, 0),   glm::vec3(0, 3, 1), glm::vec3(0, 4, 0), glm::vec3(1, 0, 0),
		    glm::vec3(1, 0, 1),   glm::vec3(1, 0, 2), glm::vec3(1, 0, 3), glm::vec3(1, 1, 0),
		    glm::vec3(-60, 1, 1), glm::vec3(0, 1, 2), glm::vec3(1, 2, 0), glm::vec3(1, 2, 1),
		    glm::vec3(1, 3, 0),   glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 0, 2),
		    glm::vec3(2, 1, 0),   glm::vec3(2, 1, 1), glm::vec3(2, 2, 0), glm::vec3(3, 0, 0),
		    glm::vec3(3, 0, 1),   glm::vec3(3, 1, 0), glm::vec3(4, 0, 0),
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron4);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(raytracingScene, tetrahedron4);
		}
		visualizeTetrahedronSides(raytracingScene,
		                          tetrahedron4,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume,
		                          0.05f);
	}
	else
	{
		std::printf("Scene %d not implemented\n", sceneNr);
	}
}

template <>
std::vector<RaytracingWorldObject<BezierTriangle2>>& RaytracingScene::getTriangleList()
{
	return bezierTriangles2;
}

template <>
std::vector<RaytracingWorldObject<BezierTriangle3>>& RaytracingScene::getTriangleList()
{
	return bezierTriangles3;
}

template <>
std::vector<RaytracingWorldObject<BezierTriangle4>>& RaytracingScene::getTriangleList()
{
	return bezierTriangles4;
}

} // namespace rt
} // namespace tracer
