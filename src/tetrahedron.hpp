#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

namespace ltracer
{

struct Tetrahedron
{
	const glm::vec3 a;
	const glm::vec3 b;
	const glm::vec3 c;
};

struct Sphere
{
	const glm::vec3 center;
	const float radius;
};

class Triangle
{
  public:
	Triangle(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c) : a(a), b(b), c(c)
	{
	}

	const glm::vec3 a;
	const glm::vec3 b;
	const glm::vec3 c;
};

} // namespace ltracer
