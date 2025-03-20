#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float3.hpp>
#include <glm/exponential.hpp>

inline constexpr int getControlPointIndicesBezierTriangle2(const int i, const int j, const int k)
{
	if (i == 0 && j == 0 && k == 2) return 0;
	if (i == 2 && j == 0 && k == 0) return 1;
	if (i == 0 && j == 2 && k == 0) return 2;
	if (i == 1 && j == 0 && k == 1) return 3;
	if (i == 1 && j == 1 && k == 0) return 4;
	if (i == 0 && j == 1 && k == 1) return 5;
	return 0;
}

inline constexpr int factorial(int n)
{
	// NOTE : we only work with small numbers, so we can (hopefully) safely use int instead of
	// unsigned ints
	int ret = 1;
	for (int i = 1; i <= n; ++i)
		ret *= i;
	return static_cast<int>(ret);
}

inline glm::vec3
bezierTriangleSurfacePoint(const glm::vec3 controlPoints[6], int n, double u, double v, double w)

{
	auto sum = glm::vec3();
	for (int i = 0; i <= n; i++)
	{
		for (int j = 0; j <= n - i; j++)
		{
			for (int k = 0; k <= n - i - j; k++)
			{
				if (i + j + k == n)
				{
					int idx = getControlPointIndicesBezierTriangle2(i, j, k);
					sum += controlPoints[idx]
					       * (static_cast<float>(factorial(n))
					          / static_cast<float>(factorial(i) * factorial(j) * factorial(k)))
					       * static_cast<float>(glm::pow(u, i) * glm::pow(v, j) * glm::pow(w, k));
				}
			}
		}
	}
	return sum;
}
