#pragma once

#include <cstdio>
#include <cstdlib>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <vulkan/vulkan_core.h>

#include "bezier_math.hpp"
#include "blas.hpp"
#include "common_types.h"
#include "logger.hpp"
#include "raytracing_scene.hpp"

namespace tracer
{
namespace rt
{

template <unsigned int N>
inline size_t getControlPointIndicesTetrahedron(int i, int j, int k, int l)
{
	if constexpr (N == 1)
	{
		if (i == 0 && j == 0 && k == 0 && l == 1) return 0;
		if (i == 0 && j == 0 && k == 1 && l == 0) return 1;
		if (i == 0 && j == 1 && k == 0 && l == 0) return 2;
		if (i == 1 && j == 0 && k == 0 && l == 0) return 3;
	}
	else if constexpr (N == 2)
	{
		if (i == 0 && j == 0 && k == 0 && l == 2) return 0;
		if (i == 0 && j == 0 && k == 1 && l == 1) return 1;
		if (i == 0 && j == 0 && k == 2 && l == 0) return 2;
		if (i == 0 && j == 1 && k == 0 && l == 1) return 3;
		if (i == 0 && j == 1 && k == 1 && l == 0) return 4;
		if (i == 0 && j == 2 && k == 0 && l == 0) return 5;
		if (i == 1 && j == 0 && k == 0 && l == 1) return 6;
		if (i == 1 && j == 0 && k == 1 && l == 0) return 7;
		if (i == 1 && j == 1 && k == 0 && l == 0) return 8;
		if (i == 2 && j == 0 && k == 0 && l == 0) return 9;
	}
	else if constexpr (N == 3)
	{
		if (i == 0 && j == 0 && k == 0 && l == 3) return 0;
		if (i == 0 && j == 0 && k == 1 && l == 2) return 1;
		if (i == 0 && j == 0 && k == 2 && l == 1) return 2;
		if (i == 0 && j == 0 && k == 3 && l == 0) return 3;
		if (i == 0 && j == 1 && k == 0 && l == 2) return 4;
		if (i == 0 && j == 1 && k == 1 && l == 1) return 5;
		if (i == 0 && j == 1 && k == 2 && l == 0) return 6;
		if (i == 0 && j == 2 && k == 0 && l == 1) return 7;
		if (i == 0 && j == 2 && k == 1 && l == 0) return 8;
		if (i == 0 && j == 3 && k == 0 && l == 0) return 9;
		if (i == 1 && j == 0 && k == 0 && l == 2) return 10;
		if (i == 1 && j == 0 && k == 1 && l == 1) return 11;
		if (i == 1 && j == 0 && k == 2 && l == 0) return 12;
		if (i == 1 && j == 1 && k == 0 && l == 1) return 13;
		if (i == 1 && j == 1 && k == 1 && l == 0) return 14;
		if (i == 1 && j == 2 && k == 0 && l == 0) return 15;
		if (i == 2 && j == 0 && k == 0 && l == 1) return 16;
		if (i == 2 && j == 0 && k == 1 && l == 0) return 17;
		if (i == 2 && j == 1 && k == 0 && l == 0) return 18;
		if (i == 3 && j == 0 && k == 0 && l == 0) return 19;
	}
	else if constexpr (N == 4)
	{
		if (i == 0 && j == 0 && k == 0 && l == 4) return 0;
		if (i == 0 && j == 0 && k == 1 && l == 3) return 1;
		if (i == 0 && j == 0 && k == 2 && l == 2) return 2;
		if (i == 0 && j == 0 && k == 3 && l == 1) return 3;
		if (i == 0 && j == 0 && k == 4 && l == 0) return 4;
		if (i == 0 && j == 1 && k == 0 && l == 3) return 5;
		if (i == 0 && j == 1 && k == 1 && l == 2) return 6;
		if (i == 0 && j == 1 && k == 2 && l == 1) return 7;
		if (i == 0 && j == 1 && k == 3 && l == 0) return 8;
		if (i == 0 && j == 2 && k == 0 && l == 2) return 9;
		if (i == 0 && j == 2 && k == 1 && l == 1) return 10;
		if (i == 0 && j == 2 && k == 2 && l == 0) return 11;
		if (i == 0 && j == 3 && k == 0 && l == 1) return 12;
		if (i == 0 && j == 3 && k == 1 && l == 0) return 13;
		if (i == 0 && j == 4 && k == 0 && l == 0) return 14;
		if (i == 1 && j == 0 && k == 0 && l == 3) return 15;
		if (i == 1 && j == 0 && k == 1 && l == 2) return 16;
		if (i == 1 && j == 0 && k == 2 && l == 1) return 17;
		if (i == 1 && j == 0 && k == 3 && l == 0) return 18;
		if (i == 1 && j == 1 && k == 0 && l == 2) return 19;
		if (i == 1 && j == 1 && k == 1 && l == 1) return 20;
		if (i == 1 && j == 1 && k == 2 && l == 0) return 21;
		if (i == 1 && j == 2 && k == 0 && l == 1) return 22;
		if (i == 1 && j == 2 && k == 1 && l == 0) return 23;
		if (i == 1 && j == 3 && k == 0 && l == 0) return 24;
		if (i == 2 && j == 0 && k == 0 && l == 2) return 25;
		if (i == 2 && j == 0 && k == 1 && l == 1) return 26;
		if (i == 2 && j == 0 && k == 2 && l == 0) return 27;
		if (i == 2 && j == 1 && k == 0 && l == 1) return 28;
		if (i == 2 && j == 1 && k == 1 && l == 0) return 29;
		if (i == 2 && j == 2 && k == 0 && l == 0) return 30;
		if (i == 3 && j == 0 && k == 0 && l == 1) return 31;
		if (i == 3 && j == 0 && k == 1 && l == 0) return 32;
		if (i == 3 && j == 1 && k == 0 && l == 0) return 33;
		if (i == 4 && j == 0 && k == 0 && l == 0) return 34;
	}
	else
	{
		throw std::runtime_error("Degree N: " + std::to_string(N) + " not implemented");
	}

	throw std::runtime_error("Invalid index: i:" + std::to_string(i) + " j:" + std::to_string(j)
	                         + " k:" + std::to_string(k) + " l:" + std::to_string(l));
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
		throw std::runtime_error("Invalid index: i:" + std::to_string(i) + " j:" + std::to_string(j)
		                         + " k:" + std::to_string(k));
	}

	float fraction
	    = static_cast<float>(iter_factorial(n))
	      / static_cast<float>((iter_factorial(i) * iter_factorial(j) * iter_factorial(k)));

	float powi = static_cast<float>(glm::pow(u, i));
	float powj = static_cast<float>(glm::pow(v, j));
	float powk = static_cast<float>(glm::pow(w, k));

	return fraction * powi * powj * powk;
}

template <int N>
inline float
BernsteinPolynomialTrivariate(int i, int j, int k, int l, float u, float v, float w, float s)
{
	if (i < 0 || j < 0 || k < 0 || l < 0 || i > N || j > N || k > N || l > N)
	{
		return 0;
	}

	float fraction = static_cast<float>(iter_factorial(N))
	                 / static_cast<float>((iter_factorial(i) * iter_factorial(j) * iter_factorial(k)
	                                       * iter_factorial(l)));

	float powi = static_cast<float>(glm::pow(u, i));
	float powj = static_cast<float>(glm::pow(v, j));
	float powk = static_cast<float>(glm::pow(w, k));
	float powl = static_cast<float>(glm::pow(s, l));

	return fraction * powi * powj * powk * powl;
}

template <typename T>
inline glm::vec3 bezierTetrahedronPointInVolume(
    const T& tetrahedron, const float u, const float v, const float w, const float s)
{
	constexpr const int N = degree<T>();
	auto sum = glm::vec3();
	for (int k = 0; k <= N; k++)
	{
		for (int j = 0; j <= N - k; j++)
		{
			for (int i = 0; i <= N - k - j; i++)
			{
				// NOTE: due to how the barycentric indices work, we need to add one dimension
				auto l = N - i - j - k;
				if (i + j + k + l <= N)
				{
					size_t idx = getControlPointIndicesTetrahedron<N>(i, j, k, l);
					auto& controlPoint = tetrahedron.controlPoints[idx];
					auto bernsteinValue = BernsteinPolynomialTrivariate<N>(i, j, k, l, u, v, w, s);
					auto value = controlPoint * bernsteinValue;
					sum += value;
					// std::printf("u: %f v: %f w: %f s: %f i: %d j: %d k: %d l: %d idx: %ld Point:
					// "
					//             "(%f,%f, %f) bernsteinValue: %f value: (%f,%f, %f)\n",
					//             u,
					//             v,
					//             w,
					//             s,
					//             i,
					//             j,
					//             k,
					//             l,
					//             idx,
					//             controlPoint.x,
					//             controlPoint.y,
					//             controlPoint.z,
					//             bernsteinValue,
					//             value.x,
					//             value.y,
					//             value.z);
				}
			}
		}
	}
	// std::printf("Sum: (%f,%f, %f)\n", sum.x, sum.y, sum.z);
	return sum;
}

template <typename T>
inline glm::vec3 BezierTrianglePoint(const T& triangle, const float u, const float v, const float w)
{
	constexpr const int N = degree<T>();
	vec3 sum = vec3(0);
	// check if values are in the right range and if sum is equal to 1
	// if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1 || (abs(u + v + w - 1) > 0.00001))
	// {
	// 	return sum;
	// }

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= N; k++)
	{
		for (int j = 0; j <= N - k; j++)
		{
			for (int i = 0; i <= N - k - j; i++)
			{
				if (i + j + k == N)
				{
					size_t idx
					    = static_cast<size_t>(getControlPointIndicesBezierTriangle2(i, j, k));
					sum += triangle.controlPoints[idx]
					       * BernsteinPolynomialBivariate(N, i, j, k, u, v, w);
				}
			}
		}
	}
	return sum;
}

template <typename T>
inline glm::vec3
partialBezierTriangleDirectional(const T& triangle, vec3 direction, float u, float v)
{
	constexpr const int N = degree<T>();
	vec3 sum = vec3(0);
	float w = 1.0f - u - v;

	// TODO: remove this loop and write out the formula
	for (int k = 0; k <= N; k++)
	{
		for (int j = 0; j <= N - k; j++)
		{
			for (int i = 0; i <= N - k - j; i++)
			{
				if (i + j + k == N - 1)
				{
					size_t be1 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 1, j + 0, k + 0));
					size_t be2 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 0, j + 1, k + 0));
					size_t be3 = static_cast<size_t>(
					    getControlPointIndicesBezierTriangle2(i + 0, j + 0, k + 1));
					vec3 x = triangle.controlPoints[be1] * direction.x;
					vec3 y = triangle.controlPoints[be2] * direction.y;
					vec3 z = triangle.controlPoints[be3] * direction.z;
					sum += (x + y + z) * BernsteinPolynomialBivariate(N - 1, i, j, k, u, v, w);
				}
			}
		}
	}
	return static_cast<float>(N) * sum;
}

template <typename T>
inline glm::vec2 fBezierTriangle(const T& triangle,
                                 const vec3 origin,
                                 const vec3 n1,
                                 const vec3 n2,
                                 const float u,
                                 const float v,
                                 const float w)
{
	vec3 surfacePoint = BezierTrianglePoint(triangle, u, v, w);

	// project onto planes
	float d1 = dot(-n1, origin);
	float d2 = dot(-n2, origin);
	return vec2(dot(n1, surfacePoint) + d1, dot(n2, surfacePoint) + d2);
}

template <typename T>
inline glm::mat2x2 jacobianBezierTriangle(
    const T& triangle, const vec3 n1, const vec3 n2, const float u, const float v)
{
	vec3 dirU = vec3(1, 0, -1);
	vec3 dirV = vec3(0, 1, -1);
	vec3 partialDerivative1 = partialBezierTriangleDirectional(triangle, dirU, u, v);
	vec3 partialDerivative2 = partialBezierTriangleDirectional(triangle, dirV, u, v);

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

template <typename T>
inline bool newtonsMethodTriangle2([[maybe_unused]] RaytracingScene& raytracingScene,
                                   const RaytracingDataConstants& raytracingDataConstants,
                                   glm::vec3& intersectionPoint,
                                   const glm::vec2 initialGuess,
                                   const glm::vec3 origin,
                                   const T& triangle,
                                   const glm::vec3 n1,
                                   const glm::vec3 n2)
{

	bool hit = false;
	const int max_iterations = 100 + 1;
	vec2 u[max_iterations];

	float toleranceF = raytracingDataConstants.newtonErrorFTolerance;

	u[0] = initialGuess;
	for (int c = 1; c < max_iterations; c++)
	{
		u[c] = vec2(0);
	}

	int c = 0;
	float previousErrorF = 100000.0;
	float errorF = 100000.0;

	for (; c < raytracingDataConstants.newtonMaxIterations; c++)
	{
		glm::mat2x2 j = jacobianBezierTriangle(triangle, n1, n2, u[c].x, u[c].y);
		glm::mat2x2 inv_j = inverseJacobian(j);
		if (inv_j == glm::mat2x2(0, 0, 0, 0))
		{
			hit = false;
			break;
		}

		glm::vec2 f_value
		    = fBezierTriangle(triangle, origin, n1, n2, u[c].x, u[c].y, 1.0f - u[c].x - u[c].y);

		previousErrorF = errorF;
		errorF = glm::abs(f_value.x) + glm::abs(f_value.y);

		vec2 differenceInUV = inv_j * f_value;
		u[c + 1] = u[c] - differenceInUV;

		vec3 surfacePoint = BezierTrianglePoint(triangle, u[c].x, u[c].y, 1.0f - u[c].x - u[c].y);
		auto& sceneObject = raytracingScene.createSceneObject(surfacePoint);
		raytracingScene.addObjectSphere(
		    sceneObject,
		    surfacePoint,
		    true,
		    0.1f
		        * (static_cast<float>(raytracingDataConstants.newtonMaxIterations - c)
		           / static_cast<float>(raytracingDataConstants.newtonMaxIterations)),
		    ColorIdx::t_purple);

		// TODO: check if we really don't wanna allow increases in the error during the newton
		// search abort if the error is increasing
		if ((glm::abs(raytracingDataConstants.newtonErrorFIgnoreIncrease) < 1e-8)
		    && errorF > previousErrorF)
		{
			hit = false;
			break;
		}

		if (errorF <= toleranceF)
		{
			hit = raytracingDataConstants.newtonErrorFHitBelowTolerance > 0.0;
			break;
		}
	}

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
		    = BezierTrianglePoint(triangle, u[idx].x, u[idx].y, 1.0f - u[idx].x - u[idx].y);
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

	auto& sceneObject = raytracingScene.createSceneObject();
	float stepSize = 0.1f;
	for (float x = 0; x <= sizeX; x += stepSize)
	{
		for (float z = 0; z <= sizeZ; z += stepSize)
		{
			// auto pos = glm::dot(pointOnPlane, normal);
			auto pos = pointOnPlane + (x - sizeX / 2.0f) * b2 + (z - sizeZ / 2.0f) * b3;
			raytracingScene.addObjectSphere(sceneObject, pos, false, 0.005f, ColorIdx::t_pink);
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

/// Visualizes control points
template <typename T>
inline void visualizeTetrahedronControlPoints(SceneObject& sceneObject,
                                              RaytracingScene& raytracingScene,
                                              const T& tetrahedron)
{
	for (auto& point : tetrahedron.controlPoints)
	{
		raytracingScene.addObjectSphere(sceneObject, point, false, 0.06f, ColorIdx::t_orange);
	}
}

/// Visualize a tetrahedron by sampling points on the surface
template <typename T>
inline void visualizeTetrahedronSides(SceneObject& sceneObject,
                                      RaytracingScene& raytracingScene,
                                      const T& tetrahedron,
                                      const bool visualizeSides,
                                      const bool visualizeVolume,
                                      float stepSize = 0.1f)
{
	for (float u = 0; u <= 1.0f + 1e-4f; u += stepSize)
	{
		for (float v = 0; v <= 1.0f + 1e-4f; v += stepSize)
		{
			for (float w = 0; w <= 1.0f + 1e-4f; w += stepSize)
				if (u + v + w <= 1.0f + 1e-4f)
				{
					float s = 1.0f - u - v - w;

					auto p = bezierTetrahedronPointInVolume(tetrahedron, u, v, w, s);

					auto isFace1 = glm::abs(u) < 1e-4 && v + w <= 1;
					if (visualizeSides && isFace1)
					{
						raytracingScene.addObjectSphere(
						    sceneObject, p, false, 0.01f, ColorIdx::t_red);
					}

					auto isFace2 = glm::abs(v) < 1e-4 && u + w <= 1;
					if (visualizeSides && isFace2)
					{
						raytracingScene.addObjectSphere(
						    sceneObject, p, false, 0.01f, ColorIdx::t_purple);
					}

					auto isFace3 = glm::abs(w) < 1e-4 && u + v <= 1;
					if (visualizeSides && isFace3)
					{
						raytracingScene.addObjectSphere(
						    sceneObject, p, false, 0.01f, ColorIdx::t_green);
					}

					auto isFace4 = glm::abs(u + v + w - 1.0f) <= 1e-4;
					if (visualizeSides && isFace4)
					{
						raytracingScene.addObjectSphere(
						    sceneObject, p, false, 0.01f, ColorIdx::t_blue);
					}

					if (visualizeVolume && !isFace1 && !isFace2 && !isFace3 && !isFace4)
					{
						raytracingScene.addObjectSphere(
						    sceneObject, p, false, 0.02f, ColorIdx::t_black);
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
