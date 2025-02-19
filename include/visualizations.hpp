#pragma once

#include <functional>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <tuple>

#include "common_types.h"

namespace ltracer
{

// TODO: maybe convert this file into some kind of Scene class that holds all the objects
// and does the updating

// using FunctionToFindRootFor
//     = std::function<glm::vec3(const float t, const float u, const float v, const float w)>;

// using PartialFunc = std::function<glm::vec3(
//     const std::array<glm::vec3, 10> controlPoints, const double u, const double v, const double
//     w)>;
// using PartialFuncQuotient = std::function<float(PartialFunc, float x)>;
// using JacobianMatrix = std::function<std::array<float, 16>(
//     const std::array<PartialFuncQuotient, 4> Partials, const std::array<float, 4> X)>;

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
inline std::tuple<int, int, int> getControlPointIndices(size_t idx)
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

inline glm::vec3
bezierVolumePoint(const std::array<glm::vec3, 10> controlPoints, double u, double v, double w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= 2; k++)
	{
		for (int j = 0; j <= 2 - k; j++)
		{
			for (int i = 0; i <= 2 - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, 2, u)
				       * BernsteinPolynomial<float>(j, 2, v) * BernsteinPolynomial<float>(k, 2, w);
			}
		}
	}
	return sum;
}

inline glm::vec3 tetrahedronSide1PartialDerivative2V(const std::array<glm::vec3, 10> controlPoints,
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
					       * BernsteinPolynomial<float>(i, 1, 0)
					       * BernsteinPolynomial<float>(j, 1, v)
					       * BernsteinPolynomial<float>(k, 1, w);
				}
			}
		}
	}

	return static_cast<float>(2) * sum;
}

inline glm::vec3 tetrahedronSide1PartialDerivative2W(const std::array<glm::vec3, 10> controlPoints,
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
					size_t p_a = getControlPointIndices(i, j, k + 1);
					size_t p_b = getControlPointIndices(i, j, k);
					sum += (controlPoints[p_a] - controlPoints[p_b])
					       * BernsteinPolynomial<float>(i, 2, 0)
					       * BernsteinPolynomial<float>(j, 2, v)
					       * BernsteinPolynomial<float>(k, 2, w);
				}
			}
		}
	}

	return static_cast<float>(2) * sum;
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

inline glm::vec3
H1(const std::array<glm::vec3, 10> controlPoints, int n, const double v, const double w);

inline glm::vec2 fSide1(const std::array<glm::vec3, 10> controlPoints,
                        const glm::vec3 origin,
                        const glm::vec3 n1,
                        const glm::vec3 n2,
                        const double v,
                        const double w)
{
	auto surfacePoint = H1(controlPoints, 2, v, w);
	auto d1 = glm::dot(-n1, origin);
	auto d2 = glm::dot(-n2, origin);
	// std::cout << "d1: " << d1 << " d2: " << d2 << std::endl;
	return glm::vec2{
	    glm::dot(n1, surfacePoint) + d1,
	    glm::dot(n2, surfacePoint) + d2,
	};
}

inline glm::vec3 partialDerivativeBezierSurfaceU2(const std::array<glm::vec3, 10> controlPoints,
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
					size_t p_a = getControlPointIndices(i + 1, j, k);
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

inline glm::vec3 partialDerivativeBezierSurfaceW2(const std::array<glm::vec3, 10> controlPoints,
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
					size_t p_a = getControlPointIndices(i, j, k + 1);
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

inline glm::vec3
partialH1v2(const std::array<glm::vec3, 10> controlPoints, const double v, const double w);
inline glm::vec3
partialH1w2(const std::array<glm::vec3, 10> controlPoints, const double v, const double w);

inline glm::mat2x2 jacobian2Side1(const std::array<glm::vec3, 10> controlPoints,
                                  const glm::vec3 n1,
                                  const glm::vec3 n2,
                                  const double v,
                                  const double w)
{
	auto partialDerivativeV = partialH1v2(controlPoints, v, w);
	auto partialDerivativeW = partialH1w2(controlPoints, v, w);
	return glm::mat2x2{
	    glm::dot(n1, partialDerivativeV),
	    glm::dot(n1, partialDerivativeW),
	    glm::dot(n2, partialDerivativeV),
	    glm::dot(n2, partialDerivativeW),
	};
}

inline bool newtonsMethod2([[maybe_unused]] std::vector<Sphere>& spheres,
                           glm::vec3& intersectionPoint,
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

	const int max_iterations = 10;
	glm::vec2 u[max_iterations + 1];

	const auto tolerance = glm::vec2(0.001f, 0.001f);

	u[0] = initialGuess;

	glm::vec2 previousError = glm::vec2(), error = glm::vec2(100000, 100000);

	for (int c = 0; c < max_iterations; c++)
	{
		{
			auto cPoint = H1(controlPoints, 2, u[c].x, u[c].y);
			spheres.emplace_back(
			    cPoint,
			    (static_cast<float>(max_iterations - c) / static_cast<float>(max_iterations))
			        * 0.03f,
			    static_cast<int>(ColorIdx::t_white));
			std::cout << "Current point on surface for UV: "
			          << "(" << u[c].x << "," << u[c].y << ")"
			          << " -> " << cPoint.x << " " << cPoint.y << " " << cPoint.z << std::endl;
		}

		auto j = jacobian2Side1(controlPoints, n1, n2, u[c].x, u[c].y);
		glm::mat2x2 inv_j{};

		if (inverseJacobian(j, inv_j) == false)
		{
			return false;
		}

		auto f_value = fSide1(controlPoints, origin, n1, n2, u[c].x, u[c].y);

		u[c + 1] = u[c] - (inv_j * f_value);

		previousError = error;
		error = abs(f_value);

		std::cout << "=================== Step: " << c << " ===================" << std::endl;
		auto point = H1(controlPoints, 2, u[c].x, u[c].y);
		std::cout << "Testing point on surface for UV: "
		          << "(" << u[c].x << "," << u[c].y << ")"
		          << " -> " << point.x << " " << point.y << " " << point.z << std::endl;

		std::cout << "f value: " << f_value.x << " " << f_value.y << std::endl;
		// std::cout << "differenceInUV: " << differenceInUV.x << " " << differenceInUV.y <<
		// std::endl;
		std::cout << "Jacobian: " << j[0][0] << " " << j[0][1] << " " << j[1][0] << " " << j[1][1]
		          << std::endl;
		std::cout << "Inverse Jacobian: " << inv_j[0][0] << " " << inv_j[0][1] << " " << inv_j[1][0]
		          << " " << inv_j[1][1] << std::endl;
		std::cout << "Determinant of J: " << glm::determinant(j) << std::endl;
		std::cout << "u value: " << u[c + 1].x << " " << u[c + 1].y << std::endl;
		std::cout << "Error: " << error.x << " " << error.y << std::endl;

		if (error.x < tolerance.x && error.t < tolerance.y)
		{
			intersectionPoint = H1(controlPoints, 2, u[c].x, u[c].y);
			std::cout << "Intersection point: " << intersectionPoint.x << " " << intersectionPoint.y
			          << " " << intersectionPoint.z << std::endl;
			return true;
		}

		// if (length(error) > length(previousError))
		// {
		// 	intersectionPoint = glm::vec3();
		// 	std::cout << "No intersection point found" << std::endl;
		// 	return false;
		// }
	}
	intersectionPoint = glm::vec3();
	std::cout << "No intersection point found" << std::endl;
	return false;

	// return false;
}

inline bool newtonsMethod3(std::vector<Sphere>& spheres,
                           glm::vec3& intersectionPoint,
                           const glm::vec2 initialGuess,
                           const glm::vec3 origin,
                           const glm::vec3 controlPoints[16],
                           int n,
                           int m,
                           const glm::vec3 n1,
                           const glm::vec3 n2)
{
	// glm::vec2 initialGuess = glm::vec2(0.5, 0.2);
	{
		auto point = bezierSurfacePoint(controlPoints, n, m, initialGuess.x, initialGuess.y);
		std::cout << "Initial starting point on surface for UV: "
		          << "(" << initialGuess.x << "," << initialGuess.y << ")"
		          << " -> " << point.x << " " << point.y << " " << point.z << std::endl;
	}

	const int max_iterations = 6;
	glm::vec2 u[max_iterations + 1];

	const auto tolerance = glm::vec2(0.001f, 0.001f);

	u[0] = initialGuess;

	glm::vec2 previousError = glm::vec2(), error = glm::vec2(100000, 100000);

	for (int c = 0; c < max_iterations; c++)
	{
		{
			auto cPoint = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
			spheres.emplace_back(cPoint, 0.1f, 7);
			std::cout << "Current point on surface for UV: "
			          << "(" << u[c].x << "," << u[c].y << ")"
			          << " -> " << cPoint.x << " " << cPoint.y << " " << cPoint.z << std::endl;
		}

		auto j = jacobian(controlPoints, n, m, n1, n2, u[c].x, u[c].y);
		glm::mat2x2 inv_j{};
		if (inverseJacobian(j, inv_j) == false)
		{
			return false;
		}
		auto f_value = f(controlPoints, origin, n, m, n1, n2, u[c].x, u[c].y);

		auto differenceInUV = (inv_j * f_value);
		u[c + 1] = u[c] - differenceInUV;

		previousError = error;
		error = abs(f_value);

		std::cout << "=================== Step: " << c << " ===================" << std::endl;
		auto point = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
		std::cout << "Testing point on surface for UV: "
		          << "(" << u[c].x << "," << u[c].y << ")"
		          << " -> " << point.x << " " << point.y << " " << point.z << std::endl;

		std::cout << "f value: " << f_value.x << " " << f_value.y << std::endl;
		std::cout << "differenceInUV: " << differenceInUV.x << " " << differenceInUV.y << std::endl;
		std::cout << "Jacobian: " << j[0][0] << " " << j[0][1] << " " << j[1][0] << " " << j[1][1]
		          << std::endl;
		std::cout << "Inverse Jacobian: " << inv_j[0][0] << " " << inv_j[0][1] << " " << inv_j[1][0]
		          << " " << inv_j[1][1] << std::endl;
		std::cout << "Determinant of J: " << glm::determinant(j) << std::endl;
		std::cout << "u value: " << u[c + 1].x << " " << u[c + 1].y << std::endl;
		std::cout << "Error: " << error.x << " " << error.y << std::endl;

		if (error.x < tolerance.x && error.t < tolerance.y)
		{
			intersectionPoint = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
			std::cout << "Intersection point: " << intersectionPoint.x << " " << intersectionPoint.y
			          << " " << intersectionPoint.z << std::endl;
			return true;
		}

		if (length(error) > length(previousError))
		{
			intersectionPoint = glm::vec3();
			std::cout << "No intersection point found" << std::endl;
			return false;
		}
	}

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

// inline float partialDericativeBezierVolume3T(const std::array<glm::vec3, 10>
// controlPoints,
//                                              double u,
//                                              double v,
//                                              double w)
// {
// 	auto sum = glm::vec3();
// 	for (int i = 0; i <= 3; i++)
// 	{
// 		for (int j = 0; j <= 3; j++)
// 		{
// 			for (int j = 0; j <= 3; j++)
// 			{
// 				size_t p_a = convert2Dto1D_index(n + 1, i + 1, j);
// 				size_t p_b = convert2Dto1D_index(n + 1, i, j);
// 				sum += (controlPoints[p_a] - controlPoints[p_b])
// 				       * static_cast<float>(realPolynomial(i, n - 1, u) * realPolynomial(j,
// m, v));
// 			}
// 		};

// 		// return static_cast<float>(m) * sum;

// 		return glm::dot()
// 	}
// }

// inline vec3
// partialDerivativeBezierVolumeV(const glm::vec3 controlPoints[16], int n, int m, double u,
// double v)
// {
// 	auto sum = glm::vec3();
// 	for (int i = 0; i <= n; i++)
// 	{
// 		for (int j = 0; j <= m - 1; j++)
// 		{
// 			size_t p_a = convert2Dto1D_index(n + 1, i, j + 1);
// 			size_t p_b = convert2Dto1D_index(n + 1, i, j);
// 			sum += (controlPoints[p_a] - controlPoints[p_b])
// 			       * static_cast<float>(realPolynomial(i, n, u) * realPolynomial(j, m - 1,
// v));
// 		}
// 	};

// 	return static_cast<float>(n) * sum;
// }

// inline vec3
// partialDerivativeBezierVolumeW(const glm::vec3 controlPoints[16], int n, int m, double u,
// double v)
// {
// 	auto sum = glm::vec3();
// 	for (int i = 0; i <= n; i++)
// 	{
// 		for (int j = 0; j <= m - 1; j++)
// 		{
// 			size_t p_a = convert2Dto1D_index(n + 1, i, j + 1);
// 			size_t p_b = convert2Dto1D_index(n + 1, i, j);
// 			sum += (controlPoints[p_a] - controlPoints[p_b])
// 			       * static_cast<float>(realPolynomial(i, n, u) * realPolynomial(j, m - 1,
// v));
// 		}
// 	};

// 	return static_cast<float>(n) * sum;
// }

// inline JacobianMatrix jacobian3(const std::array<PartialResult, 4> Partials,
//                                 const std::array<float, 4> X)
// {
// 	return std::array<PartialResult, 4>{
// 	    Partials[0](X[0]),
// 	    Partials[0](X[1]),
// 	    Partials[0](X[2]),
// 	    Partials[0](X[3]),

// 	    Partials[1](X[0]),
// 	    Partials[1](X[1]),
// 	    Partials[1](X[2]),
// 	    Partials[1](X[3]),

// 	    Partials[2](X[0]),
// 	    Partials[2](X[1]),
// 	    Partials[2](X[2]),
// 	    Partials[2](X[3]),

// 	    Partials[3](X[0]),
// 	    Partials[3](X[1]),
// 	    Partials[3](X[2]),
// 	    Partials[3](X[3]),
// 	};
// }

// inline glm::mat2x2 inverseJacobian3(const glm::mat3x3& J)
// {
// 	float d = glm::determinant(J);
// 	assert(glm::abs(d) > 0.00001 && "Error: determinant is near zero");

// 	auto mat = 1.0f / d
// 	           * glm::mat2x2{
// 	               J[1][1],
// 	               -J[0][1],
// 	               -J[1][0],
// 	               J[0][0],
// 	           };
// 	return mat;
// }

// inline bool newtonsMethod3(std::vector<Sphere>& spheres,
//                            glm::vec2& intersectionPoint,
//                            const glm::vec2 initialGuess,
//                            // std::function<glm::vec3(const std::array<glm::vec3, 10>, const
//                            float,
//                            // const float, const float)>
//                            //     bezierFunc,
//                            FunctionToFindRootFor func,
//                            JacobianMatrix jacobianFunc)
// {
// 	// glm::vec2 initialGuess = glm::vec2(0.5, 0.2);
// 	{
// 		// auto point = bezierFunc(controlPoints, initialGuess.x, initialGuess.y,
// 		// initialGuess.z); std::cout << "Initial starting point on surface for UVW: " << "(" <<
// 		// initialGuess.x << ','
// 		//           << initialGuess.y << ',' << initialGuess.y << ")"
// 		//           << " -> (" << point.x << " " << point.y << " " << point.z << ')' <<
// 		//           std::endl;
// 	}

// 	const int max_iterations = 5;
// 	glm::vec3 u[max_iterations + 1];

// 	const auto tolerance = glm::vec2(0.02f, 0.02f);

// 	u[0] = initialGuess;

// 	auto previousError = glm::vec3();
// 	auto error = glm::vec3(100000, 100000, 100000);

// 	for (int c = 0; c < max_iterations; c++)
// 	{
// 		// Visualize steps
// 		{
// 			// auto cPoint = bezierFunc(controlPoints, u[c].x, u[c].y, u[c].y);
// 			// spheres.emplace_back(cPoint, 0.3, 7);
// 			// std::cout << "Current point on surface for UVW: " << "(" << u[c].x << "," <<
// 			// u[c].y
// 			//           << "," << u[c].z << ")"
// 			//           << " -> (" << cPoint.x << " " << cPoint.y << " " << cPoint.z << ")"
// 			//           << std::endl;
// 		}

// 		auto j = jacobian3(controlPoints, n1, n2, u[c].x, u[c].y, u[c].z);

// 		auto deltaX = (inverseJacobian(j) * f(controlPoints, origin, n, m, n1, n2, u[c].x,
// u[c].y)); 		u[c + 1] = u[c] - deltaX;

// 		previousError = error;
// 		error = abs(f_value);

// 		auto point = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);

// 		if (error.x < tolerance.x && error.t < tolerance.y)
// 		{
// 			intersectionPoint = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
// 			std::cout << "Intersection point: " << intersectionPoint.x << " " <<
// intersectionPoint.y
// 			          << " " << intersectionPoint.z << std::endl;
// 			return true;
// 		}

// 		if (length(error) > length(previousError))
// 		{
// 			intersectionPoint = glm::vec3();
// 			std::cout << "No intersection point found" << std::endl;
// 			return false;
// 		}
// 	}

// 	return false;
// }

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
				float Bj = 2.0f * BernsteinPolynomial<float>(j - 1, n - 1, v)
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
				float Bk = 2.0f
				           * (BernsteinPolynomial<float>(k - 1, n - 1, w)
				              - BernsteinPolynomial<float>(k, n - 1, w));
				sum += controlPoints[idx] * Bi * Bj * Bk;
			}
		}
	}

	return sum;
}

inline glm::vec3 H2(const std::array<glm::vec3, 10> controlPoints, const double u, const double w)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= 2; k++)
	{
		for (int j = 0; j <= 2 - k; j++)
		{
			for (int i = 0; i <= 2 - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, 2, u)
				       * BernsteinPolynomial<float>(j, 2, 0) * BernsteinPolynomial<float>(k, 2, w);
			}
		}
	}
	return sum;
}

inline glm::vec3 H3(const std::array<glm::vec3, 10> controlPoints, const double u, const double v)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= 2; k++)
	{
		for (int j = 0; j <= 2 - k; j++)
		{
			for (int i = 0; i <= 2 - k - j; i++)
			{
				size_t idx = getControlPointIndices(i, j, k);
				sum += controlPoints[idx] * BernsteinPolynomial<float>(i, 2, u)
				       * BernsteinPolynomial<float>(j, 2, v) * BernsteinPolynomial<float>(k, 2, 0);
			}
		}
	}
	return sum;
}

inline glm::vec3 H4(const std::array<glm::vec3, 10> controlPoints, const double r, const double t)
{
	auto sum = glm::vec3();
	for (int k = 0; k <= 2; k++)
	{
		for (int j = 0; j <= 2 - k; j++)
		{
			for (int i = 0; i <= 2 - k - j; i++)
			{
				if (r + t <= 1)
				{
					size_t idx = getControlPointIndices(i, j, k);
					sum += controlPoints[idx] * BernsteinPolynomial<float>(i, 2, r)
					       * BernsteinPolynomial<float>(j, 2, t)
					       * BernsteinPolynomial<float>(k, 2, 1.0 - r - t);
				}
			}
		}
	}
	return sum;
}

inline void visualizeTetrahedron2(std::vector<Sphere>& spheres, const Tetrahedron2& tetrahedron)
{
	// Visualize control points
	for (auto& point : tetrahedron.controlPoints)
	{
		spheres.emplace_back(point, 0.05f, static_cast<int>(ColorIdx::t_green));
	}
	static auto min = 100000000.0f;
	static auto minParameter = glm::vec3(0);
	// Visualize volume
	double stepSize = 0.1;
	for (double u = 0; u <= 1; u += stepSize)
	{
		for (double v = 0; v <= 1; v += stepSize)
		{
			for (double w = 0; w <= 1; w += stepSize)
			{
				if (u + v + w <= 2)
				{
					// auto rayO = glm::vec3(0.5f, 0.8f, 0.6f);
					// auto p = bezierVolumePoint(std::to_array(tetrahedron.controlPoints), u, v,
					// w);

					// auto d = glm::distance(rayO, p);
					// std::cout << "Distance for parameter (u,v,w) ->: " << u << " " << v << " " <<
					// w
					//           << " is: " << std::fixed << std::setprecision(4) << d << std::endl;
					// if (d < min)
					// {
					// 	min = d;
					// 	minParameter = glm::vec3(u, v, w);
					// }

					const auto side1Point = H1(std::to_array(tetrahedron.controlPoints), 2, u, v);
					spheres.emplace_back(side1Point, 0.01f, static_cast<int>(ColorIdx::t_yellow));

					// auto isEdge = glm::abs(u - 1) <= 1e-8 || glm::abs(v - 1) <= 1e-8
					//               || glm::abs(w - 1) <= 1e-8 || glm::abs(u) <= 1e-8
					//               || glm::abs(v) <= 1e-8 || glm::abs(w) <= 1e-8;
					auto isFace1 = u == 0 && v + w <= 1;
					if (isFace1)
					{
						// spheres.emplace_back(p, 0.01f, static_cast<int>(ColorIdx::t_red));
					}

					auto isFace2 = v == 0 && u + w <= 1;
					if (isFace2)
					{
						// spheres.emplace_back(p, 0.01f, static_cast<int>(ColorIdx::t_purple));
					}

					auto isFace3 = w == 0 && u + v <= 1;
					if (isFace3)
					{
						// spheres.emplace_back(p, 0.01f, static_cast<int>(ColorIdx::t_green));
					}

					auto isFace4 = glm::abs(u + v + w - 1) <= 1e-8;
					if (isFace4)
					{
						// spheres.emplace_back(p, 0.01f, static_cast<int>(ColorIdx::t_white));
					}

					if (!isFace1 && !isFace2 && !isFace3 && !isFace4)
					{
						// spheres.emplace_back(p, 0.01f, static_cast<int>(ColorIdx::t_black));
					}
				}
			}
		}
	}

	std::cout << "Min with parameter (u,v,w) is:" << std::fixed << min << " (" << minParameter.x
	          << " " << minParameter.y << " " << minParameter.z << ")" << std::endl;

	auto pO = bezierVolumePoint(
	    std::to_array(tetrahedron.controlPoints), minParameter.x, minParameter.y, minParameter.z);
	spheres.emplace_back(pO, 0.01f, static_cast<int>(ColorIdx::t_red));
}

inline glm::vec2
transformPointToD(const glm::vec3 n1, const glm::vec3 n2, const glm::vec2 e, const glm::vec3 p)
{
	return glm::vec2(n1.x * p.x + n1.x * p.x + n1.x * p.x + e.x,
	                 n2.x * p.x + n2.x * p.x + n2.x * p.x + e.y);
}

inline bool intersectWithPlane(const glm::vec3 planeNormal,
                               const glm::vec3 planeOrigin,
                               const glm::vec3 rayOrigin,
                               const glm::vec3 rayDirection,
                               float& t)
{
	// debugPrintfEXT("Test intesection with Plane: RayOrigin: (%v3f) rayDirection: (%v3f)
	// PlaneNormal(%v3f) PlanePosition:(%v3f)",rayOrigin, rayDirection, planeNormal, planeOrigin);
	float denom = dot(planeNormal, rayDirection);
	if (denom > 1e-6)
	{
		glm::vec3 rayToPlanePoint = planeOrigin - rayOrigin;
		t = dot(rayToPlanePoint, planeNormal) / denom;
		return t >= 0;
	}
	return false;
}

} // namespace ltracer
