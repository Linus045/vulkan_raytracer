#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 1) rayPayloadEXT BounceRayPayload
{
	bool isShadow;
}
bounceRayPayload;

void main()
{
	bounceRayPayload.isShadow = false;
}
