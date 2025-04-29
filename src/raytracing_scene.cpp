
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
	for (auto& sceneObject : sceneObjects)
	{
		getWorldObjectSpheres(sceneObject).clear();
		// raytracingScene.getWorldObjectTetrahedrons().clear();
		getWorldObjectBezierTriangles<BezierTriangle2>(sceneObject).clear();
		getWorldObjectBezierTriangles<BezierTriangle3>(sceneObject).clear();
		getWorldObjectBezierTriangles<BezierTriangle4>(sceneObject).clear();
		getWorldObjectRectangularBezierSurfaces2x2(sceneObject).clear();
	}
	sceneObjects.clear();
	objectNameToSceneObjectMap.clear();

	spheresList.clear();
	bezierTriangles2List.clear();
	bezierTriangles3List.clear();
	bezierTriangles4List.clear();
	rectangularSurfaces2x2List.clear();

	spheres.clear();
	bezierTriangles2.clear();
	bezierTriangles3.clear();
	bezierTriangles4.clear();
	rectangularBezierSurfaces2x2.clear();

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
	// TODO: add into its own BLAS Instance
	auto& sceneObjectLight = raytracingScene.createNamedSceneObject(
	    "light", renderer.getRaytracingDataConstants().globalLightPosition);
	raytracingScene.addObjectSphere(sceneObjectLight,
	                                renderer.getRaytracingDataConstants().globalLightPosition,
	                                true,
	                                0.2f,
	                                ColorIdx::t_yellow);

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
		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron2);

		auto& sceneObjectControlPoints = raytracingScene.createNamedSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
		                          tetrahedron2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);

		// const auto& p = glm::vec3(4, 0, 0);
		// auto& s = raytracingScene.createSceneObject(p);
		// raytracingScene.addObjectSphere(s, p, 1.0f, ColorIdx::t_orange);
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

		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron2);

		auto& sceneObjectControlPoints = raytracingScene.createNamedSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
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

		[[maybe_unused]] auto tetrahedron2_1 = tracer::createTetrahedron2(std::to_array({
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

		[[maybe_unused]] auto tetrahedron2_2 = tracer::createTetrahedron2(std::to_array({
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

		// create a scene object to which is build up of multiple teteahedron (they will be stored
		// in
		// the same BLAS)
		auto& sceneObject = raytracingScene.createNamedSceneObject("model");

		// first add all the triangles (so the data is in one chunk inside the SceneObject)
		// mark left side triangle as inside
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(
		    sceneObject, tetrahedron2_1, {true, true, true, true}, {false, false, true, false});
		// mark right side triangle as inside
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(
		    sceneObject, tetrahedron2_2, {true, true, true, true}, {false, false, false, true});

		auto& sceneObjectControlPoints = raytracingScene.createNamedSceneObject();
		// second add all the spheres
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron2_1);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
		                          tetrahedron2_1,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);

		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron2_2);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
		                          tetrahedron2_2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
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

		// create a scene object to which is build up of multiple teteahedron (they will be stored
		// in
		// the same BLAS)
		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron3);

		auto& sceneObjectControlPoints = raytracingScene.createNamedSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron3);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
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

		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron2);

		auto& sceneObjectControlPoints = raytracingScene.createNamedSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron2);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
		                          tetrahedron2,
		                          sceneConfig.visualizeSampledSurface,
		                          sceneConfig.visualizeSampledVolume);
	}
	else if (sceneNr == 6)
	{
		// create a scene object to which is build up of multiple teteahedron (they will be stored
		// in the same BLAS)
		auto& sceneObject = raytracingScene.createNamedSceneObject("model");

		auto tetrahedrons = std::vector<Tetrahedron2>();
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
				tetrahedrons.push_back(tetrahedron2);

				raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron2);
			}
		}

		auto& sceneObjectControlPoints = raytracingScene.createSceneObject();
		// after creating all the tetrahedrons (in one chunk), we can add the spheres
		for (const auto& tetrahedron2 : tetrahedrons)
		{
			if (sceneConfig.visualizeControlPoints)
			{
				visualizeTetrahedronControlPoints(
				    sceneObjectControlPoints, raytracingScene, tetrahedron2);
			}
			visualizeTetrahedronSides(sceneObjectControlPoints,
			                          raytracingScene,
			                          tetrahedron2,
			                          sceneConfig.visualizeSampledSurface,
			                          sceneConfig.visualizeSampledVolume);
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

		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron4);

		auto& sceneObjectControlPoints = raytracingScene.createSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron4);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
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
		    glm::vec3(-60, 1, 1), glm::vec3(1, 1, 2), glm::vec3(1, 2, 0), glm::vec3(1, 2, 1),
		    glm::vec3(1, 3, 0),   glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 0, 2),
		    glm::vec3(2, 1, 0),   glm::vec3(2, 1, 1), glm::vec3(2, 2, 0), glm::vec3(3, 0, 0),
		    glm::vec3(3, 0, 1),   glm::vec3(3, 1, 0), glm::vec3(4, 0, 0),
		}));

		auto& sceneObject = raytracingScene.createNamedSceneObject("model");
		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(sceneObject, tetrahedron4);

		auto& sceneObjectControlPoints = raytracingScene.createSceneObject();
		if (sceneConfig.visualizeControlPoints)
		{
			visualizeTetrahedronControlPoints(
			    sceneObjectControlPoints, raytracingScene, tetrahedron4);
		}
		visualizeTetrahedronSides(sceneObjectControlPoints,
		                          raytracingScene,
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
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle2>>>&
RaytracingScene::getTriangleListForSceneObject(SceneObject& sceneObject)
{
	return sceneObject.bezierTriangles2;
}

template <>
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle3>>>&
RaytracingScene::getTriangleListForSceneObject(SceneObject& sceneObject)
{
	return sceneObject.bezierTriangles3;
}

template <>
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle4>>>&
RaytracingScene::getTriangleListForSceneObject(SceneObject& sceneObject)
{
	return sceneObject.bezierTriangles4;
}

template <>
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle2>>>&
RaytracingScene::getTriangleList()
{
	return bezierTriangles2;
}

template <>
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle3>>>&
RaytracingScene::getTriangleList()
{
	return bezierTriangles3;
}

template <>
std::vector<std::shared_ptr<RaytracingWorldObject<BezierTriangle4>>>&
RaytracingScene::getTriangleList()
{
	return bezierTriangles4;
}

} // namespace rt
} // namespace tracer
