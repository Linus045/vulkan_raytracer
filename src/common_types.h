/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

// CHANGES: edited to include my custom tyoes
// This file can be included in the header and any cpp/hpp file to organize the types
// See original here:
// https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/master/ray_tracing_intersection/shaders/host_device.h
// specifically (with commit hash):
// https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/1fc4444819c9fc01f7f1c1bd3c9c7f4ae2534430/ray_tracing_intersection/shaders/host_device.h

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
	t_Sphere = 1,
	t_Triangle = 2,
	t_Tetrahedron = 3,
	t_RectangularBezierSurface2x2 = 4
END_BINDING();

// clang-format on

struct Aabb
{
	vec3 minimum;
	vec3 maximum;
};

struct Sphere
{
	vec3 center;
	float radius;
};

struct Tetrahedron
{
	vec3 a;
	vec3 b;
	vec3 c;
};

struct Triangle
{
	vec3 a;
	vec3 b;
	vec3 c;
};

struct RectangularBezierSurface2x2
{
	vec3 controlPoints[16];
};

struct Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
};

#endif
