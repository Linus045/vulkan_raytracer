#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/common.hpp"

#include "src/tetrahedron.hpp"

namespace ltracer
{

// Axis Aligned Bounding Box
class AABB
{
  public:
	enum ObjectType
	{
		t_Surface = 1,
		t_Triangle = 2,
		t_Tetrahedron = 3,
	};

	// Bottom left corder
	const glm::vec3 min;

	// Top right corner
	const glm::vec3 max;
	const ObjectType objectType;

	VkAabbPositionsKHR getAabb() const
	{
		return VkAabbPositionsKHR{
		    .minX = min.x,
		    .minY = min.y,
		    .minZ = min.z,
		    .maxX = max.x,
		    .maxY = max.y,
		    .maxZ = max.z,
		};
	}

	static AABB fromTetrahedron(const Tetrahedron& tetrahedron)
	{
		return AABB
		{
			.min = {
				glm::min(tetrahedron.a.x, glm::min(tetrahedron.b.x, tetrahedron.c.x)),
				glm::min(tetrahedron.a.y, glm::min(tetrahedron.b.y, tetrahedron.c.y)),
				glm::min(tetrahedron.a.z, glm::min(tetrahedron.b.z, tetrahedron.c.z)),
			},
			.max = {
				glm::max(tetrahedron.a.x, glm::max(tetrahedron.b.x, tetrahedron.c.x)),
				glm::max(tetrahedron.a.y, glm::max(tetrahedron.b.y, tetrahedron.c.y)),
				glm::max(tetrahedron.a.z, glm::max(tetrahedron.b.z, tetrahedron.c.z)),
			}
		};
	}

	static AABB fromTriangle(const Triangle& triangle)
	{
		return AABB
		{
			.min = {
				glm::min(triangle.a.x, glm::min(triangle.b.x, triangle.c.x)),
				glm::min(triangle.a.y, glm::min(triangle.b.y, triangle.c.y)),
				glm::min(triangle.a.z, glm::min(triangle.b.z, triangle.c.z)),
			},
			.max = {
				glm::max(triangle.a.x, glm::max(triangle.b.x, triangle.c.x)),
				glm::max(triangle.a.y, glm::max(triangle.b.y, triangle.c.y)),
				glm::max(triangle.a.z, glm::max(triangle.b.z, triangle.c.z)),
			}
		};
	}
};

} // namespace ltracer
