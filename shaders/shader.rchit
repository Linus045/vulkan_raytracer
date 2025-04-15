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

#include "../include/common_types.h"
#include "../include/common_shader_functions.glsl"

struct HitData
{
	vec3 point;
	vec2 coords;
	vec3 normal;
};

bool isCrosshairRay = false;
hitAttributeEXT HitData hitData;

layout(location = 0) rayPayloadInEXT Payload
{
	vec3 rayOrigin;
	vec3 rayDirection;
	vec3 previousNormal;

	vec3 directColor;
	vec3 indirectColor;
	int rayDepth;

	int rayActive;
	// bool hitOnSlicingPlaneInsideObject;
}
payload;

layout(location = 1) rayPayloadEXT bool isShadow;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0) uniform UBO{// see common_types.h
                                         UNIFORM_MEMBERS} ubo;

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

layout(set = 0, binding = 11, scalar) buffer BezierTriangles3
{
	BezierTriangle3[] bezierTriangles3;
};

layout(set = 0, binding = 6, scalar) buffer Spheres
{
	Sphere[] spheres;
};

layout(set = 0, binding = 8, scalar) buffer SlicingPlanes
{
	SlicingPlane[] slicingPlanes;
};

layout(set = 0, binding = 9, scalar) buffer GPUInstances
{
	GPUInstance[] gpuInstances;
};

layout(set = 0, binding = 10, scalar) buffer BezierTriangles2
{
	BezierTriangle2[] bezierTriangles2;
};

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

	vec3 cameraDir = raytracingDataConstants.cameraDir;

	// whether or not the current ray direction is the crosshair's direction
	if (abs(dot(normalize(gl_WorldRayDirectionEXT), normalize(cameraDir)) - 1) < 0.0000001)
	{
		isCrosshairRay = true;
	}

	isCrosshairRay = raytracingDataConstants.debugPrintCrosshairRay > 0.0 && isCrosshairRay;

	GPUInstance instance = gpuInstances[gl_InstanceCustomIndexEXT];

	vec3 lightColor
	    = raytracingDataConstants.globalLightColor * raytracingDataConstants.globalLightIntensity;

	// debugPrintfEXT("gl_HitKindTEXT: %d", gl_HitKindEXT);
	if (gl_HitKindEXT == t_AABBDebug)
	{
		// we hit a AABB (debugging)
		payload.directColor = vec3(0.8, 0.8, 0.8);
		payload.indirectColor = vec3(0, 0, 0);
	}
	else if (gl_HitKindEXT == t_BezierTriangle2)
	{
		vec3 surfaceColor = vec3(1.0, 1.0, 0.0);

		// BezierTriangle2 bezierTriangle = bezierTriangles2[instance.bufferIndex];

		payload.indirectColor = surfaceColor * raytracingDataConstants.environmentColor
		                        * raytracingDataConstants.environmentLightIntensity;

		vec3 actualHitpoint = hitData.point;
		vec3 actualNormal = hitData.normal;
		bool hitSlicingPlane = false;

		// if we have slicing planes enables, we wanna treat points in front of the slicing
		// plane special
		//
		// check if we hit the inside of the object by comparing the normal with our ray direction
		// if the normal and the ray direction faces in the same direction, we hit the inside,
		// if the normal is facing us we hit the outside of the object
		if (raytracingDataConstants.enableSlicingPlanes > 0.0
		    && dot(hitData.normal, payload.rayDirection) > 0.0)
		{
			// we hit the inside of the object,
			// so we need move the hitpoint to the slicing plane instead
			SlicingPlane plane = slicingPlanes[0];
			float t = 0;
			vec4 cameraOrigin = ubo.viewInverse * vec4(0, 0, 0, 1);

			if (intersectWithPlane(
			        plane.normal, plane.planeOrigin, cameraOrigin.xyz, payload.rayDirection, t))
			{
				payload.rayOrigin = payload.rayOrigin + payload.rayDirection * t;
				actualHitpoint = payload.rayOrigin;
				actualNormal = plane.normal;
				hitSlicingPlane = true;
			}
			else
			{
				// this case should be impossible? how do we hit the inside of the object
				// without hitting the slicing plane on the way? we can only hit the inside of
				// an object if the slicing plane cuts it open because in the intersection
				// shader we only ignore hits if they are in front of the slicing plane - any
				// hit returned should be either on the outside aka. the normal faces us, or the
				// inside by going through the slicing plane first therefore in that case
				// the intersectionWithPlane() should always return true

				// One possibility is if the calculation of the hitpoint with the outside
				// fails and the hitpoint with the inside is returned instead
				// the other more likely possibility is if the camera origin is behind the slicing
				// plane therefore the ray direction is pointing away from the slicing plane

				// we can't differentiate if we hit the inside of the object because we went through
				// the slicing plane or if we hit the inside because we failed to calculate the
				// intersection with the outside

				// it is possible to check if the camera is hehind or in front of the slicing plane
				//  but that would not help really

				// calculation of the hit point failed, color red
				payload.directColor = vec3(1.0, 0.0, 0.0);
				payload.indirectColor = vec3(0.0, 0.0, 0.0);
			}
		}

		// fire a ray to the light to check if we are in shadow
		vec3 positionToLightDirection
		    = normalize(raytracingDataConstants.globalLightPosition - actualHitpoint);
		vec3 shadowRayOrigin = actualHitpoint;
		vec3 shadowRayDirection = positionToLightDirection;
		uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT
		                      | gl_RayFlagsSkipClosestHitShaderEXT;

		float tMin = 0.001;
		float tMax = length(raytracingDataConstants.globalLightPosition - shadowRayOrigin) - 0.005f;

		isShadow = true;
		traceRayEXT(topLevelAS,         // top level acceleration structure
		            shadowRayFlags,     // rayFlags
		            0xFF,               // cullMask
		            0,                  // sbtRecordOffset
		            0,                  // sbtRecordStride
		            1,                  // miss index
		            shadowRayOrigin,    // origin
		            tMin,               // Tmin
		            shadowRayDirection, // direction
		            tMax,               // Tmax
		            1                   // payloadIndex (location = 1)
		);

		if (isCrosshairRay)
		{
			// debugprint the hit pos, normal
			debugPrintfEXT("HitPos: (%.2v3f), normal: (%.2v3f), isShadow: %d",
			               actualHitpoint,
			               actualNormal,
			               isShadow);
		}

		// we hit the outside of the object, calculate color using surface normal
		if (!isShadow)
		{
			payload.directColor
			    = surfaceColor * lightColor * max(0, dot(actualNormal, positionToLightDirection));
		}
		else
		{
			payload.directColor = vec3(0.0, 0.0, 0.0);
		}

		if (hitSlicingPlane && raytracingDataConstants.debugSlicingPlanes > 0.0)
		{
			payload.directColor = vec3(1.0, 0.0, 1.0);
			payload.indirectColor = vec3(0.0, 0.0, 0.0);
		}

		if (raytracingDataConstants.debugShowSubdivisions > 0.0)
		{
			float u = hitData.coords.x;
			float v = hitData.coords.y;
			if (u < 1e-2f || v < 1e-2f || abs(1.0 - u - v) < 1e-2f)
			{
				payload.directColor = vec3(1, 1, 1);
				payload.indirectColor = vec3(0, 0, 0);
			}
		}
	}
	else if (gl_HitKindEXT == t_BezierTriangle3)
	{
		vec3 surfaceColor = vec3(1.0, 1.0, 0.0);

		// BezierTriangle3 bezierTriangle = bezierTriangles3[instance.bufferIndex];

		payload.indirectColor = surfaceColor * raytracingDataConstants.environmentColor
		                        * raytracingDataConstants.environmentLightIntensity;

		vec3 actualHitpoint = hitData.point;
		vec3 actualNormal = hitData.normal;
		bool hitSlicingPlane = false;

		// if we have slicing planes enables, we wanna treat points in front of the slicing
		// plane special
		//
		// check if we hit the inside of the object by comparing the normal with our ray direction
		// if the normal and the ray direction faces in the same direction, we hit the inside,
		// if the normal is facing us we hit the outside of the object
		if (raytracingDataConstants.enableSlicingPlanes > 0.0
		    && dot(hitData.normal, payload.rayDirection) > 0.0)
		{
			// we hit the inside of the object,
			// so we need move the hitpoint to the slicing plane instead
			SlicingPlane plane = slicingPlanes[0];
			float t = 0;
			vec4 cameraOrigin = ubo.viewInverse * vec4(0, 0, 0, 1);

			if (intersectWithPlane(
			        plane.normal, plane.planeOrigin, cameraOrigin.xyz, payload.rayDirection, t))
			{
				payload.rayOrigin = payload.rayOrigin + payload.rayDirection * t;
				actualHitpoint = payload.rayOrigin;
				actualNormal = plane.normal;
				hitSlicingPlane = true;
			}
			else
			{
				// this case should be impossible? how do we hit the inside of the object
				// without hitting the slicing plane on the way? we can only hit the inside of
				// an object if the slicing plane cuts it open because in the intersection
				// shader we only ignore hits if they are in front of the slicing plane - any
				// hit returned should be either on the outside aka. the normal faces us, or the
				// inside by going through the slicing plane first therefore in that case
				// the intersectionWithPlane() should always return true

				// One possibility is if the calculation of the hitpoint with the outside
				// fails and the hitpoint with the inside is returned instead
				// the other more likely possibility is if the camera origin is behind the slicing
				// plane therefore the ray direction is pointing away from the slicing plane

				// we can't differentiate if we hit the inside of the object because we went through
				// the slicing plane or if we hit the inside because we failed to calculate the
				// intersection with the outside

				// it is possible to check if the camera is hehind or in front of the slicing plane
				//  but that would not help really

				// calculation of the hit point failed, color red
				payload.directColor = vec3(1.0, 0.0, 0.0);
				payload.indirectColor = vec3(0.0, 0.0, 0.0);
			}
		}

		// fire a ray to the light to check if we are in shadow
		vec3 positionToLightDirection
		    = normalize(raytracingDataConstants.globalLightPosition - actualHitpoint);
		vec3 shadowRayOrigin = actualHitpoint;
		vec3 shadowRayDirection = positionToLightDirection;
		uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT
		                      | gl_RayFlagsSkipClosestHitShaderEXT;

		float tMin = 0.001;
		float tMax = length(raytracingDataConstants.globalLightPosition - shadowRayOrigin) - 0.005f;

		isShadow = true;
		traceRayEXT(topLevelAS,         // top level acceleration structure
		            shadowRayFlags,     // rayFlags
		            0xFF,               // cullMask
		            0,                  // sbtRecordOffset
		            0,                  // sbtRecordStride
		            1,                  // miss index
		            shadowRayOrigin,    // origin
		            tMin,               // Tmin
		            shadowRayDirection, // direction
		            tMax,               // Tmax
		            1                   // payloadIndex (location = 1)
		);

		if (isCrosshairRay)
		{
			// debugprint the hit pos, normal
			debugPrintfEXT("HitPos: (%.2v3f), normal: (%.2v3f), isShadow: %d",
			               actualHitpoint,
			               actualNormal,
			               isShadow);
		}

		// we hit the outside of the object, calculate color using surface normal
		if (!isShadow)
		{
			payload.directColor
			    = surfaceColor * lightColor * max(0, dot(actualNormal, positionToLightDirection));
		}
		else
		{
			payload.directColor = vec3(0.0, 0.0, 0.0);
		}

		if (hitSlicingPlane && raytracingDataConstants.debugSlicingPlanes > 0.0)
		{
			payload.directColor = vec3(1.0, 0.0, 1.0);
			payload.indirectColor = vec3(0.0, 0.0, 0.0);
		}

		if (raytracingDataConstants.debugShowSubdivisions > 0.0)
		{
			float u = hitData.coords.x;
			float v = hitData.coords.y;
			if (u < 1e-2f || v < 1e-2f || abs(1.0 - u - v) < 1e-2f)
			{
				payload.directColor = vec3(1, 1, 1);
				payload.indirectColor = vec3(0, 0, 0);
			}
		}
	}
	else if (gl_HitKindEXT == t_Sphere)
	{
		Sphere s = spheres[instance.bufferIndex];

		if (instance.bufferIndex == 0)
		{
			payload.directColor = vec3(1.0, 0.9, 0.3);
			payload.indirectColor = vec3(0, 0, 0);
		}
		else
		{
			vec3 positionToLightDirection
			    = normalize(raytracingDataConstants.globalLightPosition - hitData.point);
			// fire a ray to the light to check if we are in shadow
			vec3 shadowRayOrigin = hitData.point;
			vec3 shadowRayDirection = positionToLightDirection;
			uint shadowRayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT
			                      | gl_RayFlagsSkipClosestHitShaderEXT;

			float tMin = 0.001;
			float tMax
			    = length(raytracingDataConstants.globalLightPosition - shadowRayOrigin) - 0.005f;

			isShadow = true;
			traceRayEXT(topLevelAS,         // top level acceleration structure
			            shadowRayFlags,     // rayFlags
			            0xFF,               // cullMask
			            0,                  // sbtRecordOffset
			            0,                  // sbtRecordStride
			            1,                  // miss index
			            shadowRayOrigin,    // origin
			            tMin,               // Tmin
			            shadowRayDirection, // direction
			            tMax,               // Tmax
			            1                   // payloadIndex (location = 1)
			);

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

			vec3 normal = normalize(hitData.point - s.center);

			payload.indirectColor = surfaceColor * raytracingDataConstants.environmentColor
			                        * raytracingDataConstants.environmentLightIntensity;
			if (!isShadow)
			{
				payload.directColor
				    = surfaceColor * lightColor * max(0, dot(normal, positionToLightDirection));
			}
			else
			{
				payload.directColor = vec3(0.0, 0.0, 0.0);
			}
		}
	}
	else if (gl_HitKindEXT == t_RectangularBezierSurface2x2)
	{
		// // TODO: for now, disable any ray bounces from this surface
		// if (payload.rayDepth == 0)
		// {
		// 	vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		// 	vec3 positionToLightDirection
		// 	    = normalize(raytracingDataConstants.globalLightPosition - position);
		// 	vec3 surfaceColor = vec3(0.0, 1.0, 0.0);

		// 	// TODO: do some light calculations via the normals?
		// 	// maybe subdivide into smaller patches that have normals defined
		// 	// or add normals to the control points OR somehow calculate the normals
		// 	vec3 surfaceNormal = vec3(0, 1, 0);
		// 	payload.directColor = surfaceColor * raytracingDataConstants.globalLightColor
		// 	                      * dot(surfaceNormal, positionToLightDirection);
		// }
		// else
		// {
		// }
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
		//         payload.directColor = surfaceColor * raytracingDataConstants.globalLightColor
		//         *
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
	}
	payload.rayDepth += 1;
}
