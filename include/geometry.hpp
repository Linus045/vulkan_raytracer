
#pragma once

#include "aabb.hpp"
namespace tracer
{

// holds the geometry data and the AABB of the object to be loaded into a BLAS
template <typename T>
class Geometry
{
  public:
	Geometry(const AABB& aabb, const T& data) : aabb(aabb), data(data)
	{
	}

	virtual ~Geometry() = default;
	Geometry(const Geometry&) = default;
	Geometry& operator=(const Geometry&) = delete;

	Geometry(Geometry&&) noexcept = default;
	Geometry& operator=(Geometry&&) noexcept = delete;

	const AABB& getAABB() const
	{
		return aabb;
	}

	const T& getData() const
	{
		return data;
	}

	T& getData()
	{
		return data;
	}

  private:
	AABB aabb;
	T data;
};

} // namespace tracer
