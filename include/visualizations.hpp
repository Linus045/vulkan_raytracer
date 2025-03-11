#pragma once

#include <functional>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include <ostream>
#include <tuple>

#include "logger.hpp"
#include "common_types.h"
#include "raytracing_scene.hpp"

namespace ltracer
{
namespace rt
{

using HSideFunc = std::function<glm::vec3(
    const std::array<glm::vec3, 10> controlPoints, int n, const double v, const double w)>;

using PartialFunc = std::function<glm::vec3(
    const std::array<glm::vec3, 10> controlPoints, const double v, const double w)>;

// TODO: maybe convert this file into some kind of Scene class that holds all the objects
// and does the updating

// TODO: maybe change the implementation of this
inline double nChooseK(int n, int k)
{
	if (k == 0)
	{
		return 1;
	}
	return n * nChooseK(n - 1, k - 1) / k;
}

template <typename T>
inline T BernsteinPolynomial(int i, int n, double x)
{
	if (i > n) return 0;
	if (i < 0) return 0;
	return static_cast<T>(nChooseK(n, i) * pow(x, i) * pow(1 - x, n - i));
}

inline size_t convert2Dto1D_index(int columns, int i, int j)
{
	return static_cast<size_t>(i * columns + j);
}

inline size_t convert3Dto1D_index(int columns, int depth, int i, int j, int k)
{
	return static_cast<size_t>(i * columns * depth + j * depth + k);
}

inline vec3 bezierSurfacePoint(const glm::vec3 controlPoints[16], int n, int m, double u, double v)
{
	auto sum = glm::vec3();
	for (int i = 0; i <= n; i++)
	{
		for (int j = 0; j <= m - i; j++)
		{
			// TODO: do we really want/need to use double precision here?
			size_t idx = convert2Dto1D_index(n + 1, i, j);
			sum += controlPoints[idx] * BernsteinPolynomial<float>(i, n, u)
			       * BernsteinPolynomial<float>(j, m, v);
		}
	}
	return sum;
}

// TODO: this is only temporary, use a proper calculation
constexpr std::tuple<int, int, int> getControlPointIndices(size_t idx)
{
	switch (idx)
	{
	case 0:
		return std::make_tuple(0, 0, 0);
	case 1:
		return std::make_tuple(2, 0, 0);
	case 2:
		return std::make_tuple(0, 2, 0);
	case 3:
		return std::make_tuple(0, 0, 2);

	case 4:
		return std::make_tuple(1, 0, 0);
	case 5:
		return std::make_tuple(0, 1, 0);
	case 6:
		return std::make_tuple(0, 0, 1);
	case 7:
		return std::make_tuple(1, 1, 0);
	case 8:
		return std::make_tuple(1, 0, 1);

	case 9:
		return std::make_tuple(0, 1, 1);
	}
	throw std::runtime_error("Invalid index");
}

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

inline int factorial(int n)
{
	// NOTE : we only work with small numbers, so we can (hopefully) safely use int instead of
	// unsigned ints
	int ret = 1;
	for (int i = 1; i <= n; ++i)
		ret *= i;
	return static_cast<int>(ret);
}

inline glm::vec3 bezierVolumePoint(
    const std::array<glm::vec3, 10> controlPoints, int n, double u, double v, double w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= 2; k++)
	{
		for (int j = 0; j <= 2 - k; j++)
		{
			for (int i = 0; i <= 2 - k - j; i++)
			{
				auto d = n;
				auto l = d - i - j - k;
				size_t idx = getControlPointIndices(i, j, k);
				auto fraction = static_cast<float>(factorial(d))
				                / static_cast<float>(factorial(i) * factorial(j) * factorial(k)
				                                     * factorial(l));
				auto weight = static_cast<float>(fraction * glm::pow(u, i) * glm::pow(v, j)
				                                 * glm::pow(w, k) * glm::pow(1.0 - u - v - w, l));
				sum += controlPoints[idx] * weight;
			}
		}
	}
	// std::cout << "---------------\n";
	return sum;
}

inline glm::vec3
partialDerivativeBezierSurfaceU(const glm::vec3 controlPoints[16], int n, int m, double u, double v)
{
	auto sum = glm::vec3();
	for (int i = 0; i <= n - 1; i++)
	{
		for (int j = 0; j <= m; j++)
		{
			size_t p_a = convert2Dto1D_index(n + 1, i + 1, j);
			size_t p_b = convert2Dto1D_index(n + 1, i, j);
			sum += (controlPoints[p_a] - controlPoints[p_b])
			       * BernsteinPolynomial<float>(i, n - 1, u) * BernsteinPolynomial<float>(j, m, v);
		}
	};

	return static_cast<float>(m) * sum;
}

inline vec3
partialDerivativeBezierSurfaceV(const glm::vec3 controlPoints[16], int n, int m, double u, double v)
{
	auto sum = glm::vec3();
	for (int i = 0; i <= n; i++)
	{
		for (int j = 0; j <= m - 1; j++)
		{
			size_t p_a = convert2Dto1D_index(n + 1, i, j + 1);
			size_t p_b = convert2Dto1D_index(n + 1, i, j);
			sum += (controlPoints[p_a] - controlPoints[p_b]) * BernsteinPolynomial<float>(i, n, u)
			       * BernsteinPolynomial<float>(j, m - 1, v);
		}
	};

	return static_cast<float>(n) * sum;
}

inline vec2
projectPoint(const glm::vec3 point, const glm::vec3 origin, const glm::vec3 n1, const glm::vec3 n2)
{
	return glm::vec2(glm::dot(point, n1) - glm::dot(n1, origin),
	                 glm::dot(point, n2) - glm::dot(n2, origin));
}

inline glm::mat2x2 jacobian(const glm::vec3 controlPoints[16],
                            const int n,
                            const int m,
                            const glm::vec3 n1,
                            const glm::vec3 n2,
                            const double u,
                            const double v)
{
	auto partialDerivtiveU = partialDerivativeBezierSurfaceU(controlPoints, n, m, u, v);
	auto partialDerivativeV = partialDerivativeBezierSurfaceV(controlPoints, n, m, u, v);
	return glm::mat2x2{
	    glm::dot(n1, partialDerivtiveU),
	    glm::dot(n1, partialDerivativeV),
	    glm::dot(n2, partialDerivtiveU),
	    glm::dot(n2, partialDerivativeV),
	};
}

inline bool inverseJacobian(const glm::mat2x2& J, glm::mat2x2& inverse)
{
	float d = glm::determinant(J);
	if (glm::abs(d) < 1e-8)
	{
		return false;
	}

	auto mat = 1.0f / d
	           * glm::mat2x2{
	               J[1][1],
	               -J[0][1],
	               -J[1][0],
	               J[0][0],
	           };
	inverse = mat;
	return true;
}

inline glm::vec2 f(const glm::vec3 controlPoints[16],
                   const glm::vec3 origin,
                   const int n,
                   const int m,
                   const glm::vec3 n1,
                   const glm::vec3 n2,
                   const double u,
                   const double v)
{
	auto surfacePoint = bezierSurfacePoint(controlPoints, n, m, u, v);
	auto d1 = glm::dot(-n1, origin);
	auto d2 = glm::dot(-n2, origin);
	std::cout << "d1: " << d1 << " d2: " << d2 << std::endl;
	return glm::vec2{
	    glm::dot(n1, surfacePoint) + d1,
	    glm::dot(n2, surfacePoint) + d2,
	};
}

inline glm::vec2 fSide(const std::array<glm::vec3, 10> controlPoints,
                       HSideFunc hSideFunc,
                       const glm::vec3 origin,
                       const glm::vec3 n1,
                       const glm::vec3 n2,
                       const double v,
                       const double w)
{
	auto surfacePoint = hSideFunc(controlPoints, 2, v, w);
	auto d1 = glm::dot(-n1, origin);
	auto d2 = glm::dot(-n2, origin);
	return glm::vec2{
	    glm::dot(n1, surfacePoint) + d1,
	    glm::dot(n2, surfacePoint) + d2,
	};
}

inline glm::vec3 partialDerivativeBezierSurfaceV2(const std::array<glm::vec3, 10> controlPoints,
                                                  double u,
                                                  double v,
                                                  double w)
{
	auto sum = glm::vec3();
	for (int i = 0; i <= 2 - 1; i++)
	{
		for (int j = 0; j <= 2 - 1; j++)
		{
			for (int k = 0; k <= 2 - 1; k++)
			{
				if (i + j + k <= 2 - 1)
				{
					size_t p_a = getControlPointIndices(i, j + 1, k);
					size_t p_b = getControlPointIndices(i, j, k);
					sum += (controlPoints[p_a] - controlPoints[p_b])
					       * BernsteinPolynomial<float>(i, 2, u)
					       * BernsteinPolynomial<float>(j, 2, v)
					       * BernsteinPolynomial<float>(k, 2, w);
				}
			}
		}
	}

	return static_cast<float>(2) * sum;
}

inline glm::mat2x2 jacobian2(const std::array<glm::vec3, 10> controlPoints,
                             PartialFunc partial1,
                             PartialFunc partial2,
                             const glm::vec3 n1,
                             const glm::vec3 n2,
                             const double v,
                             const double w)
{
	auto partialDerivative1 = partial1(controlPoints, v, w);
	auto partialDerivative2 = partial2(controlPoints, v, w);
	return glm::mat2x2{
	    glm::dot(n1, partialDerivative1),
	    glm::dot(n1, partialDerivative2),
	    glm::dot(n2, partialDerivative1),
	    glm::dot(n2, partialDerivative2),
	};
}

inline bool newtonsMethod2([[maybe_unused]] std::vector<Sphere>& spheres,
                           glm::vec3& intersectionPoint,
                           const HSideFunc& hSideFunc,
                           const PartialFunc& partial1,
                           const PartialFunc& partial2,
                           const glm::vec2 initialGuess,
                           const glm::vec3 origin,
                           const std::array<glm::vec3, 10> controlPoints,
                           const glm::vec3 n1,
                           const glm::vec3 n2)
{
	// glm::vec2 initialGuess = glm::vec2(0.5, 0.2);
	{
		// auto point = bezierVolumePoint(controlPoints, 2,2, initialGuess.x, initialGuess.y);
		// std::cout << "Initial starting point on surface for UV: "
		//           << "(" << initialGuess.x << "," << initialGuess.y << ")"
		//           << " -> " << point.x << " " << point.y << " " << point.z << std::endl;
	}

	const int max_iterations = 5;
	glm::vec2 u[max_iterations + 1];

	const auto tolerance = glm::vec2(0.01f, 0.01f);

	u[0] = initialGuess;

	glm::vec2 previousError = glm::vec2(), error = glm::vec2(100000, 100000);

	for (int c = 0; c < max_iterations - 1; c++)
	{
		{
			auto cPoint = hSideFunc(controlPoints, 2, u[c].x, u[c].y);
			if (glm::abs(cPoint.x) < 100 && glm::abs(cPoint.y) < 100 && glm::abs(cPoint.z) < 100)
			{
				std::cout << "Current point on surface for UV: "
				          << "(" << u[c].x << "," << u[c].y << ")"
				          << " -> " << cPoint.x << " " << cPoint.y << " " << cPoint.z << std::endl;
				spheres.emplace_back(
				    cPoint,
				    (static_cast<float>(max_iterations - c) / static_cast<float>(max_iterations))
				        * 0.03f,
				    static_cast<int>(ColorIdx::t_white));
			}
			else
			{
				// NOTE: This happens when the u[c].x and u[c].y are way off
				std::cout << "WARNING!!!!!! Current point on surface for UV is GIGANTIC: "
				          << "(" << u[c].x << "," << u[c].y << ")"
				          << " -> " << cPoint.x << " " << cPoint.y << " " << cPoint.z << std::endl;
			}
		}

		std::cout << "=================== Step: " << c << " ===================" << std::endl;
		auto point = hSideFunc(controlPoints, 2, u[c].x, u[c].y);
		std::cout << "Testing point on surface for UV for c=" << c << ":"
		          << "(" << u[c].x << "," << u[c].y << ")"
		          << " -> " << point.x << " " << point.y << " " << point.z << std::endl;

		auto j = jacobian2(controlPoints, partial1, partial2, n1, n2, u[c].x, u[c].y);
		glm::mat2x2 inv_j{};
		// when the determinant is 0, we cannot invert the matrix
		if (inverseJacobian(j, inv_j) == false)
		{
			std::cout << "Abort! Jacobian not invertable" << std::endl;
			std::cout << "=================== Done  ===================\n" << std::endl;
			return false;
		}

		auto f_value = fSide(controlPoints, hSideFunc, origin, n1, n2, u[c].x, u[c].y);
		std::cout << "f value: " << f_value.x << " " << f_value.y << std::endl;
		u[c + 1] = u[c] - (inv_j * f_value);

		previousError = error;
		error = glm::abs(f_value);

		// std::cout << "differenceInUV: " << differenceInUV.x << " " << differenceInUV.y <<
		// std::endl;
		// std::cout << "Jacobian: " << j[0][0] << " " << j[0][1] << " " << j[1][0] << " " <<
		// j[1][1]
		//           << std::endl;
		// std::cout << "Inverse Jacobian: " << inv_j[0][0] << " " << inv_j[0][1] << " " <<
		// inv_j[1][0]
		//           << " " << inv_j[1][1] << std::endl;
		// std::cout << "Determinant of J: " << glm::determinant(j) << std::endl;
		std::cout << "Error: " << error.x << " " << error.y << std::endl;
		std::cout << "Calculated u value: " << u[c + 1].x << " " << u[c + 1].y << std::endl;

		if (error.x < tolerance.x && error.t < tolerance.y)
		{
			intersectionPoint = hSideFunc(controlPoints, 2, u[c].x, u[c].y);
			std::cout << "Intersection point: " << intersectionPoint.x << " " << intersectionPoint.y
			          << " " << intersectionPoint.z << std::endl;
			std::cout << "=================== Done  ===================\n" << std::endl;
			return true;
		}

		if (error.x > previousError.x && error.y > previousError.y)
		{
			std::cout << "Error Increased! No intersection point found" << std::endl;
			intersectionPoint = glm::vec3();
			std::cout << "=================== Done  ===================\n" << std::endl;
			return false;
		}
	}
	intersectionPoint = glm::vec3();
	std::cout << "No intersection point found" << std::endl;
	std::cout << "=================== Done  ===================\n" << std::endl;
	return false;
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

inline void visualizePlane(std::vector<Sphere>& spheres,
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
			spheres.emplace_back(pointOnPlane + (x - sizeX / 2.0f) * b2 + (z - sizeZ / 2.0f) * b3,
			                     0.005f,
			                     static_cast<int>(ColorIdx::t_pink));
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
	// 		spheres.emplace_back(bezierSurfacePoint(surface.controlPoints, 3, 3, u, v), 0.05f, 1);
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

inline glm::vec3
H1(const std::array<glm::vec3, 10> controlPoints, int n, const double v, const double w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, n, 0)
				       * BernsteinPolynomial<float>(j, n, v) * BernsteinPolynomial<float>(k, n, w);
			}
		}
	}
	return sum;
}

inline glm::vec3
partialH1v2(const std::array<glm::vec3, 10> controlPoints, const double v, const double w)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = BernsteinPolynomial<float>(i, n, 0);
				float Bj = static_cast<float>(n) * BernsteinPolynomial<float>(j - 1, n - 1, v)
				           - BernsteinPolynomial<float>(j, n - 1, v);
				float Bk = BernsteinPolynomial<float>(k, n, w);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
partialH1w2(const std::array<glm::vec3, 10> controlPoints, const double v, const double w)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = BernsteinPolynomial<float>(i, n, 0);
				float Bj = BernsteinPolynomial<float>(j, n, v);
				float Bk = static_cast<float>(n)
				           * (BernsteinPolynomial<float>(k - 1, n - 1, w)
				              - BernsteinPolynomial<float>(k, n - 1, w));
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
H2(const std::array<glm::vec3, 10> controlPoints, int n, const double u, const double w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, n, u)
				       * BernsteinPolynomial<float>(j, n, 0) * BernsteinPolynomial<float>(k, n, w);
			}
		}
	}
	return sum;
}

inline glm::vec3
partialH2u2(const std::array<glm::vec3, 10> controlPoints, const double u, const double w)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = static_cast<float>(n) * BernsteinPolynomial<float>(i - 1, n - 1, u)
				           - BernsteinPolynomial<float>(i, n - 1, u);
				float Bj = BernsteinPolynomial<float>(j, n, 0);
				float Bk = BernsteinPolynomial<float>(k, n, w);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
partialH2w2(const std::array<glm::vec3, 10> controlPoints, const double u, const double w)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = BernsteinPolynomial<float>(i, n, u);
				float Bj = BernsteinPolynomial<float>(j, n, 0);
				float Bk = static_cast<float>(n)
				           * (BernsteinPolynomial<float>(k - 1, n - 1, w)
				              - BernsteinPolynomial<float>(k, n - 1, w));
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
H3(const std::array<glm::vec3, 10> controlPoints, int n, const double u, const double v)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, n, u)
				       * BernsteinPolynomial<float>(j, n, v) * BernsteinPolynomial<float>(k, n, 0);
			}
		}
	}
	return sum;
}

inline glm::vec3
partialH3u2(const std::array<glm::vec3, 10> controlPoints, const double u, const double v)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = static_cast<float>(n) * BernsteinPolynomial<float>(i - 1, n - 1, u)
				           - BernsteinPolynomial<float>(i, n - 1, u);
				float Bj = BernsteinPolynomial<float>(j, n, v);
				float Bk = BernsteinPolynomial<float>(k, n, 0);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
partialH3v2(const std::array<glm::vec3, 10> controlPoints, const double u, const double v)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = BernsteinPolynomial<float>(i, n, u);
				float Bj = static_cast<float>(n)
				           * (BernsteinPolynomial<float>(j - 1, n - 1, v)
				              - BernsteinPolynomial<float>(j, n - 1, v));
				float Bk = BernsteinPolynomial<float>(k, n, 0);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
H4(const std::array<glm::vec3, 10> controlPoints, int n, const double r, const double t)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				if (r + t <= 1)
				{
					size_t idx = getControlPointIndices(i, j, k);
					sum += controlPoints[idx] * BernsteinPolynomial<float>(i, n, r)
					       * BernsteinPolynomial<float>(j, n, t)
					       * BernsteinPolynomial<float>(k, n, 1.0 - r - t);
				}
			}
		}
	}
	return sum;
}

inline glm::vec3
partialH4r2(const std::array<glm::vec3, 10> controlPoints, const double r, const double t)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = static_cast<float>(n) * BernsteinPolynomial<float>(i - 1, n - 1, r)
				           - BernsteinPolynomial<float>(i, n - 1, r);
				float Bj = BernsteinPolynomial<float>(j, n, t);
				float Bk = BernsteinPolynomial<float>(k, n, 1.0 - r - t);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3
partialH4t2(const std::array<glm::vec3, 10> controlPoints, const double r, const double t)
{
	int n = 2;
	auto sum = glm::vec3();
	for (int k = 0; k <= n; k++)
	{
		for (int j = 0; j <= n - k; j++)
		{
			for (int i = 0; i <= n - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);

				float Bi = BernsteinPolynomial<float>(i, n, r);
				float Bj = static_cast<float>(n) * BernsteinPolynomial<float>(j - 1, n - 1, t)
				           - BernsteinPolynomial<float>(j, n - 1, t);
				float Bk = BernsteinPolynomial<float>(k, n, 1.0 - r - t);
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline void visualizeTetrahedron2([[maybe_unused]] RaytracingScene& raytracingScene,
                                  [[maybe_unused]] const Tetrahedron2& tetrahedron)
{
	// Visualize control points
	for (auto& point : tetrahedron.controlPoints)
	{
		raytracingScene.addSphere(point, 0.02f, ColorIdx::t_black);
	}
	// static auto min = 100000000.0f;
	// static auto minParameter = glm::vec3(0);
	// Visualize volume
	double stepSize = 0.1;
	for (double u = 0; u <= 1; u += stepSize)
	{
		for (double v = 0; v <= 1; v += stepSize)
		{
			for (double w = 0; w <= 1; w += stepSize)
			{
				if (u + v + w <= 1)
				{
					// auto rayO = glm::vec3(0.5f, 0.8f, 0.6f);
					auto p
					    = bezierVolumePoint(std::to_array(tetrahedron.controlPoints), 2, u, v, w);

					// auto isEdge = glm::abs(u - 1) <= 1e-5 || glm::abs(v - 1) <= 1e-5
					//               || glm::abs(w - 1) <= 1e-5 || glm::abs(u) <= 1e-5
					//               || glm::abs(v) <= 1e-5 || glm::abs(w) <= 1e-5;
					auto isFace1 = u == 0 && v + w <= 1;
					if (isFace1)
					{
						raytracingScene.addSphere(p, 0.01f, ColorIdx::t_red);
					}

					auto isFace2 = v == 0 && u + w <= 1;
					if (isFace2)
					{
						raytracingScene.addSphere(p, 0.01f, ColorIdx::t_purple);
					}

					auto isFace3 = w == 0 && u + v <= 1;
					if (isFace3)
					{
						raytracingScene.addSphere(p, 0.01f, ColorIdx::t_green);
					}

					auto isFace4 = glm::abs(u + v + w - 1) <= 1e-4;
					if (isFace4)
					{
						raytracingScene.addSphere(p, 0.01f, ColorIdx::t_white);
					}

					if (!isFace1 && !isFace2 && !isFace3 && !isFace4)
					{
						// raytracingScene.addSphere(p, 0.01f, ColorIdx::t_black);
					}
				}
			}
		}
	}

	// std::cout << "Min with parameter (u,v,w) is:" << std::fixed << min << " (" <<
	// minParameter.x
	//           << " " << minParameter.y << " " << minParameter.z << ")" << std::endl;

	// auto pO = bezierVolumePoint(
	//     std::to_array(tetrahedron.controlPoints), minParameter.x, minParameter.y,
	//     minParameter.z);
	// spheres.emplace_back(pO, 0.01f, static_cast<int>(ColorIdx::t_red));
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

} // namespace rt
} // namespace ltracer
