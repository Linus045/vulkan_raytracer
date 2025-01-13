#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"

#include "src/transform.hpp"

namespace ltracer
{

class WorldObject
{
  public:
	Transform transform;

	WorldObject(glm::vec3 position = glm::vec3{0, 0, 0})
	{
		transform.position = position;
	}

	// TODO: make this a more generic rotation function
	void updateRotation(float time)
	{
		transform.rotation
		    = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void translate(glm::vec3 translation)
	{
		transform.position += translation;
	}

	void translate(float x, float y, float z)
	{
		translate(glm::vec3(x, y, z));
	}

	void setPosition(glm::vec3 position)
	{
		transform.position = position;
	}

	glm::mat4 getModelMatrix() const
	{
		glm::mat4 modelMatrix = glm::identity<glm::mat4>();
		modelMatrix = glm::scale(modelMatrix, transform.scale);
		modelMatrix = glm::toMat4(transform.rotation) * modelMatrix;
		modelMatrix = glm::translate(modelMatrix, transform.position);
		return modelMatrix;
	}

  private:
};

} // namespace ltracer
