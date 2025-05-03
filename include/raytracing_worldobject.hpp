#pragma once

#include <stdexcept>

#include "aabb.hpp"
#include "worldobject.hpp"
#include "common_types.h"

namespace tracer
{
namespace rt
{

template <typename T>
class RaytracingWorldObject : public WorldObject
{
  public:
	RaytracingWorldObject(const ObjectType type,
	                      const AABB& aabb,
	                      const T& object,
	                      const glm::vec3 position)
	    : WorldObject(position), geometry(Geometry(aabb, object)), type(type)
	{
	}

	~RaytracingWorldObject() override = default;
	RaytracingWorldObject(const RaytracingWorldObject&) = delete;
	RaytracingWorldObject& operator=(const RaytracingWorldObject&) = delete;

	RaytracingWorldObject(RaytracingWorldObject&&) noexcept = default;
	RaytracingWorldObject& operator=(RaytracingWorldObject&&) noexcept = delete;

	const Geometry<T>& getGeometry() const
	{
		return geometry;
	}

	Geometry<T>& getGeometry()
	{
		return geometry;
	}

	ObjectType getType() const
	{
		return type;
	}

	void setType(const ObjectType new_type)
	{
		type = new_type;
	}

	void setPosition(const glm::vec3) override
	{
		throw new std::runtime_error("setPosition not implemented for type T. Please add a "
		                             "specialization in raytracing_worldobject.cpp");
	}

	void translate(const glm::vec3) override
	{
		throw new std::runtime_error("setPosition not implemented for type T. Please add a "
		                             "specialization in raytracing_worldobject.cpp");
	}

	void translate(const float, const float, const float) override
	{
		throw new std::runtime_error("setPosition not implemented for type T. Please add a "
		                             "specialization in raytracing_worldobject.cpp");
	}

	// const TLASInstance& getTLASInstance() const
	// {
	// 	return tlasInstance;
	// }

  protected:
	Geometry<T> geometry;
	ObjectType type;
};

template <>
void RaytracingWorldObject<Sphere>::setPosition(const glm::vec3 position);

template <>
void RaytracingWorldObject<Sphere>::translate(const glm::vec3 translation);

template <>
void RaytracingWorldObject<Sphere>::translate(const float x, const float y, const float z);

} // namespace rt
} // namespace tracer
