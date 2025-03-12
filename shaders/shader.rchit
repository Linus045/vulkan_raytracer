#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_debug_printf : enable

// TODO: Figure out what these do
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#define M_PI 3.1415926535897932384626433832795

#include "../include/common_types.h"
#include "../include/common_shader_functions.glsl"

hitAttributeEXT vec2 hitCoordinate;

layout(location = 0) rayPayloadInEXT Payload
{
	vec3 rayOrigin;
	vec3 rayDirection;
	vec3 previousNormal;

	vec3 directColor;
	vec3 indirectColor;
	int rayDepth;

	int rayActive;
}
payload;

layout(location = 1) rayPayloadEXT bool isShadow;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0) uniform Camera
{
	vec4 position;
	vec4 right;
	vec4 up;
	vec4 forward;

	uint frameCount;
}
camera;

layout(push_constant) uniform RaytracingDataConstants{
    // see common_types.h
    PUSH_CONSTANT_MEMBERS} raytracingDataConstants;

// layout(binding = 2, set = 0) buffer IndexBuffer { uint data[]; }
// indexBuffer;
// layout(binding = 3, set = 0) buffer VertexBuffer { float data[]; }
// vertexBuffer;

// layout(binding = 0, set = 1) buffer MaterialIndexBuffer { uint data[]; }
// materialIndexBuffer;
// layout(binding = 1, set = 1) buffer MaterialBuffer { Material data[]; }
// materialBuffer;

layout(binding = 5, set = 0, scalar) buffer Tetrahedrons
{
	Tetrahedron2[] tetrahedrons;
};

layout(set = 0, binding = 6, scalar) buffer Spheres
{
	Sphere[] spheres;
};

layout(set = 0, binding = 9, scalar) buffer GPUInstances
{
	GPUInstance[] gpuInstances;
};

float random(vec2 uv, float seed)
{
	return fract(sin(mod(dot(uv, vec2(12.9898, 78.233)) + 1113.1 * seed, M_PI)) * 43758.5453);
}

vec3 uniformSampleHemisphere(vec2 uv)
{
	float z = uv.x;
	float r = sqrt(max(0, 1.0 - z * z));
	float phi = 2.0 * M_PI * uv.y;

	return vec3(r * cos(phi), z, r * sin(phi));
}

vec3 alignHemisphereWithCoordinateSystem(vec3 hemisphere, vec3 up)
{
	vec3 right = normalize(cross(up, vec3(0.0072f, 1.0f, 0.0034f)));
	vec3 forward = cross(right, up);

	return hemisphere.x * right + hemisphere.y * up + hemisphere.z * forward;
}

void main()
{
	if (payload.rayActive == 0)
	{
		return;
	}

	// debugPrintfEXT("gl_HitKindTEXT: %d", gl_HitKindEXT);
	if (gl_HitKindEXT == t_SlicingPlane)
	{
		payload.rayActive = 0;
	}
	else if (gl_HitKindEXT == t_AABBDebug)
	{
		// we hit a AABB
		payload.directColor = vec3(0.8, 0.8, 0.8);
		payload.indirectColor = vec3(0, 0, 0);
		payload.rayActive = 0;
	}
	else if (gl_HitKindEXT == t_Tetrahedron1)
	{
		// if (payload.rayDepth == 0) {
		//  vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		//  vec3 positionToLightDirection = normalize(raytracingDataConstants.globalLightPosition -
		//  position); vec3 surfaceColor = vec3(1.0, 1.0, 0.0);

		//  Tetrahedron1 tetrahedron = tetrahedrons[gl_PrimitiveID];
		//  vec3 tetrahedronCenter = (tetrahedron.controlPoints[0] + tetrahedron.controlPoints[1] +
		//  tetrahedron.controlPoints[2] + tetrahedron.controlPoints[3]) / 4.0;

		//  // TODO: fix normal calculation, calculate using intersection point as input to geometry
		//  formula vec3 normal = normalize(position - tetrahedronCenter); payload.directColor =
		//  surfaceColor * raytracingDataConstants.globalLightColor *
		//   dot(normal, positionToLightDirection);
		// }
		// payload.rayActive = 0;
	}
	else if (gl_HitKindEXT == t_Tetrahedron2)
	{
		// debugPrintfEXT("gl_HitKindTEXT: %d", gl_HitKindEXT);
		if (payload.rayDepth == 0)
		{
			vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
			vec3 positionToLightDirection
			    = normalize(raytracingDataConstants.globalLightPosition - position);
			vec3 surfaceColor = vec3(1.0, 1.0, 0.0);

			Tetrahedron2 tetrahedron = tetrahedrons[gl_PrimitiveID];

			float u = hitCoordinate.x;
			float v = hitCoordinate.y;
			vec3 partialU = partialHu(tetrahedron.controlPoints, 2, u, v, 0);
			vec3 partialV = partialHv(tetrahedron.controlPoints, 2, u, v, 0);

			vec3 normal = normalize(cross(partialU, partialV));

			vec3 lightColor = raytracingDataConstants.globalLightColor
			                  * raytracingDataConstants.globalLightIntensity;

			payload.directColor
			    = surfaceColor * lightColor * max(0, dot(normal, positionToLightDirection));
			payload.indirectColor = vec3(0, 0, 0);
			payload.indirectColor = surfaceColor * raytracingDataConstants.environmentColor
			                        * raytracingDataConstants.environmentLightIntensity;
		}
		payload.rayActive = 0;
	}
	else if (gl_HitKindEXT == t_Sphere)
	{
		vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

		Sphere s = spheres[gl_PrimitiveID];

		vec3 positionToLightDirection
		    = normalize(raytracingDataConstants.globalLightPosition - position);

		// see ColorIdx in common_types.h
		vec3 colorlist[9] = {
		    vec3(1.0, 1.0, 1.0), // t_white
		    vec3(1.0, 0.0, 0.0), // t_red
		    vec3(0.0, 0.0, 1.0), // t_blue
		    vec3(0.0, 1.0, 0.0), // t_green
		    vec3(1.0, 1.0, 0.0), // t_yellow
		    vec3(1.0, 0.5, 0.0), // t_orange
		    vec3(1.0, 0.5, 0.8), // t_pink
		    vec3(0.5, 0.0, 1.0), // t_purple
		    vec3(0.0, 0.0, 0.0)  // t_black
		};
		vec3 surfaceColor = colorlist[s.colorIdx - 1];

		vec3 sphereNormal = normalize(position - s.center);
		payload.directColor = surfaceColor * raytracingDataConstants.globalLightColor
		                      * dot(sphereNormal, positionToLightDirection);
	}
	else if (gl_HitKindEXT == t_RectangularBezierSurface2x2)
	{
		// TODO: for now, disable any ray bounces from this surface
		if (payload.rayDepth == 0)
		{
			vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
			vec3 positionToLightDirection
			    = normalize(raytracingDataConstants.globalLightPosition - position);
			vec3 surfaceColor = vec3(0.0, 1.0, 0.0);

			// TODO: do some light calculations via the normals?
			// maybe subdivide into smaller patches that have normals defined
			// or add normals to the control points OR somehow calculate the normals
			vec3 surfaceNormal = vec3(0, 1, 0);
			payload.directColor = surfaceColor * raytracingDataConstants.globalLightColor
			                      * dot(surfaceNormal, positionToLightDirection);
			payload.rayActive = 0;
		}
		else
		{
		}
	}
	else
	{
		//   ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0],
		//                         indexBuffer.data[3 * gl_PrimitiveID + 1],
		//                         indexBuffer.data[3 * gl_PrimitiveID + 2]);

		//   vec3 barycentric = vec3(1.0 - hitCoordinate.x - hitCoordinate.y,
		//                           hitCoordinate.x, hitCoordinate.y);

		//   vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0],
		//                       vertexBuffer.data[3 * indices.x + 1],
		//                       vertexBuffer.data[3 * indices.x + 2]);
		//   vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0],
		//                       vertexBuffer.data[3 * indices.y + 1],
		//                       vertexBuffer.data[3 * indices.y + 2]);
		//   vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0],
		//                       vertexBuffer.data[3 * indices.z + 1],
		//                       vertexBuffer.data[3 * indices.z + 2]);

		//   vec3 position = vertexA * barycentric.x + vertexB * barycentric.y +
		//                   vertexC * barycentric.z;
		//   vec3 geometricNormal = normalize(cross(vertexB - vertexA, vertexC - vertexA));

		//   vec3 surfaceColor =
		//       materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]].diffuse;

		//   // 40 & 41 == light
		//   if (gl_PrimitiveID == 40 || gl_PrimitiveID == 41) {
		//     if (payload.rayDepth == 0) {
		//       payload.directColor =
		//           materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]]
		//               .emission;
		//     } else {
		//       payload.indirectColor +=
		//           (1.0 / payload.rayDepth) *
		//           materialBuffer.data[materialIndexBuffer.data[gl_PrimitiveID]]
		//               .emission *
		//           dot(payload.previousNormal, payload.rayDirection);
		//     }
		//   } else {
		//     int randomIndex =
		//         int(random(gl_LaunchIDEXT.xy, camera.frameCount) * 2 + 40);

		//     ivec3 lightIndices = ivec3(indexBuffer.data[3 * randomIndex + 0],
		//                                indexBuffer.data[3 * randomIndex + 1],
		//                                indexBuffer.data[3 * randomIndex + 2]);

		//     vec3 lightVertexA = vec3(vertexBuffer.data[3 * lightIndices.x + 0],
		//                              vertexBuffer.data[3 * lightIndices.x + 1],
		//                              vertexBuffer.data[3 * lightIndices.x + 2]);
		//     vec3 lightVertexB = vec3(vertexBuffer.data[3 * lightIndices.y + 0],
		//                              vertexBuffer.data[3 * lightIndices.y + 1],
		//                              vertexBuffer.data[3 * lightIndices.y + 2]);
		//     vec3 lightVertexC = vec3(vertexBuffer.data[3 * lightIndices.z + 0],
		//                              vertexBuffer.data[3 * lightIndices.z + 1],
		//                              vertexBuffer.data[3 * lightIndices.z + 2]);

		//     vec2 uv = vec2(random(gl_LaunchIDEXT.xy, camera.frameCount),
		//                    random(gl_LaunchIDEXT.xy, camera.frameCount + 1));
		//     if (uv.x + uv.y > 1.0f) {
		//       uv.x = 1.0f - uv.x;
		//       uv.y = 1.0f - uv.y;
		//     }

		//     vec3 lightBarycentric = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
		//     vec3 lightPosition = lightVertexA * lightBarycentric.x +
		//                          lightVertexB * lightBarycentric.y +
		//                          lightVertexC * lightBarycentric.z;

		//     vec3 positionToLightDirection = normalize(lightPosition - position);

		//     vec3 shadowRayOrigin = position;
		//     vec3 shadowRayDirection = positionToLightDirection;
		//     float shadowRayDistance = length(lightPosition - position) - 0.001f;

		//     uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT |
		//                           gl_RayFlagsOpaqueEXT |
		//                           gl_RayFlagsSkipClosestHitShaderEXT;

		//     isShadow = true;
		//     traceRayEXT(topLevelAS, shadowRayFlags, 0xFF, 0, 0, 1, shadowRayOrigin,
		//                 0.001, shadowRayDirection, shadowRayDistance, 1);

		//     if (!isShadow) {
		//       if (payload.rayDepth == 0) {
		//         payload.directColor = surfaceColor * raytracingDataConstants.globalLightColor *
		//                               dot(geometricNormal, positionToLightDirection);
		//       } else {
		//         payload.indirectColor +=
		//             (1.0 / payload.rayDepth) * surfaceColor *
		//             raytracingDataConstants.globalLightColor * dot(payload.previousNormal,
		//             payload.rayDirection) * dot(geometricNormal, positionToLightDirection);
		//       }
		//     } else {
		//       if (payload.rayDepth == 0) {
		//         payload.directColor = vec3(0.0, 0.0, 0.0);
		//       } else {
		//         payload.rayActive = 0;
		//       }
		//     }
		//   }

		//   vec3 hemisphere = uniformSampleHemisphere(
		//       vec2(random(gl_LaunchIDEXT.xy, camera.frameCount),
		//            random(gl_LaunchIDEXT.xy, camera.frameCount + 1)));
		//   vec3 alignedHemisphere =
		//       alignHemisphereWithCoordinateSystem(hemisphere, geometricNormal);

		//   payload.rayOrigin = position;
		//   payload.rayDirection = alignedHemisphere;
		//   payload.previousNormal = geometricNormal;

		//   payload.rayDepth += 1;
	}
}
