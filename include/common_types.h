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

const int MAX_NEWTON_ITERATIONS = 30;

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
	t_Tetrahedron1 = 1,
	t_Tetrahedron2 = 2,
	t_Tetrahedron3 = 3,
	t_Tetrahedron4 = 4,

	// not used
	t_RectangularBezierSurface2x2 = 10,

	// represents the sides of a tetrahedron that are outside, default
	t_BezierTriangle1 = 11,
	t_BezierTriangle2 = 12,
	t_BezierTriangle3 = 13,
	t_BezierTriangle4 = 14,

	// represents the sides of a tetrahedron that are inside (used for the slicing plane calculations)
	t_BezierTriangleInside1 = 21,
	t_BezierTriangleInside2 = 22,
	t_BezierTriangleInside3 = 23,
	t_BezierTriangleInside4 = 24,

	t_Sphere = 99,
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

#define UNIFORM_MEMBERS                                                                            \
	ALIGNAS(64) mat4 viewProj;                                                                     \
	ALIGNAS(64) mat4 viewInverse;                                                                  \
	ALIGNAS(64) mat4 projInverse;                                                                  \
	ALIGNAS(4) uint frameCount;

struct UniformStructure
{
	UNIFORM_MEMBERS
};

#define PUSH_CONSTANT_MEMBERS                                                                      \
	ALIGNAS(4) float newtonErrorXTolerance;                                                        \
	ALIGNAS(4) float newtonErrorFTolerance;                                                        \
	ALIGNAS(4) float newtonErrorFIgnoreIncrease;                                                   \
	ALIGNAS(4) float newtonErrorFHitBelowTolerance;                                                \
	ALIGNAS(4) int newtonMaxIterations;                                                            \
	ALIGNAS(4) int newtonGuessesAmount;                                                            \
	ALIGNAS(16) vec3 globalLightPosition;                                                          \
	ALIGNAS(16) vec3 globalLightColor;                                                             \
	ALIGNAS(4) float globalLightIntensity;                                                         \
	ALIGNAS(16) vec3 environmentColor;                                                             \
	ALIGNAS(4) float environmentLightIntensity;                                                    \
	ALIGNAS(4) float debugShowAABBs;                                                               \
	ALIGNAS(4) float renderSideTriangle;                                                           \
	ALIGNAS(4) int recursiveRaysPerPixel;                                                          \
	ALIGNAS(4) float debugPrintCrosshairRay;                                                       \
	ALIGNAS(4) float debugSlicingPlanes;                                                           \
	ALIGNAS(4) float enableSlicingPlanes;                                                          \
	ALIGNAS(4) float renderShadows;                                                                \
	ALIGNAS(4) float debugHighlightObjectEdges;                                                    \
	ALIGNAS(4) float debugFastRenderMode;                                                          \
	ALIGNAS(4) float debugVisualizeControlPoints;                                                  \
	ALIGNAS(4) float debugVisualizeSampledSurface;                                                 \
	ALIGNAS(4) float debugVisualizeSampledVolume;                                                  \
	ALIGNAS(16) vec3 cameraDir;

#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
struct RaytracingDataConstants
{
	PUSH_CONSTANT_MEMBERS
};

#else
// because we are using push_constant a block is required so we
// manually have to use PUSH_CONSTANT_MEMBERS in the shader inside the block
// we could also use a macro but I prefer the more explicit way
#endif

// stores the type and the index of the object to look it up in the respective buffer e.g. Spheres,
// Tetrahedrons, AABBs buffer
// type is the ObjectType enum
struct GPUInstance
{
	int type;
	int bufferIndex;

#ifdef __cplusplus
	GPUInstance() : type(0), bufferIndex(0)
	{
	}

	GPUInstance(ObjectType type, size_t bufferIndex)
	    : type(static_cast<int>(type)), bufferIndex(static_cast<int>(bufferIndex))
	{
	}
#endif
};

struct Ray
{
	vec3 origin;
	vec3 direction;
};

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

struct Tetrahedron4
{
	vec3 controlPoints[35];
};

struct Triangle
{
	vec3 a;
	vec3 b;
	vec3 c;
};

// NOTE: as far as I'm aware it is impossible to retrieve the AABB from the Acceleration Structure
// directly, so we need to manually store the AABB in the triangle struct to be able to
// retrieve it later in the shader
// Formula for amount of control points is: (n+1)(n+2)/2
struct BezierTriangle1
{
	vec3 controlPoints[3];
	Aabb aabb;
};

struct BezierTriangle2
{
	vec3 controlPoints[6];
	Aabb aabb;
};

struct BezierTriangle3
{
	vec3 controlPoints[10];
	Aabb aabb;
};

struct BezierTriangle4
{
	vec3 controlPoints[15];
	Aabb aabb;
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
