#pragma once

#include "geometry.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"

#include "transform.hpp"

namespace tracer
{

// Another wrapper for the simple object classes in common_types.h
class WorldObject
{
  public:
	WorldObject(const glm::vec3 position = glm::vec3{0, 0, 0}) : transform(position)
	{
	}

	virtual ~WorldObject() = default;
	WorldObject(const WorldObject&) = default;
	WorldObject& operator=(const WorldObject&) = delete;

	WorldObject(WorldObject&&) noexcept = default;
	WorldObject& operator=(WorldObject&&) noexcept = default;

	// TODO: make this a more generic rotation function
	void updateRotation(const float time)
	{
		transform.setRotation(
		    glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	virtual void translate(const glm::vec3 translation)
	{
		transform.translate(translation);
	}

	virtual void translate(const float x, const float y, const float z)
	{
		translate(glm::vec3(x, y, z));
	}

	virtual void setPosition(const glm::vec3 position)
	{
		transform.setPos(position);
	}

	glm::mat4 getModelMatrix() const
	{
		glm::mat4 modelMatrix = glm::identity<glm::mat4>();
		modelMatrix = glm::scale(modelMatrix, transform.getScale());
		modelMatrix = glm::toMat4(transform.getRotation()) * modelMatrix;
		modelMatrix = glm::translate(modelMatrix, transform.getPos());
		return modelMatrix;
	}

	const Transform& getTransform() const
	{
		return transform;
	}

  protected:
	// geometry is only present if its not a mesh model e.g. we wanna use raytracing to render it
	Transform transform;
};

} // namespace tracer
