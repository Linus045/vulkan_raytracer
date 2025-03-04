#include "geometry.hpp"
namespace ltracer
{

template <>
void Geometry<Sphere>::recalculateAABB()
{
	aabb = AABB::fromSphere(data);
}

template <>
void Geometry<Tetrahedron2>::recalculateAABB()
{
	aabb = AABB::fromTetrahedron2(data);
}

template <>
void Geometry<RectangularBezierSurface2x2>::recalculateAABB()
{
	aabb = AABB::fromRectangularBezierSurface2x2(data);
}

} // namespace ltracer
