#pragma once

#include <algorithm>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

#include "common_types.h"

namespace ltracer
{

class RectangularBezierSurface
{
  public:
	RectangularBezierSurface(const uint32_t degreeN,
	                         const uint32_t degreeM,
	                         const std::vector<glm::vec3> controlPoints)
	    : n(degreeN), m(degreeM), controlPoints(controlPoints)
	{
		assert(n > 0 && "RectangularBezierSurface - Degree N can't be 0");
		assert(m > 0 && "RectangularBezierSurface - Degree M can't be 0");
		assert(controlPoints.size() == (1 + n) * (1 + m)
		       && "RectangularBezierSurface - controlpoint count does not match degree");
	}

	const std::vector<glm::vec3>& getControlPoints() const
	{
		return controlPoints;
	}

	bool tryConvertToRectangularSurfaces2x2(
	    std::vector<RectangularBezierSurface2x2>& rectangularBezierSurfaces2x2)
	{
		// TODO: figure out how to split a bigger surface into smaller ones
		// for now just we just work with 2x2 surfaces so we can just copy the values
		auto surface = RectangularBezierSurface2x2{};
		assert(std::size(surface.controlPoints) == 16 && "Amount of control points is not 16???");
		if (std::size(surface.controlPoints) == controlPoints.size())
		{
			std::copy_n(
			    controlPoints.begin(), std::size(surface.controlPoints), surface.controlPoints);
			rectangularBezierSurfaces2x2.push_back(std::move(surface));
			return true;
		}
		return false;
	}

  private:
	const uint32_t n;
	const uint32_t m;
	const std::vector<glm::vec3> controlPoints;
};

inline Tetrahedron1 createTetrahedron1(const std::array<glm::vec3, 4>& points)
{
	auto tetrahedron = Tetrahedron1();
	tetrahedron.controlPoints[0] = points[0];
	tetrahedron.controlPoints[1] = points[1];
	tetrahedron.controlPoints[2] = points[2];
	tetrahedron.controlPoints[3] = points[3];
	return tetrahedron;
}

inline Tetrahedron2 createTetrahedron2(const std::array<glm::vec3, 10>& points)
{
	auto tetrahedron = Tetrahedron2();
	for (size_t i = 0; i < 10; i++)
	{
		tetrahedron.controlPoints[i] = points[i];
	}
	return tetrahedron;
}

} // namespace ltracer
