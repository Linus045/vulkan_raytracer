#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/common.hpp"

#include "tetrahedron.hpp"
#include "common_types.h"

namespace ltracer
{

// Axis Aligned Bounding Box
class AABB
{
  public:
	// Bottom left corder
	const glm::vec3 min;

	// Top right corner
	const glm::vec3 max;

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

	static AABB
	fromRectangularBezierSurface(const RectangularBezierSurface2x2& rectangularBezierSurface2x2)
	{
		glm::vec3 min = rectangularBezierSurface2x2.controlPoints[0];
		glm::vec3 max = rectangularBezierSurface2x2.controlPoints[0];

		for (const glm::vec3& point : rectangularBezierSurface2x2.controlPoints)
		{
			min.x = glm::min(min.x, point.x);
			min.y = glm::min(min.y, point.y);
			min.z = glm::min(min.z, point.z);

			max.x = glm::max(max.x, point.x);
			max.y = glm::max(max.y, point.y);
			max.z = glm::max(max.z, point.z);
		}

		return AABB{
		    .min = min,
		    .max = max,
		};
	}
	static AABB
	fromRectangularBezierSurface(const RectangularBezierSurface& rectangularBezierSurface)
	{
		glm::vec3 min, max;
		for (const glm::vec3& point : rectangularBezierSurface.getControlPoints())
		{
			min.x = glm::min(min.x, point.x);
			min.y = glm::min(min.y, point.y);
			min.z = glm::min(min.z, point.z);

			max.x = glm::min(max.x, point.x);
			max.y = glm::min(max.y, point.y);
			max.z = glm::min(max.z, point.z);
		}

		return AABB{
		    .min = min,
		    .max = max,
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
			},
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
			},
		};
	}

	static AABB fromSphere(const Sphere& sphere)
	{
		return AABB
		{
			.min = {
				sphere.center.x - sphere.radius,
				sphere.center.y - sphere.radius,
				sphere.center.z - sphere.radius,
			},
			.max = {
				sphere.center.x + sphere.radius,
				sphere.center.y + sphere.radius,
				sphere.center.z + sphere.radius,
			},
		};
	}
};

} // namespace ltracer
