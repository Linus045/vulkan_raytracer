#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"

#include "glm/gtx/quaternion.hpp"

#include <cassert>
#include <cstdint>

#include "transform.hpp"

namespace ltracer
{

class Camera
{
  public:
	Transform transform;
	const glm::vec3 globalUp = {0, 1, 0};

  public:
	explicit Camera()
	{
		transform.setPos(glm::vec3(0.0, 0, -0.01f));
		glm::vec3 up = globalUp;
		transform.setRotation(glm::quatLookAtRH(glm::normalize(glm::vec3(0.0, 0, 1)), up));
		pitchRadians = glm::pitch(transform.getRotation());
		yawRadians = glm::yaw(transform.getRotation());

		updateViewMatrix();
	}

	~Camera() = default;

	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;

	Camera(Camera&&) = delete;
	Camera& operator=(Camera&&) = delete;

	glm::mat4 getProjectionMatrix() const
	{
		return projectionMatrix;
	}

	glm::mat4 getViewMatrix() const
	{
		return viewMatrix;
	}

	void updateScreenSize(const uint32_t width, const uint32_t height)
	{
		screen_width = width;
		screen_height = height;
		updateProjectionMatrix();
	}

	void translate(glm::vec3 translation)
	{
		transform.translate(translation);
		updateViewMatrix();
		cameraMoved = true;
	}

	void translate(float x, float y, float z)
	{
		translate(glm::vec3(x, y, z));
	}

	void rotateAxisAngle(const glm::vec3& axis, const float angleDegree)
	{
		// Calculate the new alignment
		glm::quat q = glm::angleAxis(glm::radians(angleDegree), glm::normalize(axis));
		transform.setRotation(glm::normalize(q * transform.getRotation()));
		updateViewMatrix();
		cameraMoved = true;
	}

	void rotateYawY(const float angleDegree)
	{
		yawRadians += glm::radians(angleDegree);

		if (yawRadians > glm::pi<float>())
		{
			yawRadians -= glm::two_pi<float>();
		}
		else if (yawRadians < -glm::pi<float>())
		{
			yawRadians += glm::two_pi<float>();
		}

		transform.setRotation(glm::normalize(glm::quat(glm::vec3(pitchRadians, yawRadians, 0))));

		updateViewMatrix();
		cameraMoved = true;
	}

	void rotatePitchX(const float angleDegree)
	{
		pitchRadians += glm::radians(angleDegree);
		limitPitch();

		transform.setRotation(glm::normalize(glm::quat(glm::vec3(pitchRadians, yawRadians, 0))));

		updateViewMatrix();
		cameraMoved = true;
	}

	void limitPitch()
	{
		pitchRadians = glm::clamp(
		    pitchRadians, -glm::radians(pitchLimit_degree), glm::radians(pitchLimit_degree));
	}

	bool isCameraMoved() const
	{
		return cameraMoved;
	}

	void resetCameraMoved()
	{
		cameraMoved = false;
	}

	float getPitchRadians() const
	{
		return pitchRadians;
	}

	float getYawRadians() const
	{
		return yawRadians;
	}

	void setMovementSpeed(const float speed)
	{
		movementSpeed = glm::clamp(speed, movementSpeedMin, movementSpeedMax);
	}

	float getMovementSpeed() const
	{
		return movementSpeed;
	}

	void setRotationSpeed(const float speed)
	{
		rotationSpeed = glm::clamp(speed, rotationSpeedMin, rotationSpeedMax);
	}

	float getRotationSpeed() const
	{
		return rotationSpeed;
	}

  private:
	void updateProjectionMatrix()
	{
		assert(screen_width != 0);
		assert(screen_height != 0);

		// float aspect = screen_width / (float)screen_height;
		// float fov_x = atan(tan(90.0f / 2.0f) * aspect) * 2.0f;

		projectionMatrix = glm::perspectiveFovRH_ZO(
		    glm::radians(fovy_degree), (float)screen_width, (float)screen_height, zNear, zFar);
		projectionMatrix[1][1] *= -1.0f;

		// logMat4("projectionMatrix", projectionMatrix);
	}

	void updateViewMatrix()
	{
		auto cameraUp = globalUp;
		auto cameraRight = glm::vec3(1, 0, 0);
		glm::mat4 cameraRotation = glm::mat4();
		cameraRotation = glm::rotate(cameraRotation, yawRadians, cameraUp);
		cameraRotation = glm::rotate(cameraRotation, pitchRadians, cameraRight);

		viewMatrix = glm::inverse(cameraRotation);
	}

  private:
	float pitchLimit_degree = 89.0f; // Limit for up/down rotation (in degrees)
	bool cameraMoved = true;         // start with true to reset image on first frame

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	uint32_t screen_width = 0;
	uint32_t screen_height = 0;

	float zNear = 0.01f;
	float zFar = 1000.0f;

	float fovy_degree = 70.0f; // degree

	float pitchRadians = 0.0f;
	float yawRadians = 0.0f;

	float movementSpeedMin = 0.01f;
	float movementSpeedMax = 60.0f;
	float movementSpeed = 0.01f;

	float rotationSpeedMin = 1.0f;
	float rotationSpeedMax = 100.0f;
	float rotationSpeed = 60.0f;
};

} // namespace ltracer
