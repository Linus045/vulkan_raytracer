#pragma once

#include <algorithm>
#include <array>
#include <stdexcept>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

#include "common_types.h"
#include "bezier_math.hpp"

namespace ltracer
{

struct SubdividedTetrahedron2
{
	BezierTriangle2 bottomLeft;
	BezierTriangle2 bottomRight;
	BezierTriangle2 top;
	BezierTriangle2 center;
};

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
	[[maybe_unused]] const uint32_t n;
	[[maybe_unused]] const uint32_t m;
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
	for (size_t i = 0; i < points.size(); i++)
	{
		tetrahedron.controlPoints[i] = points[i];
	}
	return tetrahedron;
}

inline BezierTriangle2 extractBezierTriangleFromTetrahedron(const Tetrahedron2& tetrahedron2,
                                                            const int side)
{
	BezierTriangle2 bezierTriangle{};
	if (side == 1)
	{
		bezierTriangle.controlPoints[0] = tetrahedron2.controlPoints[0];
		bezierTriangle.controlPoints[1] = tetrahedron2.controlPoints[3];
		bezierTriangle.controlPoints[2] = tetrahedron2.controlPoints[2];
		bezierTriangle.controlPoints[3] = tetrahedron2.controlPoints[6];
		bezierTriangle.controlPoints[4] = tetrahedron2.controlPoints[9];
		bezierTriangle.controlPoints[5] = tetrahedron2.controlPoints[5];
	}
	else if (side == 2)
	{
		bezierTriangle.controlPoints[0] = tetrahedron2.controlPoints[1];
		bezierTriangle.controlPoints[1] = tetrahedron2.controlPoints[3];
		bezierTriangle.controlPoints[2] = tetrahedron2.controlPoints[0];
		bezierTriangle.controlPoints[3] = tetrahedron2.controlPoints[8];
		bezierTriangle.controlPoints[4] = tetrahedron2.controlPoints[6];
		bezierTriangle.controlPoints[5] = tetrahedron2.controlPoints[4];
	}
	else if (side == 3)
	{
		bezierTriangle.controlPoints[0] = tetrahedron2.controlPoints[1];
		bezierTriangle.controlPoints[1] = tetrahedron2.controlPoints[0];
		bezierTriangle.controlPoints[2] = tetrahedron2.controlPoints[2];
		bezierTriangle.controlPoints[3] = tetrahedron2.controlPoints[4];
		bezierTriangle.controlPoints[4] = tetrahedron2.controlPoints[5];
		bezierTriangle.controlPoints[5] = tetrahedron2.controlPoints[7];
	}
	else if (side == 4)
	{
		bezierTriangle.controlPoints[0] = tetrahedron2.controlPoints[3];
		bezierTriangle.controlPoints[1] = tetrahedron2.controlPoints[1];
		bezierTriangle.controlPoints[2] = tetrahedron2.controlPoints[2];
		bezierTriangle.controlPoints[3] = tetrahedron2.controlPoints[8];
		bezierTriangle.controlPoints[4] = tetrahedron2.controlPoints[7];
		bezierTriangle.controlPoints[5] = tetrahedron2.controlPoints[9];
	}
	else
	{
		throw new std::runtime_error("extractBezierTriangleFromTetrahedron - side invalid");
	}

	return bezierTriangle;
}

inline BezierTriangle2 getBottomLeftSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
{
	// new triangle
	//          b020
	//         /    \
	//        /      \
	//     b011      b110
	//     /    BL     \
	//    /             \
	// b002--- b101 ---b200
	constexpr auto idxb002 = getControlPointIndicesBezierTriangle2(0, 0, 2);
	constexpr auto idxb011 = getControlPointIndicesBezierTriangle2(0, 1, 1);
	constexpr auto idxb101 = getControlPointIndicesBezierTriangle2(1, 0, 1);

	// calculate the new 6 control points (np = new point)
	const vec3 np002 = bezierTriangle2.controlPoints[idxb002];
	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);

	const vec3 np011
	    = 0.5f * (bezierTriangle2.controlPoints[idxb002] + bezierTriangle2.controlPoints[idxb011]);
	const vec3 np110 = 0.5f * (np020 + np200);
	const vec3 np101
	    = 0.5f * (bezierTriangle2.controlPoints[idxb002] + bezierTriangle2.controlPoints[idxb101]);

	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
}

inline BezierTriangle2 getBottomRightSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
{
	// new triangle
	//          b020
	//         /    \
	//        /      \
	//     b011      b110
	//     /    BR     \
	//    /             \
	// b002--- b101 ---b200
	constexpr auto idxb200 = getControlPointIndicesBezierTriangle2(2, 0, 0);
	constexpr auto idxb101 = getControlPointIndicesBezierTriangle2(1, 0, 1);
	constexpr auto idxb110 = getControlPointIndicesBezierTriangle2(1, 1, 0);

	// calculate the new 6 control points (np = new point)
	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);
	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);
	const vec3 np200 = bezierTriangle2.controlPoints[idxb200];

	const vec3 np011 = 0.5f * (np002 + np020);
	const vec3 np110
	    = 0.5f * (bezierTriangle2.controlPoints[idxb200] + bezierTriangle2.controlPoints[idxb110]);
	const vec3 np101
	    = 0.5f * (bezierTriangle2.controlPoints[idxb200] + bezierTriangle2.controlPoints[idxb101]);

	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
}

inline BezierTriangle2 getTopSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
{
	// new triangle
	//          b020
	//         /    \
	//        /      \
	//     b011      b110
	//     /     T     \
	//    /             \
	// b002--- b101 ---b200
	constexpr auto idxb020 = getControlPointIndicesBezierTriangle2(0, 2, 0);
	constexpr auto idxb011 = getControlPointIndicesBezierTriangle2(0, 1, 1);
	constexpr auto idxb110 = getControlPointIndicesBezierTriangle2(1, 1, 0);

	// calculate the new 6 control points (np = new point)
	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
	const vec3 np020 = bezierTriangle2.controlPoints[idxb020];
	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);

	const vec3 np011
	    = 0.5f * (bezierTriangle2.controlPoints[idxb020] + bezierTriangle2.controlPoints[idxb011]);
	const vec3 np110
	    = 0.5f * (bezierTriangle2.controlPoints[idxb020] + bezierTriangle2.controlPoints[idxb110]);
	const vec3 np101 = 0.5f * (np002 + np200);

	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
}

inline BezierTriangle2 getCenterSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
{
	// new triangle
	//          b020
	//         /    \
	//        /      \
	//     b011      b110
	//     /     C     \
	//    /             \
	// b002--- b101 ---b200

	// calculate the new 6 control points (np = new point)
	// Note: the triangle is flipped, for some reason it causes problem with the normal calculation

	// WARNING: getCenterSubdivisionTriangle2 - calculates wrong control points
	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);
	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);

	const vec3 np011 = 0.5f * (np002 + np020);
	const vec3 np110 = 0.5f * (np020 + np200);
	const vec3 np101 = 0.5f * (np002 + np200);

	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
}

inline SubdividedTetrahedron2 subdivideBezierTriangle2(const BezierTriangle2& bezierTriangle2)
{
	// T = top, BL = bottom left, BR = bottom right, C = center
	//          /\
	//         /  \
	//        / T  \
	//       /------\
	//     	/\  C   /\
	//     /  \    /  \
	//    / BL \  / BR \
	//    --------------

	return SubdividedTetrahedron2{
	    .bottomLeft = getBottomLeftSubdivisionTriangle2(bezierTriangle2),
	    .bottomRight = getBottomRightSubdivisionTriangle2(bezierTriangle2),
	    .top = getTopSubdivisionTriangle2(bezierTriangle2),
	    .center = getCenterSubdivisionTriangle2(bezierTriangle2),
	};
}

} // namespace ltracer
