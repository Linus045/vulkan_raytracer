#version 450

// layout(set = 0, binding = 0) uniform GlobalInfo {
// 	mat4 view;
// 	mat4 proj;
// } globalInfo;
//
// layout(set = 1, binding = 0) uniform UniformBufferObject {
// 	mat4 modelMatrix;
// } ubo;

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inColor;
// layout(location = 2) in vec3 inNormal;

// fullscreen quad rendering
// see:
// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
layout(location = 0) out vec2 outUV;

void main()
{
	//	gl_Position = globalInfo.proj * globalInfo.view * ubo.modelMatrix * vec4(inPosition, 1.0);

	//	vec3 l = normalize(-vec3(-0.5,1,1));
	//	float cosTheta = dot(inNormal, l);

	//	fragColor = vec3(0,1,0); //cosTheta * inColor;

	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}
