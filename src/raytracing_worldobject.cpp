#include "raytracing_worldobject.hpp"

namespace tracer
{
namespace rt
{

template <>
void RaytracingWorldObject<Sphere>::setPosition(const glm::vec3 position)
{
	Sphere& sphere = geometry.getData();
	sphere.center = position;
	transform.setPos(position);
}

template <>
void RaytracingWorldObject<Sphere>::translate(const glm::vec3 translation)
{
	Sphere& sphere = geometry.getData();
	sphere.center += translation;
	transform.translate(translation);
}

template <>
void RaytracingWorldObject<Sphere>::translate(const float x, const float y, const float z)
{
	Sphere& sphere = geometry.getData();
	auto t = glm::vec3(x, y, z);
	sphere.center += t;
	transform.translate(t);
}

} // namespace rt
} // namespace tracer
