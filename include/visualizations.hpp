#pragma once

#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include <ostream>

#include "common_types.h"

namespace ltracer
{

// TODO: maybe change the implementation of this
inline double nChooseK(int n, int k)
{
	if (k == 0)
	{
		return 1;
	}
	return n * nChooseK(n - 1, k - 1) / k;
}

inline double realPolynomial(int i, int n, double x)
{
	return nChooseK(n, i) * pow(x, i) * pow(1 - x, n - i);
}

inline size_t convert2Dto1D_index(int columns, int i, int j)
{
	return static_cast<size_t>(i * columns + j);
}

inline vec3 bezierSurfacePoint(const glm::vec3 controlPoints[16], int n, int m, double u, double v)
{
	auto sum = glm::vec3();
	for (int i = 0; i <= n; i++)
	{
		for (int j = 0; j <= m; j++)
		{
			// TODO: do we really want/need to use double precision here?
			size_t idx = convert2Dto1D_index(n + 1, i, j);
			sum += controlPoints[idx]
			       * static_cast<float>(realPolynomial(i, n, u) * realPolynomial(j, m, v));
		}
	}
	return sum;
}

inline vec3
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
			       * static_cast<float>(realPolynomial(i, n - 1, u) * realPolynomial(j, m, v));
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
			sum += (controlPoints[p_a] - controlPoints[p_b])
			       * static_cast<float>(realPolynomial(i, n, u) * realPolynomial(j, m - 1, v));
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

inline glm::mat2x2 inverseJacobian(const glm::mat2x2& J)
{
	float d = glm::determinant(J);
	assert(glm::abs(d) > 0.00001 && "Error: determinant is near zero");

	auto mat = 1.0f / d
	           * glm::mat2x2{
	               J[1][1],
	               -J[0][1],
	               -J[1][0],
	               J[0][0],
	           };
	return mat;
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

inline bool newtonsMethod(std::vector<Sphere>& spheres,
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
		std::cout << "Initial starting point on surface for UV: " << "(" << initialGuess.x << ","
		          << initialGuess.y << ")"
		          << " -> " << point.x << " " << point.y << " " << point.z << std::endl;
	}

	const int max_iterations = 20;
	glm::vec2 u[max_iterations + 1];

	const auto tolerance = glm::vec2(0.2f, 0.2f);

	u[0] = initialGuess;

	glm::vec2 previousError = glm::vec2(), error = glm::vec2(100000, 100000);

	for (int c = 0; c < max_iterations; c++)
	{
		{
			auto cPoint = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
			spheres.emplace_back(cPoint, 0.3, 7);
			std::cout << "Current point on surface for UV: " << "(" << u[c].x << "," << u[c].y
			          << ")"
			          << " -> " << cPoint.x << " " << cPoint.y << " " << cPoint.z << std::endl;
		}

		auto j = jacobian(controlPoints, n, m, n1, n2, u[c].x, u[c].y);
		auto inv_j = inverseJacobian(j);
		auto f_value = f(controlPoints, origin, n, m, n1, n2, u[c].x, u[c].y);

		auto differenceInUV = (inv_j * f_value);
		u[c + 1] = u[c] - differenceInUV;

		previousError = error;
		error = abs(f_value);

		std::cout << "=================== Step: " << c << " ===================" << std::endl;
		auto point = bezierSurfacePoint(controlPoints, n, m, u[c].x, u[c].y);
		std::cout << "Testing point on surface for UV: " << "(" << u[c].x << "," << u[c].y << ")"
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
                            const float length)
{
	float stepSize = 0.01f;
	for (float s = 0; s <= length; s += stepSize)
	{
		auto pos = startPos + direction * s;
		spheres.emplace_back(pos, 0.06f, 2);
	}
}

inline void visualizeBezierSurface(std::vector<Sphere>& spheres,
                                   RectangularBezierSurface2x2 surface)
{
	double stepSize = 0.01;
	for (double u = 0; u <= 1; u += stepSize)
	{
		for (double v = 0; v <= 1; v += stepSize)
		{
			spheres.emplace_back(bezierSurfacePoint(surface.controlPoints, 3, 3, u, v), 0.05, 1);
		}
	}

	vec3 rayOrigin = glm::vec3(8, 1, 0);
	vec3 rayDirection = glm::vec3(0, 0, 1);

	for (float v = 0; v <= 15; v += static_cast<float>(stepSize))
	{
		spheres.emplace_back(rayOrigin + rayDirection * v, 0.01, 4);
	}

	auto n1 = glm::vec3(1, 0, 0);
	auto n2 = glm::vec3(0, 1, 0);

	// for (int i = 0; i < 16; i++)
	//{
	//	auto projectedPoint = projectPoint(surface.controlPoints[i], rayOrigin, n1, n2);
	//	spheres.emplace_back(glm::vec3(projectedPoint.x, projectedPoint.y, 0), 0.4, 5);
	// }

	glm::vec3 intersectionPoint;
	glm::vec2 initialGuess = glm::vec2(1, 1);

	// glm::vec2 initialGuess = projectPoint(surface.controlPoints[10], rayOrigin, n1, n2);
	std::cout << "Initial guess: " << initialGuess.x << " " << initialGuess.y << std::endl;
	if (newtonsMethod(spheres,
	                  intersectionPoint,
	                  initialGuess,
	                  rayOrigin,
	                  surface.controlPoints,
	                  3,
	                  3,
	                  n1,
	                  n2))
	{
		std::cout << "HIT!" << std::endl;
		spheres.emplace_back(intersectionPoint, 1, 6);
	}

	for (int i = 0; i < 16; i++)
	{
		auto controlPointPos = surface.controlPoints[i];
		spheres.emplace_back(controlPointPos, 0.1f, 2);
	}

	// Visualize Normals in corners
	{
		int i = 0, j = 0;
		auto dirA = surface.controlPoints[convert2Dto1D_index(4, i + 1, j)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto dirB = surface.controlPoints[convert2Dto1D_index(4, i, j + 1)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto normal = glm::cross(dirA, dirB);

		auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
		visualizeVector(spheres, controlPointPos, normal, 0.1f);
	}
	{
		int i = 3, j = 0;
		auto dirA = surface.controlPoints[convert2Dto1D_index(4, i, j + 1)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto dirB = surface.controlPoints[convert2Dto1D_index(4, i - 1, j)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto normal = glm::cross(dirA, dirB);

		auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
		visualizeVector(spheres, controlPointPos, normal, 0.1f);
	}
	{
		int i = 0, j = 3;
		auto dirA = surface.controlPoints[convert2Dto1D_index(4, i, j - 1)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto dirB = surface.controlPoints[convert2Dto1D_index(4, i + 1, j)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto normal = glm::cross(dirA, dirB);

		auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
		visualizeVector(spheres, controlPointPos, normal, 0.1f);
	}
	{
		int i = 3, j = 3;
		auto dirA = surface.controlPoints[convert2Dto1D_index(4, i - 1, j)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto dirB = surface.controlPoints[convert2Dto1D_index(4, i, j - 1)]
		            - surface.controlPoints[convert2Dto1D_index(4, i, j)];
		auto normal = glm::cross(dirA, dirB);

		auto controlPointPos = surface.controlPoints[convert2Dto1D_index(4, i, j)];
		visualizeVector(spheres, controlPointPos, normal, 0.1f);
	}
}

} // namespace ltracer
