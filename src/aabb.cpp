#include "aabb.hpp"

#include "tetrahedron.hpp"
#include "rectangularBezierSurface.hpp"
#include "common_types.h"

namespace tracer
{

VkAabbPositionsKHR AABB::getAabbPositions() const
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

AABB AABB::fromRectangularBezierSurface2x2(
    const RectangularBezierSurface2x2& rectangularBezierSurface2x2)
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

AABB AABB::fromRectangularBezierSurface(const RectangularBezierSurface& rectangularBezierSurface)
{
	glm::vec3 min = rectangularBezierSurface.getControlPoints()[0];
	glm::vec3 max = rectangularBezierSurface.getControlPoints()[0];

	for (const glm::vec3& point : rectangularBezierSurface.getControlPoints())
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

AABB AABB::fromTetrahedron(const Tetrahedron1& tetrahedron)
{
	glm::vec3 min = tetrahedron.controlPoints[0];
	glm::vec3 max = tetrahedron.controlPoints[0];

	for (const glm::vec3& point : tetrahedron.controlPoints)
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

AABB AABB::fromTetrahedron2(const Tetrahedron2& tetrahedron)
{
	glm::vec3 min = tetrahedron.controlPoints[0];
	glm::vec3 max = tetrahedron.controlPoints[0];

	for (const glm::vec3& point : tetrahedron.controlPoints)
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

AABB AABB::fromTriangle(const Triangle& triangle)
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

AABB AABB::fromSphere(const Sphere& sphere, const bool localSpace)
{
	glm::vec3 offset = localSpace ? glm::vec3(0) : sphere.center;

	return AABB
		{
			.min = {
				-sphere.radius + offset.x,
				-sphere.radius + offset.y,
				-sphere.radius + offset.z,
			},
			.max = {
				+sphere.radius + offset.x,
				+sphere.radius + offset.y,
				+sphere.radius + offset.z,
			},
		};
}
} // namespace tracer
