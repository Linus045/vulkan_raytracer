#pragma once

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace tracer
{

// a simple transform class to hold the position, rotation and scale of an object
struct Transform
{
	explicit Transform(const glm::vec3 pos,
	                   const glm::quat rotation = glm::quat(),
	                   const glm::vec3 scale = glm::vec3(1))
	    : position(pos), rotation(rotation), scale(scale)
	{
	}

	inline void setPos(const glm::vec3 pos)
	{
		position = pos;
	}

	inline void setRotation(const glm::quat rot)
	{
		rotation = rot;
	}

	inline void setScale(const glm::vec3 s)
	{
		scale = s;
	}

	inline glm::vec3 getPos() const
	{
		return position;
	}

	inline glm::quat getRotation() const
	{
		return rotation;
	}

	inline glm::vec3 getScale() const
	{
		return scale;
	}

	inline float getX() const
	{
		return position.x;
	}

	inline float getY() const
	{
		return position.y;
	}

	inline float getZ() const
	{
		return position.z;
	}

	inline void translate(glm::vec3 offset)
	{
		position += offset;
	}

	inline glm::vec3 getForward() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(0, 0, -1)));
	}

	inline glm::vec3 getUp() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(0, 1, 0)));
	}

	inline glm::vec3 getRight() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(1, 0, 0)));
	}

	VkTransformMatrixKHR getTransformMatrix() const
	{
		glm::mat4 matrix = glm::translate(glm::identity<glm::mat4>(), position);
		matrix = matrix * glm::toMat4(rotation);
		matrix = glm::scale(matrix, scale);

		VkTransformMatrixKHR transformationMatrix {
		    .matrix = {
				{matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]},
				{matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]},
				{matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]},
			},
		};
		return transformationMatrix;
	}

  private:
	glm::vec3 position{0.0f, 0.0f, 0.0f};
	glm::quat rotation = glm::quat();
	glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

} // namespace tracer
