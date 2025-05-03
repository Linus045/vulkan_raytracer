#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include "glm/ext/vector_float3.hpp"

// forward declaration
struct Tetrahedron1;
struct Tetrahedron2;
struct Triangle;
struct Sphere;
struct RectangularBezierSurface2x2;

namespace tracer
{

// forward declaration
class RectangularBezierSurface;

// Axis Aligned Bounding Box
class AABB
{
  public:
	// Bottom left corder
	glm::vec3 min;

	// Top right corner
	glm::vec3 max;

	VkAabbPositionsKHR getAabbPositions() const;

	static AABB
	fromRectangularBezierSurface2x2(const RectangularBezierSurface2x2& rectangularBezierSurface2x2);

	static AABB
	fromRectangularBezierSurface(const RectangularBezierSurface& rectangularBezierSurface);

	static AABB fromTetrahedron(const Tetrahedron1& tetrahedron);

	static AABB fromTetrahedron2(const Tetrahedron2& tetrahedron);

	static AABB fromTriangle(const Triangle& triangle);

	template <typename T>
	static AABB fromBezierTriangle(const T& bezierTriangle)
	{
		glm::vec3 min = bezierTriangle.controlPoints[0];
		glm::vec3 max = bezierTriangle.controlPoints[0];

		for (const glm::vec3& point : bezierTriangle.controlPoints)
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

	// localSpace is used to determine if the AABB should be created in local space or
	// in global space.
	// We need the global space if the transform matrix of the BLAS that contains this sphere is the
	// identity matrix
	// Note that moving the sphere without recreating the BLAS is only possible when
	// using localSpace since then we only modity the transform matrix and not the actually sphere
	// position -- localSpace is used for the light sphere to move it without rebuilding the
	// acceleration struture
	static AABB fromSphere(const Sphere& sphere, const bool localSpace);
};

} // namespace tracer
