
#pragma once

#include "aabb.hpp"
#include <stdexcept>
namespace ltracer
{

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

	/**
	 * @brief recalculate the AABB for the geometry
	 *
	 * @return whether the AABB dimensions changed
	 */
	virtual bool recalculateAABB()
	{
		throw new std::runtime_error("recalculateAABB not implemented for type T. Please add a "
		                             "specialization in geometry.cpp");
	}

  private:
	AABB aabb;
	T data;
};

template <>
bool Geometry<Sphere>::recalculateAABB();

template <>
bool Geometry<Tetrahedron2>::recalculateAABB();

template <>
bool Geometry<RectangularBezierSurface2x2>::recalculateAABB();

} // namespace ltracer
