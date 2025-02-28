#pragma once

#include "blas.hpp"
#include "tlas.hpp"
#include "worldobject.hpp"

namespace ltracer
{
namespace rt
{

template <typename T>
class RaytracingWorldObject : public WorldObject
{
  public:
	RaytracingWorldObject<T>(const ObjectType type,
	                         const AABB& aabb,
	                         const T& object,
	                         glm::vec3 position)
	    : WorldObject(position), geometry(Geometry(aabb, object)), type(type)
	{
	}

	~RaytracingWorldObject() override = default;
	RaytracingWorldObject(const RaytracingWorldObject&) = default;
	RaytracingWorldObject& operator=(const RaytracingWorldObject&) = delete;

	RaytracingWorldObject(RaytracingWorldObject&&) noexcept = default;
	RaytracingWorldObject& operator=(RaytracingWorldObject&&) noexcept = delete;

	Geometry<T>& getGeometry()
	{
		return geometry;
	}

	ObjectType getType() const
	{
		return type;
	}

	const VkTransformMatrixKHR getTransformMatrix() const
	{
		auto pos = getTransform().getPos();
		auto scale = getTransform().getScale();
		auto rot = getTransform().getRotation();

		glm::mat4 modelMatrix = glm::identity<glm::mat4>();
		modelMatrix = glm::scale(modelMatrix, scale);
		modelMatrix = glm::toMat4(rot) * modelMatrix;
		modelMatrix = glm::translate(modelMatrix, pos);
		return {
			.matrix = {
				{modelMatrix[0][0], modelMatrix[1][0], modelMatrix[2][0], modelMatrix[3][0]},
				{modelMatrix[0][1], modelMatrix[1][1], modelMatrix[2][1], modelMatrix[3][1]},
				{modelMatrix[0][2], modelMatrix[1][2], modelMatrix[2][2], modelMatrix[3][2]},
			},
		};
	}

	// const TLASInstance& getTLASInstance() const
	// {
	// 	return tlasInstance;
	// }

  private:
	Geometry<T> geometry;
	const ObjectType type;
	// TLASInstance& tlasInstance;
};

} // namespace rt
} // namespace ltracer
