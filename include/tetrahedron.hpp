#pragma once

#include <array>
#include <stdexcept>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

#include "aabb.hpp"
#include "common_types.h"

namespace tracer
{

// NOTE: not used
struct SubdividedTetrahedron2
{
	BezierTriangle2 bottomLeft;
	BezierTriangle2 bottomRight;
	BezierTriangle2 top;
	BezierTriangle2 center;
};

// Tetrahedron -> BezierTriangle class mapping
template <typename T>
struct BezierTriangleFromTetrahedron;

template <>
struct BezierTriangleFromTetrahedron<Tetrahedron1>
{
	using type = BezierTriangle1;
};

template <>
struct BezierTriangleFromTetrahedron<Tetrahedron2>
{
	using type = BezierTriangle2;
};

template <>
struct BezierTriangleFromTetrahedron<Tetrahedron3>
{
	using type = BezierTriangle3;
};

template <>
struct BezierTriangleFromTetrahedron<Tetrahedron4>
{
	using type = BezierTriangle4;
};

/// workaround to get degree of a tetrahedron
/// because Tetrahedron1/2... need to stay simple structs (in common_types.h) to be
/// able to function within a shader as well
template <typename T>
constexpr int degree()
{
	if (!std::is_constant_evaluated())
	{
		throw std::runtime_error("degree() should only be run at compile time!");
	}

	if constexpr (std::is_same_v<T, Tetrahedron1> || std::is_same_v<T, BezierTriangle1>)
	{
		return 1;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron2> || std::is_same_v<T, BezierTriangle2>)
	{
		return 2;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron3> || std::is_same_v<T, BezierTriangle3>)
	{
		return 3;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron4> || std::is_same_v<T, BezierTriangle4>)
	{
		return 4;
	}
	else
	{
		static_assert(false, "Unsupported tetrahedron type");
	}
}

/// workaround to get object type of a tetrahedron
template <typename T>
constexpr ObjectType getObjectType()
{
	if (!std::is_constant_evaluated())
	{
		throw std::runtime_error("objectType() should only be run at compile time!");
	}

	if constexpr (std::is_same_v<T, Sphere>)
	{
		return ObjectType::t_Sphere;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron1>)
	{
		return ObjectType::t_Tetrahedron1;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron2>)
	{
		return ObjectType::t_Tetrahedron2;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron3>)
	{
		return ObjectType::t_Tetrahedron3;
	}
	else if constexpr (std::is_same_v<T, Tetrahedron4>)
	{
		return ObjectType::t_Tetrahedron4;
	}
	else if constexpr (std::is_same_v<T, BezierTriangle1>)
	{
		return ObjectType::t_BezierTriangle1;
	}
	else if constexpr (std::is_same_v<T, BezierTriangle2>)
	{
		return ObjectType::t_BezierTriangle2;
	}
	else if constexpr (std::is_same_v<T, BezierTriangle3>)
	{
		return ObjectType::t_BezierTriangle3;
	}
	else if constexpr (std::is_same_v<T, BezierTriangle4>)
	{
		return ObjectType::t_BezierTriangle4;
	}
	else
	{
		static_assert(false, "Unsupported tetrahedron type");
	}
}

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

inline Tetrahedron3 createTetrahedron3(const std::array<glm::vec3, 20>& points)
{
	auto tetrahedron = Tetrahedron3();
	for (size_t i = 0; i < points.size(); i++)
	{
		tetrahedron.controlPoints[i] = points[i];
	}
	return tetrahedron;
}

inline Tetrahedron4 createTetrahedron4(const std::array<glm::vec3, 35>& points)
{
	auto tetrahedron = Tetrahedron4();
	for (size_t i = 0; i < points.size(); i++)
	{
		tetrahedron.controlPoints[i] = points[i];
	}
	return tetrahedron;
}

template <typename T, typename S>
inline S extractBezierTriangleFromTetrahedron(const T& tetrahedron, const int side)
{
	S bezierTriangle{};
	if constexpr (std::is_same<T, Tetrahedron2>())
	{
		if (side == 1)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[5];
		}
		else if (side == 2)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[6];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[9];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[7];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[2];
		}
		else if (side == 3)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[5];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[6];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[9];
		}
		else if (side == 4)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[7];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[9];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[5];
		}
		else
		{
			throw new std::runtime_error("extractBezierTriangleFromTetrahedron - side invalid");
		}
	}
	else if constexpr (std::is_same<T, Tetrahedron3>())
	{
		if (side == 1)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[5];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[6];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[7];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[9];
		}
		else if (side == 2)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[10];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[16];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[19];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[11];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[17];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[12];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[3];
		}
		else if (side == 3)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[7];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[9];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[10];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[13];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[15];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[16];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[18];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[19];
		}
		else if (side == 4)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[12];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[17];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[19];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[6];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[14];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[18];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[15];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[9];
		}
		else
		{
			throw new std::runtime_error("extractBezierTriangleFromTetrahedron - side invalid");
		}
	}
	else if constexpr (std::is_same<T, Tetrahedron4>())
	{
		if (side == 1)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[5];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[6];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[7];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[9];
			bezierTriangle.controlPoints[10] = tetrahedron.controlPoints[10];
			bezierTriangle.controlPoints[11] = tetrahedron.controlPoints[11];
			bezierTriangle.controlPoints[12] = tetrahedron.controlPoints[12];
			bezierTriangle.controlPoints[13] = tetrahedron.controlPoints[13];
			bezierTriangle.controlPoints[14] = tetrahedron.controlPoints[14];
		}
		else if (side == 2)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[15];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[25];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[31];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[34];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[1];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[16];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[26];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[32];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[2];
			bezierTriangle.controlPoints[10] = tetrahedron.controlPoints[17];
			bezierTriangle.controlPoints[11] = tetrahedron.controlPoints[27];
			bezierTriangle.controlPoints[12] = tetrahedron.controlPoints[3];
			bezierTriangle.controlPoints[13] = tetrahedron.controlPoints[18];
			bezierTriangle.controlPoints[14] = tetrahedron.controlPoints[4];
		}
		else if (side == 3)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[0];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[5];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[9];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[12];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[14];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[15];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[19];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[22];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[24];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[25];
			bezierTriangle.controlPoints[10] = tetrahedron.controlPoints[28];
			bezierTriangle.controlPoints[11] = tetrahedron.controlPoints[30];
			bezierTriangle.controlPoints[12] = tetrahedron.controlPoints[31];
			bezierTriangle.controlPoints[13] = tetrahedron.controlPoints[33];
			bezierTriangle.controlPoints[14] = tetrahedron.controlPoints[34];
		}
		else if (side == 4)
		{
			bezierTriangle.controlPoints[0] = tetrahedron.controlPoints[4];
			bezierTriangle.controlPoints[1] = tetrahedron.controlPoints[18];
			bezierTriangle.controlPoints[2] = tetrahedron.controlPoints[27];
			bezierTriangle.controlPoints[3] = tetrahedron.controlPoints[32];
			bezierTriangle.controlPoints[4] = tetrahedron.controlPoints[34];
			bezierTriangle.controlPoints[5] = tetrahedron.controlPoints[8];
			bezierTriangle.controlPoints[6] = tetrahedron.controlPoints[21];
			bezierTriangle.controlPoints[7] = tetrahedron.controlPoints[29];
			bezierTriangle.controlPoints[8] = tetrahedron.controlPoints[33];
			bezierTriangle.controlPoints[9] = tetrahedron.controlPoints[11];
			bezierTriangle.controlPoints[10] = tetrahedron.controlPoints[23];
			bezierTriangle.controlPoints[11] = tetrahedron.controlPoints[30];
			bezierTriangle.controlPoints[12] = tetrahedron.controlPoints[13];
			bezierTriangle.controlPoints[13] = tetrahedron.controlPoints[24];
			bezierTriangle.controlPoints[14] = tetrahedron.controlPoints[14];
		}
		else
		{
			throw new std::runtime_error("extractBezierTriangleFromTetrahedron - side invalid");
		}
	}
	else
	{
		static_assert(false, "Unsupported tetrahedron type");
	}

	auto aabb = tracer::AABB::fromBezierTriangle(bezierTriangle);
	bezierTriangle.aabb = Aabb{aabb.min, aabb.max};

	return bezierTriangle;
}

// inline BezierTriangle2 getBottomLeftSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
// {
// 	// new triangle
// 	//          b020
// 	//         /    \
// 	//        /      \
// 	//     b011      b110
// 	//     /    BL     \
// 	//    /             \
// 	// b002--- b101 ---b200
// 	constexpr auto idxb002 = getControlPointIndicesBezierTriangle2(0, 0, 2);
// 	constexpr auto idxb011 = getControlPointIndicesBezierTriangle2(0, 1, 1);
// 	constexpr auto idxb101 = getControlPointIndicesBezierTriangle2(1, 0, 1);

// 	// calculate the new 6 control points (np = new point)
// 	const vec3 np002 = bezierTriangle2.controlPoints[idxb002];
// 	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
// 	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);

// 	const vec3 np011
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb002] + bezierTriangle2.controlPoints[idxb011]);
// 	const vec3 np110 = 0.5f * (np020 + np200);
// 	const vec3 np101
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb002] + bezierTriangle2.controlPoints[idxb101]);

// 	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
// }

// inline BezierTriangle2 getBottomRightSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
// {
// 	// new triangle
// 	//          b020
// 	//         /    \
// 	//        /      \
// 	//     b011      b110
// 	//     /    BR     \
// 	//    /             \
// 	// b002--- b101 ---b200
// 	constexpr auto idxb200 = getControlPointIndicesBezierTriangle2(2, 0, 0);
// 	constexpr auto idxb101 = getControlPointIndicesBezierTriangle2(1, 0, 1);
// 	constexpr auto idxb110 = getControlPointIndicesBezierTriangle2(1, 1, 0);

// 	// calculate the new 6 control points (np = new point)
// 	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);
// 	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);
// 	const vec3 np200 = bezierTriangle2.controlPoints[idxb200];

// 	const vec3 np011 = 0.5f * (np002 + np020);
// 	const vec3 np110
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb200] + bezierTriangle2.controlPoints[idxb110]);
// 	const vec3 np101
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb200] + bezierTriangle2.controlPoints[idxb101]);

// 	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
// }

// inline BezierTriangle2 getTopSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
// {
// 	// new triangle
// 	//          b020
// 	//         /    \
// 	//        /      \
// 	//     b011      b110
// 	//     /     T     \
// 	//    /             \
// 	// b002--- b101 ---b200
// 	constexpr auto idxb020 = getControlPointIndicesBezierTriangle2(0, 2, 0);
// 	constexpr auto idxb011 = getControlPointIndicesBezierTriangle2(0, 1, 1);
// 	constexpr auto idxb110 = getControlPointIndicesBezierTriangle2(1, 1, 0);

// 	// calculate the new 6 control points (np = new point)
// 	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
// 	const vec3 np020 = bezierTriangle2.controlPoints[idxb020];
// 	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);

// 	const vec3 np011
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb020] + bezierTriangle2.controlPoints[idxb011]);
// 	const vec3 np110
// 	    = 0.5f * (bezierTriangle2.controlPoints[idxb020] + bezierTriangle2.controlPoints[idxb110]);
// 	const vec3 np101 = 0.5f * (np002 + np200);

// 	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
// }

// inline BezierTriangle2 getCenterSubdivisionTriangle2(const BezierTriangle2& bezierTriangle2)
// {
// 	// new triangle
// 	//          b020
// 	//         /    \
// 	//        /      \
// 	//     b011      b110
// 	//     /     C     \
// 	//    /             \
// 	// b002--- b101 ---b200

// 	// calculate the new 6 control points (np = new point)
// 	// Note: the triangle is flipped, for some reason it causes problem with the normal calculation

// 	// WARNING: getCenterSubdivisionTriangle2 - calculates wrong control points
// 	const vec3 np020 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0.5, 0);
// 	const vec3 np002 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0, 0.5, 0.5);
// 	const vec3 np200 = bezierTriangleSurfacePoint(bezierTriangle2.controlPoints, 2, 0.5, 0, 0.5);

// 	const vec3 np011 = 0.5f * (np002 + np020);
// 	const vec3 np110 = 0.5f * (np020 + np200);
// 	const vec3 np101 = 0.5f * (np002 + np200);

// 	return BezierTriangle2{np002, np200, np020, np101, np110, np011};
// }

// inline SubdividedTetrahedron2 subdivideBezierTriangle2(const BezierTriangle2& bezierTriangle2)
// {
// 	// T = top, BL = bottom left, BR = bottom right, C = center
// 	//          /\
// 	//         /  \
// 	//        / T  \
// 	//       /------\
// 	//     	/\  C   /\
// 	//     /  \    /  \
// 	//    / BL \  / BR \
// 	//    --------------

// 	return SubdividedTetrahedron2{
// 	    .bottomLeft = getBottomLeftSubdivisionTriangle2(bezierTriangle2),
// 	    .bottomRight = getBottomRightSubdivisionTriangle2(bezierTriangle2),
// 	    .top = getTopSubdivisionTriangle2(bezierTriangle2),
// 	    .center = getCenterSubdivisionTriangle2(bezierTriangle2),
// 	};
// }

} // namespace tracer
