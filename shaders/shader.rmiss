#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT Payload {
  vec3 rayOrigin;
  vec3 rayDirection;
  vec3 previousNormal;

  vec3 directColor;
  vec3 indirectColor;
  int rayDepth;

  int rayActive;
}
payload;

void main() { 
	// TODO: REMOVE THIS LINE - it's just for testing
	// payload.directColor = vec3(0.6);


	payload.rayActive = 0;
}
