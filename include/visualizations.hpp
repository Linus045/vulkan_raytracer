#pragma once

#include <cstdlib>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "bezier_math.hpp"
#include "common_types.h"
#include "raytracing_scene.hpp"

namespace tracer
{
namespace rt
{

inline size_t getControlPointIndices(int i, int j, int k)
{
	if (i == 0 && j == 0 && k == 0) return 0;
	if (i == 2 && j == 0 && k == 0) return 1;
	if (i == 0 && j == 2 && k == 0) return 2;
	if (i == 0 && j == 0 && k == 2) return 3;
	if (i == 1 && j == 0 && k == 0) return 4;
	if (i == 0 && j == 1 && k == 0) return 5;
	if (i == 0 && j == 0 && k == 1) return 6;
	if (i == 1 && j == 1 && k == 0) return 7;
	if (i == 1 && j == 0 && k == 1) return 8;
	if (i == 0 && j == 1 && k == 1) return 9;

	throw std::runtime_error("Invalid index");
}

inline int iter_factorial(int n)
{
	int ret = 1;
	for (int i = 1; i <= n; ++i)
		ret *= i;
	return ret;
}

inline float BernsteinPolynomialBivariate(int n, int i, int j, int k, float u, float v, float w)
{
	if (i < 0 || j < 0 || k < 0)
	{
		return 0;
	}

	float fraction
	    = static_cast<float>(iter_factorial(n))
	      / static_cast<float>((iter_factorial(i) * iter_factorial(j) * iter_factorial(k)));

	float powi = static_cast<float>(glm::pow(u, i));
	float powj = static_cast<float>(glm::pow(v, j));
	float powk = static_cast<float>(glm::pow(w, k));

	return fraction * powi * powj * powk;
}

template <unsigned int C>
inline glm::vec3
bezierVolumePoint(const std::array<glm::vec3, C> controlPoints, int n, float u, float v, float w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k <= n)
				{
					size_t idx = getControlPointIndices(i, j, k);
					sum += controlPoints[idx] * BernsteinPolynomialBivariate(n, i, j, k, u, v, w);
				}
			}
		}
	}
	// std::cout << "---------------\n";
	return sum;
}

inline glm::vec3 BezierTriangle2Point(const std::array<glm::vec3, 6> controlPoints,
                                      const float u,
                                      const float v,
                                      const float w)
{
	int n = 2;
	vec3 sum = vec3(0);
	// check if values are in the right range and if sum is equal to 1
	// if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1 || (abs(u + v + w - 1) > 0.00001))
	// {
	// 	return sum;
	// }

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n)
				{
					size_t idx
					    = static_cast<size_t>(getControlPointIndicesBezierTriangle2(i, j, k));
					sum += controlPoints[idx] * BernsteinPolynomialBivariate(n, i, j, k, u, v, w);
				}
			}
		}
	}
	return sum;
}

inline glm::vec3 partialBezierTriangle2Directional(const std::array<glm::vec3, 6> controlPoints,
                                                   vec3 direction,
                                                   float u,
                                                   float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0f - u - v;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n - 1)
				{
					size_t be1 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 1, j + 0, k + 0));
					size_t be2 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 0, j + 1, k + 0));
					size_t be3 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 0, j + 0, k + 1));
					vec3 x = controlPoints[be1] * direction.x;
					vec3 y = controlPoints[be2] * direction.y;
					vec3 z = controlPoints[be3] * direction.z;
					sum += (x + y + z) * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	return static_cast<float>(n) * sum;
}

inline glm::vec2 fBezierTriangle(const std::array<glm::vec3, 6> controlPoints,
                                 const vec3 origin,
                                 const vec3 n1,
                                 const vec3 n2,
                                 const float u,
                                 const float v,
                                 const float w)
{
	vec3 surfacePoint = BezierTriangle2Point(controlPoints, u, v, w);

	// project onto planes
	float d1 = dot(-n1, origin);
	float d2 = dot(-n2, origin);
	return vec2(dot(n1, surfacePoint) + d1, dot(n2, surfacePoint) + d2);
}

inline glm::mat2x2 jacobianBezierTriangle2(const std::array<glm::vec3, 6> controlPoints,
                                           const vec3 n1,
                                           const vec3 n2,
                                           const float u,
                                           const float v)
{
	vec3 dirU = vec3(1, 0, -1);
	vec3 dirV = vec3(0, 1, -1);
	vec3 partialDerivative1 = partialBezierTriangle2Directional(controlPoints, dirU, u, v);
	vec3 partialDerivative2 = partialBezierTriangle2Directional(controlPoints, dirV, u, v);

	return glm::mat2x2(dot(n1, partialDerivative1),
	                   dot(n2, partialDerivative1),
	                   dot(n1, partialDerivative2),
	                   dot(n2, partialDerivative2));
}

inline glm::mat2x2 inverseJacobian(const glm::mat2x2 J)
{
	float d = determinant(J);
	if (glm::abs(d) < 0.00001)
	{
		return glm::mat2x2(0, 0, 0, 0);
	}

	glm::mat2x2 adj = glm::mat2x2(J[1][1], -J[0][1], -J[1][0], J[0][0]);
	glm::mat2x2 mat = 1.0f / d * adj;
	return mat;
}

inline bool newtonsMethodTriangle2([[maybe_unused]] RaytracingScene& raytracingScene,
                                   const RaytracingDataConstants& raytracingDataConstants,
                                   glm::vec3& intersectionPoint,
                                   const glm::vec2 initialGuess,
                                   const glm::vec3 origin,
                                   const std::array<glm::vec3, 6> controlPoints,
                                   const glm::vec3 n1,
                                   const glm::vec3 n2)
{
	bool hit = false;
	const int max_iterations = 100 + 1;
	vec2 u[max_iterations];

	[[maybe_unused]] float toleranceX = raytracingDataConstants.newtonErrorXTolerance;
	float toleranceF = raytracingDataConstants.newtonErrorFTolerance;

	u[0] = initialGuess;
	for (int c = 1; c < max_iterations; c++)
	{
		u[c] = vec2(0);
	}

	int c = 0;
	float previousErrorF = 100000.0;
	float errorF = 100000.0;

	[[maybe_unused]] float previousErrorX = 100000.0;
	float errorX = 100000.0;

	for (; c < raytracingDataConstants.newtonMaxIterations; c++)
	{
		glm::mat2x2 j = jacobianBezierTriangle2(controlPoints, n1, n2, u[c].x, u[c].y);
		glm::mat2x2 inv_j = inverseJacobian(j);
		if (inv_j == glm::mat2x2(0, 0, 0, 0))
		{
			hit = false;
			break;
		}

		glm::vec2 f_value = fBezierTriangle(
		    controlPoints, origin, n1, n2, u[c].x, u[c].y, 1.0f - u[c].x - u[c].y);

		previousErrorF = errorF;
		errorF = glm::abs(f_value.x) + glm::abs(f_value.y);

		vec2 differenceInUV = inv_j * f_value;
		u[c + 1] = u[c] - differenceInUV;

		// previousErrorX = errorX;
		// errorX = abs(differenceInUV.x) + abs(differenceInUV.y);

		vec3 surfacePoint
		    = BezierTriangle2Point(controlPoints, u[c].x, u[c].y, 1.0f - u[c].x - u[c].y);
		raytracingScene.addObjectSphere(
		    surfacePoint,
		    0.1f
		        * (static_cast<float>(raytracingDataConstants.newtonMaxIterations - c)
		           / static_cast<float>(raytracingDataConstants.newtonMaxIterations)),
		    ColorIdx::t_purple);
		std::printf("c: %d, n1: (%.2f, %.2f, %.2f), n2: "
		            "(%.2f, %.2f, %.2f), f: (%.2f, %.2f), differenceInUV:(%.2f, %.2f), u: "
		            "(%.2f, %.2f), u+1: (%.2f, %.2f), "
		            "errorX: (%.5f), errorF: (%.5f), j:(%.2f, %.2f, %.2f, %.2f),  inv_j:(%.2f, "
		            "%.2f, %.2f, %.2f) surfacePoint (%.2f, %.2f, %.2f)\n\n",
		            c,
		            n1.x,
		            n1.y,
		            n1.z,
		            n2.x,
		            n2.y,
		            n2.z,
		            f_value.x,
		            f_value.y,
		            differenceInUV.x,
		            differenceInUV.y,
		            u[c].x,
		            u[c].y,
		            u[c + 1].x,
		            u[c + 1].y,
		            errorX,
		            errorF,
		            j[0][0],
		            j[0][1],
		            j[1][0],
		            j[1][1],
		            inv_j[0][0],
		            inv_j[0][1],
		            inv_j[1][0],
		            inv_j[1][1],
		            surfacePoint.x,
		            surfacePoint.y,
		            surfacePoint.z);

		if ((glm::abs(raytracingDataConstants.newtonErrorFIgnoreIncrease) < 1e-8)
		    && errorF > previousErrorF)
		{
			// debugPrintfEXT("abort: errorF increased: errorF: %.8f, previousErrorF: %.8f",
			//                errorF,
			//                previousErrorF);
			hit = false;
			break;
		}

		if (errorF <= toleranceF)
		{
			hit = raytracingDataConstants.newtonErrorFHitBelowTolerance > 0.0;
			// debugPrintfEXT("hit: errorF <= toleranceF: (%.5f)", errorF);
			break;
		}

		// if (raytracingDataConstants.newtonErrorXIgnoreIncrease == 0.0 && errorX > previousErrorX)
		// {
		// 	if (isCrosshairRay)
		// 	{
		// 		debugPrintfEXT("abort: errorX increased: errorX: %.5f, previousErrorX: %.5f",
		// 		               errorX,
		// 		               previousErrorX);
		// 	}
		// 	hit = false;
		// 	break;
		// }

		// // TODO: maybe we want different tolerances for errorX and errorF
		// if (errorX <= toleranceX)
		// {
		// 	hit = raytracingDataConstants.newtonErrorXHitBelowTolerance > 0.0;
		// 	if (isCrosshairRay)
		// 	{
		// 		debugPrintfEXT("hit: errorX <= toleranceX (%.5f)", errorX);
		// 	}
		// 	break;
		// }
	}
	//	}

	std::printf("loop ended: hit: %d\n", hit);

	intersectionPoint = vec3(0);
	if (hit)
	{
		int idx = c;

		if (u[idx].x < 0 || u[idx].y < 0 || (u[idx].x + u[idx].y) > 1)
		{
			hit = false;
			return hit;
		}

		intersectionPoint
		    = BezierTriangle2Point(controlPoints, u[idx].x, u[idx].y, 1.0f - u[idx].x - u[idx].y);
		// debugPrintfEXT("u: (%.2v2f), intersectionPoint: (%.2v3f)", u[idx],
		// intersectionPoint);
		glm::vec2 hitCoordinate = vec2(u[idx].x, u[idx].y);
		std::printf("hitCoordinate: (%.2f, %.2f)", hitCoordinate.x, hitCoordinate.y);
	}
	return hit;
}

inline void visualizeVector(std::vector<Sphere>& spheres,
                            const glm::vec3& startPos,
                            const glm::vec3& direction,
                            const float length,
                            const float sphereSize = 0.06f)
{
	float stepSize = 0.01f;
	for (float s = 0; s <= length; s += stepSize)
	{
		auto pos = startPos + direction * s;
		spheres.emplace_back(pos, sphereSize, 2);
	}
}

inline void visualizePlane(RaytracingScene& raytracingScene,
                           const glm::vec3& normal,
                           const glm::vec3& pointOnPlane,
                           const float sizeX,
                           [[maybe_unused]] const float sizeZ)
{
	glm::vec3 u;
	if (glm::abs(normal.z) < glm::abs(normal.x))
	{
		u = glm::vec3(0, 0, -1);
	}
	else
	{
		u = glm::vec3(1, 0, 0);
	}

	glm::vec3 b1 = glm::normalize(glm::cross(normal, u));
	glm::vec3 b2 = glm::normalize(glm::cross(normal, b1));
	glm::vec3 b3 = glm::normalize(glm::cross(normal, b2));

	float stepSize = 0.1f;
	for (float x = 0; x <= sizeX; x += stepSize)
	{
		for (float z = 0; z <= sizeZ; z += stepSize)
		{
			// auto pos = glm::dot(pointOnPlane, normal);
			auto pos = pointOnPlane + (x - sizeX / 2.0f) * b2 + (z - sizeZ / 2.0f) * b3;
			raytracingScene.addObjectSphere(pos, 0.005f, ColorIdx::t_pink);
		}
	}
}

inline void visualizeBezierSurface([[maybe_unused]] std::vector<Sphere>& spheres,
                                   [[maybe_unused]] RectangularBezierSurface2x2 surface)
{
	// double stepSize = 0.1f;
	// for (double u = 0; u <= 1; u += stepSize)
	// {
	// 	for (double v = 0; v <= 1; v += stepSize)
	// 	{
	// 		spheres.emplace_back(bezierSurfacePoint(surface.controlPoints, 3, 3, u, v), 0.05f,
	// 1);
	// 	}
	// }

	// vec3 rayOrigin = glm::vec3(8, 1, 0);
	// vec3 rayDirection = glm::vec3(0, 0, 1);

	// for (float v = 0; v <= 15; v += static_cast<float>(stepSize))
	// {
	// 	spheres.emplace_back(rayOrigin + rayDirection * v, 0.01f, 4);
	// }

	// auto n1 = glm::vec3(1, 0, 0);
	// auto n2 = glm::vec3(0, 1, 0);

	// // for (int i = 0; i < 16; i++)
	// //{
	// //	auto projectedPoint = projectPoint(surface.controlPoints[i], rayOrigin, n1, n2);
	// //	spheres.emplace_back(glm::vec3(projectedPoint.x, projectedPoint.y, 0), 0.4, 5);
	// // }

	// glm::vec3 intersectionPoint;
	// glm::vec2 initialGuess = glm::vec2(1, 1);

	// // glm::vec2 initialGuess = projectPoint(surface.controlPoints[10], rayOrigin, n1, n2);
	// std::cout << "Initial guess: " << initialGuess.x << " " << initialGuess.y << std::endl;
	// if (newtonsMethod(spheres,
	//                   intersectionPoint,
	//                   initialGuess,
	//                   rayOrigin,
	//                   surface.controlPoints,
	//                   3,
	//                   3,
	//                   n1,
	//                   n2))
	// {
	// 	std::cout << "HIT!" << std::endl;
	// 	spheres.emplace_back(intersectionPoint, 0.1f, 6);
	// }

	// for (int i = 0; i < 16; i++)
	// {
	// 	auto controlPointPos = surface.controlPoints[i];
	// 	spheres.emplace_back(controlPointPos, 0.1f, 2);
	// }

	// // Visualize Normals in corners
	// {
	// 	int i = 0, j = 0;
	// 	auto dirA = surface.controlPoints[convert2Dto1D_index(4, i + 1, j)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto dirB = surface.controlPoints[convert2Dto1D_index(4, i, j + 1)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto normal = glm::cross(dirA, dirB);

	// 	auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	visualizeVector(spheres, controlPointPos, normal, 0.1f);
	// }
	// {
	// 	int i = 3, j = 0;
	// 	auto dirA = surface.controlPoints[convert2Dto1D_index(4, i, j + 1)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto dirB = surface.controlPoints[convert2Dto1D_index(4, i - 1, j)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto normal = glm::cross(dirA, dirB);

	// 	auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	visualizeVector(spheres, controlPointPos, normal, 0.1f);
	// }
	// {
	// 	int i = 0, j = 3;
	// 	auto dirA = surface.controlPoints[convert2Dto1D_index(4, i, j - 1)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto dirB = surface.controlPoints[convert2Dto1D_index(4, i + 1, j)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto normal = glm::cross(dirA, dirB);

	// 	auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	visualizeVector(spheres, controlPointPos, normal, 0.1f);
	// }
	// {
	// 	int i = 3, j = 3;
	// 	auto dirA = surface.controlPoints[convert2Dto1D_index(4, i - 1, j)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto dirB = surface.controlPoints[convert2Dto1D_index(4, i, j - 1)]
	// 	            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	auto normal = glm::cross(dirA, dirB);

	// 	auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
	// 	visualizeVector(spheres, controlPointPos, normal, 0.1f);
	// }
}

template <typename T, unsigned int C>
inline void visualizeTetrahedron([[maybe_unused]] RaytracingScene& raytracingScene,
                                 [[maybe_unused]] const T& tetrahedron)
{
	// Visualize control points
	for (auto& point : tetrahedron.controlPoints)
	{
		raytracingScene.addObjectSphere(point, 0.04f, ColorIdx::t_black);
	}

	float stepSize = 0.1f;
	for (float u = 0; u <= 1; u += stepSize)
	{
		for (float v = 0; v <= 1; v += stepSize)
		{
			for (float w = 0; w <= 1; w += stepSize)
			{
				if (u + v + w <= 1)
				{
					auto p = bezierVolumePoint<C>(
					    std::to_array(tetrahedron.controlPoints), 2, u, v, w);

					auto isFace1 = glm::abs(u) < 1e-4 && v + w <= 1;
					if (isFace1)
					{
						raytracingScene.addObjectSphere(p, 0.01f, ColorIdx::t_red);
					}

					auto isFace2 = glm::abs(v) < 1e-4 && u + w <= 1;
					if (isFace2)
					{
						raytracingScene.addObjectSphere(p, 0.01f, ColorIdx::t_purple);
					}

					auto isFace3 = glm::abs(w) < 1e-4 && u + v <= 1;
					if (isFace3)
					{
						raytracingScene.addObjectSphere(p, 0.01f, ColorIdx::t_green);
					}

					auto isFace4 = glm::abs(u + v + w - 1) <= 1e-4;
					if (isFace4)
					{
						raytracingScene.addObjectSphere(p, 0.01f, ColorIdx::t_white);
					}

					if (!isFace1 && !isFace2 && !isFace3 && !isFace4)
					{
						// raytracingScene.addSphere(p, 0.01f, ColorIdx::t_black);
					}
				}
			}
		}
	}
}

inline bool intersectWithPlane(const glm::vec3 planeNormal,
                               const glm::vec3 planeOrigin,
                               const glm::vec3 rayOrigin,
                               const glm::vec3 rayDirection,
                               float& t)
{
	// debugPrintfEXT("Test intesection with Plane: RayOrigin: (%v3f) rayDirection: (%v3f)
	// PlaneNormal(%v3f) PlanePosition:(%v3f)",rayOrigin, rayDirection, planeNormal,
	// planeOrigin);
	float denom = dot(planeNormal, rayDirection);
	if (denom > 1e-6)
	{
		glm::vec3 rayToPlanePoint = planeOrigin - rayOrigin;
		t = dot(rayToPlanePoint, planeNormal) / denom;
		return t >= 0;
	}
	return false;
}

inline glm::vec3 IntersectPlane(
    const glm::vec3 rayOrigin, const glm::vec3 direction, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2)
{
	glm::vec3 D = direction;
	glm::vec3 N = glm::cross(p1 - p0, p2 - p0);
	glm::vec3 X = rayOrigin + D * dot(p0 - rayOrigin, N) / dot(D, N);

	return X;
}

inline float PointInOrOn(glm::vec3 P1, glm::vec3 P2, glm::vec3 A, glm::vec3 B)
{
	glm::vec3 CP1 = glm::cross(B - A, P1 - A);
	glm::vec3 CP2 = glm::cross(B - A, P2 - A);
	return glm::step(0.0f, glm::dot(CP1, CP2));
}

inline float PointInTriangle(glm::vec3 px, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2)
{
	return PointInOrOn(px, p0, p1, p2) * PointInOrOn(px, p1, p2, p0) * PointInOrOn(px, p2, p0, p1);
}

inline bool IntersectTriangle(const glm::vec3 rayOrigin,
                              const glm::vec3 direction,
                              glm::vec3 p0,
                              glm::vec3 p1,
                              glm::vec3 p2,
                              float& t)
{
	vec3 X = IntersectPlane(rayOrigin, direction, p0, p1, p2);

	if (PointInTriangle(X, p0, p1, p2) > 0)
	{
		float dist = distance(rayOrigin, X);
		t = dist;
		return dist > 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// TEST
////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

inline glm::vec3 casteljauAlgorithmIntermediatePoint(
    vec3 controlPoints[6], int r, int i, int j, int k, float u, float v, float w)
{
	vec3 sum = vec3(0);
	int n = 2;
	for (int jk = 0; jk <= n; jk++)
	{
		for (int jj = 0; jj <= n - jk; jj++)
		{
			for (int ji = 0; ji <= n - jk - jj; ji++)
			{
				if (ji + jj + jk == r)
				{
					int idx = getControlPointIndicesBezierTriangle2(i + ji, j + jj, k + jk);
					sum += controlPoints[idx]
					       * BernsteinPolynomialBivariate(r, ji, jj, jk, u, v, w);
				}
			}
		}
	}

	return sum;
}

inline glm::vec3 deCasteljauBezierTrianglePoint(vec3 controlPoints[6], float u, float v, float w)
{
	vec3 sum = vec3(0);
	int n = 2;
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n)
				{
					int idx = getControlPointIndicesBezierTriangle2(i, j, k);
					sum += controlPoints[idx] * BernsteinPolynomialBivariate(n, i, j, k, u, v, w);
				}
			}
		}
	}

	return sum;
}

inline vec3 partialBezierTriangle2U(vec3 controlPoints[6], float u, float v)
{
	int n = 2;
	vec3 sum = vec3(0);
	float w = 1.0f - u - v;

	int dx = 1;
	int dy = 0;
	int dz = 0;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (i + j + k == n - 1)
				{
					sum += casteljauAlgorithmIntermediatePoint(
					           controlPoints, 1, dx, dy, dz, u, v, w)
					       * BernsteinPolynomialBivariate(n - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	sum *= n;
	return sum;
}

} // namespace rt
} // namespace tracer
