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

namespace ltracer
{

struct Transform
{
	glm::vec3 position{0.0f, 0.0f, 0.0f};
	glm::quat rotation = glm::quat();
	glm::vec3 scale{1.0f, 1.0f, 1.0f};

	glm::vec3 getForward() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(0, 0, -1)));
	}

	glm::vec3 getUp() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(0, 1, 0)));
	}

	glm::vec3 getRight() const
	{
		return glm::normalize(glm::rotate(rotation, glm::vec3(1, 0, 0)));
	}

	VkTransformMatrixKHR getTransformMatrix()
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
};

} // namespace ltracer
