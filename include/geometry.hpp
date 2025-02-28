
#pragma once

#include "aabb.hpp"
namespace ltracer
{

template <typename T>
class Geometry
{
  public:
	Geometry(const AABB aabb, const T& data) : aabb(aabb), data(data)
	{
	}

	~Geometry() = default;
	Geometry(const Geometry&) = default;
	Geometry& operator=(const Geometry&) = delete;

	Geometry(Geometry&&) noexcept = default;
	Geometry& operator=(Geometry&&) noexcept = delete;

	AABB getAABB() const
	{
		return aabb;
	}

	T& getData()
	{
		return data;
	}

  private:
	const AABB aabb;
	T data;
};

} // namespace ltracer
