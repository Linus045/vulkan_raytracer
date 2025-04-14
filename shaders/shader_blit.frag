#version 450

layout(set = 0, binding = 0) uniform sampler2D raytracedImage;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

// used to copy the raytraced image to the framebuffer
void main()
{
	fragColor = texture(raytracedImage, uv);
	// fragColor = vec4(uv.x, uv.y, 0.4, 1.0);
}
