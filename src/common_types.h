#pragma once

// This file can be included in the header and any cpp/hpp file to organize the types
// copied concept from
// https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/master/ray_tracing_intersection/shaders/host_device.h

#ifndef COMMON_TYPES
#define COMMON_TYPES

#ifdef __cplusplus
#include <glm/glm.hpp>
// GLSL Type
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
 #define START_BINDING(a) enum class a {
 #define END_BINDING() }
#else
 #define START_BINDING(a)  const uint
 #define END_BINDING() 
#endif

START_BINDING(ObjectType)
	t_Surface = 1,
	t_Triangle = 2,
	t_Tetrahedron = 3,
END_BINDING();

// clang-format on

// struct Aabb
// {
// 	vec3 minimum;
// 	vec3 maximum;
// };

// struct Sphere
// {
// 	vec3 center;
// 	float radius;
// };

#endif
