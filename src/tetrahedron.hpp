#pragma once

#include <algorithm>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

#include "src/common_types.h"

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
		assert(degreeN > 0 && "RectangularBezierSurface - Degree N can't be 0");
		assert(degreeM > 0 && "RectangularBezierSurface - Degree M can't be 0");
		assert(controlPoints.size() == (1 + degreeN) * (1 + degreeM)
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

} // namespace ltracer
