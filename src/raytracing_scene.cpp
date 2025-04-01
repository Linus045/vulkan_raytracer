
#include "raytracing_scene.hpp"
#include "renderer.hpp"
#include "visualizations.hpp"

namespace ltracer
{
namespace rt
{

/// Loads the scene with the given index. A manual rebuild of the acceleration
/// structure is still required
void RaytracingScene::loadScene(const Renderer& renderer,
                                RaytracingScene& raytracingScene,
                                const int index)
{
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

	if (index == 1)
	{
		[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
	else if (index == 2)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(8.0f, 0, 10.0f);
		[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
		    glm::vec3(0.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(2.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(2.0f, 2.0f, 2.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 2.0f) * scalar + offset,

		    glm::vec3(1.0f, 0.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(0.0f, 0.0f, 1.0f) * scalar + offset,

		    glm::vec3(1.0f, 1.0f, 0.0f) * scalar + offset,
		    glm::vec3(1.0f, 0.0f, 1.0f) * scalar + offset,
		    glm::vec3(0.0f, 1.0f, 1.0f) * scalar + offset,
		}));

		raytracingScene.addSidesFromTetrahedronAsBezierTriangles(tetrahedron2);
		ltracer::rt::visualizeTetrahedron2(raytracingScene, tetrahedron2);
	}
	else if (index == 3)
	{
		float scalar = 1.0f;
		glm::vec3 offset = glm::vec3(0.0f, 0, 0.0f);
		[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
	else if (index == 4)
	{
		{
			[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
			[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
	else if (index == 5)
	{
		{
			[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
			[[maybe_unused]] auto tetrahedron2 = ltracer::createTetrahedron2(std::to_array({
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
	else
	{
		std::printf("Scene %d not implemented\n", index);
	}
}

} // namespace rt
} // namespace ltracer
