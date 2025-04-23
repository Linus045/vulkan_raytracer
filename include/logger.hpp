#pragma once

#include "common_types.h"
#include <cstdio>
#include <iostream>
#include <string>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

#ifdef NDEBUG
#define debug_print(fmt, ...)
#define debug_printFmt(fmt, ...)
#else
#define debug_printFmt(fmt, ...)                                                                   \
	do                                                                                             \
	{                                                                                              \
		std::fprintf(stderr, fmt, __VA_ARGS__);                                                    \
	} while (0)

#define debug_print(str)                                                                           \
	do                                                                                             \
	{                                                                                              \
		std::fprintf(stderr, str);                                                                 \
	} while (0)

#endif

inline void logVec3(const std::string& info, const glm::vec3& vector)
{
	std::printf("%s: (%.2f, %.2f, %.2f)\n", info.c_str(), vector.x, vector.y, vector.z);
}

inline void logVec2(const std::string& info, const glm::vec2& vector)
{
	std::printf("%s: (%.2f, %.2f)\n", info.c_str(), vector.x, vector.y);
}

inline void logMat4(const std::string& info, const glm::mat4& mat)
{
	std::cout << info << '\n';
	std::printf("%.02f, %.02f, %.02f, %.02f\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
	std::printf("%.02f, %.02f, %.02f, %.02f\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
	std::printf("%.02f, %.02f, %.02f, %.02f\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
	std::printf("%.02f, %.02f, %.02f, %.02f\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}
