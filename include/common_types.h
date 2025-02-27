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
 #define ALIGNAS(x) alignas(x)
#else
 #define START_BINDING(a)  const uint
 #define END_BINDING() 
 #define ALIGNAS(x)
#endif

START_BINDING(ObjectType)
	t_Sphere = 1,
	t_Triangle = 2,
	t_Tetrahedron1 = 3,
	t_Tetrahedron2 = 4,
	t_Tetrahedron3 = 5,
	t_RectangularBezierSurface2x2 = 6,
	t_SlicingPlane = 80,
	t_AABBDebug = 100
END_BINDING();

// TODO: add proper materials, this is just temporary to make debugging easier
START_BINDING(ColorIdx)
	t_white = 1,
	t_red = 2,
	t_blue = 3,
	t_green = 4,
	t_yellow = 5,
	t_orange = 6,
	t_pink = 7,
	t_purple = 8,
	t_black = 9
END_BINDING();
// clang-format on

#define PUSH_CONSTANT_MEMBERS                                                                      \
	ALIGNAS(4) float newtonErrorXTolerance;                                                        \
	ALIGNAS(4) float newtonErrorFTolerance;                                                        \
	ALIGNAS(4) float newtonErrorFIgnoreIncrease;                                                   \
	ALIGNAS(4) float newtonErrorFHitBelowTolerance;                                                \
	ALIGNAS(4) float newtonErrorXIgnoreIncrease;                                                   \
	ALIGNAS(4) float newtonErrorXHitBelowTolerance;                                                \
	ALIGNAS(4) int newtonMaxIterations;                                                            \
	ALIGNAS(4) float someFloatingScalar;                                                           \
	ALIGNAS(4) int someScalar;                                                                     \
	ALIGNAS(16) vec3 globalLightPosition;                                                          \
	ALIGNAS(16) vec3 globalLightColor;                                                             \
	ALIGNAS(16) vec3 environmentColor;                                                             \
	ALIGNAS(4) float debugShowAABBs;                                                               \
	ALIGNAS(4) float renderSide1;                                                                  \
	ALIGNAS(4) float renderSide2;                                                                  \
	ALIGNAS(4) float renderSide3;                                                                  \
	ALIGNAS(4) float renderSide4;                                                                  \
	ALIGNAS(4) int raysPerPixel;                                                                   \
	ALIGNAS(16) vec3 cameraDir;

#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
struct RaytracingDataConstants
{
	PUSH_CONSTANT_MEMBERS
};
#else
// because we are using push_constant a block is required so we
// manually have to use PUSH_CONSTANT_DATA in the shader inside the block
// we could also use a macro but I prefer the more explicit way
#endif

struct Aabb
{
	vec3 minimum;
	vec3 maximum;
};

struct Sphere
{
	vec3 center;
	float radius;
	int colorIdx;
};

struct Tetrahedron1
{
	vec3 controlPoints[4];
};

struct Tetrahedron2
{
	vec3 controlPoints[10];
};

struct Tetrahedron3
{
	vec3 controlPoints[20];
};

struct Triangle
{
	vec3 a;
	vec3 b;
	vec3 c;
};

struct SlicingPlane
{
	vec3 planeOrigin;
	vec3 normal;
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
