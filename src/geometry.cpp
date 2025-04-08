#include "geometry.hpp"
namespace tracer
{

template <>
bool Geometry<Sphere>::recalculateAABB()
{
	auto oldAABB = aabb;
	aabb = AABB::fromSphere(data);

	float xDimension = aabb.max.x - aabb.min.x;
	float yDimension = aabb.max.y - aabb.min.y;
	float zDimension = aabb.max.z - aabb.min.z;
	float oldXDimension = oldAABB.max.x - oldAABB.min.x;
	float oldYDimension = oldAABB.max.y - oldAABB.min.y;
	float oldZDimension = oldAABB.max.z - oldAABB.min.z;
	return glm::abs(xDimension - oldXDimension) > 1e-5
	       || glm::abs(yDimension - oldYDimension) > 1e-5
	       || glm::abs(zDimension - oldZDimension) > 1e-5;
}

template <>
bool Geometry<Tetrahedron2>::recalculateAABB()
{
	auto oldAABB = aabb;
	aabb = AABB::fromTetrahedron2(data);

	float xDimension = aabb.max.x - aabb.min.x;
	float yDimension = aabb.max.y - aabb.min.y;
	float zDimension = aabb.max.z - aabb.min.z;
	float oldXDimension = oldAABB.max.x - oldAABB.min.x;
	float oldYDimension = oldAABB.max.y - oldAABB.min.y;
	float oldZDimension = oldAABB.max.z - oldAABB.min.z;
	return glm::abs(xDimension - oldXDimension) > 1e-5
	       || glm::abs(yDimension - oldYDimension) > 1e-5
	       || glm::abs(zDimension - oldZDimension) > 1e-5;
}

template <>
bool Geometry<RectangularBezierSurface2x2>::recalculateAABB()
{
	auto oldAABB = aabb;
	aabb = AABB::fromRectangularBezierSurface2x2(data);

	float xDimension = aabb.max.x - aabb.min.x;
	float yDimension = aabb.max.y - aabb.min.y;
	float zDimension = aabb.max.z - aabb.min.z;
	float oldXDimension = oldAABB.max.x - oldAABB.min.x;
	float oldYDimension = oldAABB.max.y - oldAABB.min.y;
	float oldZDimension = oldAABB.max.z - oldAABB.min.z;
	return glm::abs(xDimension - oldXDimension) > 1e-5
	       || glm::abs(yDimension - oldYDimension) > 1e-5
	       || glm::abs(zDimension - oldZDimension) > 1e-5;
}

} // namespace tracer
