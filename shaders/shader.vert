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

// layout(location = 0) out vec3 fragColor;

void main(){
//	gl_Position = globalInfo.proj * globalInfo.view * ubo.modelMatrix * vec4(inPosition, 1.0);

//	vec3 l = normalize(-vec3(-0.5,1,1));
//	float cosTheta = dot(inNormal, l);

//	fragColor = vec3(0,1,0); //cosTheta * inColor;
}
