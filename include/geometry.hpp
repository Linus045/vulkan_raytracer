
#pragma once

#include "aabb.hpp"
#include <stdexcept>
namespace ltracer
{

template <typename T>
class Geometry
{
  public:
	Geometry(const AABB aabb, const T& data) : aabb(aabb), data(data)
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

	T& getData()
	{
		return data;
	}

	virtual void recalculateAABB()
	{
		throw new std::runtime_error("recalculateAABB not implemented for type T. Please add a "
		                             "specialization in geometry.cpp");
	}

  private:
	AABB aabb;
	T data;
};

} // namespace ltracer
