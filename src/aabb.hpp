#pragma once

#include "glm/common.hpp"
#include "src/tetrahedron.hpp"
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
