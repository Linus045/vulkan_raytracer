#pragma once

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

#include <cassert>
#include <cstdint>

#include "transform.hpp"

namespace ltracer {

class Camera {
public:
  Transform transform;

  Camera() {
    transform.position = glm::vec3(1.5, 3.5, 6);
    glm::vec3 up = globalUp;
    transform.rotation = glm::quatLookAt(
        glm::normalize(glm::vec3(1.5, 3.5, 0) - transform.position), up);

    updateViewMatrix();
  }

  ~Camera() = default;

  Camera(const Camera &) = delete;
  Camera(const Camera &&) = delete;
  Camera &operator=(const Camera &) = delete;

  glm::mat4 getProjectionMatrix() const { return projectionMatrix; }

  glm::mat4 getViewMatrix() const { return viewMatrix; }

  void updateScreenSize(const uint32_t width, const uint32_t height) {
    screen_width = width;
    screen_height = height;
    updateProjectionMatrix();
  }

  void translate(glm::vec3 translation) {
    transform.position += translation;
    updateViewMatrix();
    cameraMoved = true;
  }

  void translate(float x, float y, float z) { translate(glm::vec3(x, y, z)); }

  // TODO: figure out what the difference is ?!
  void rotateBROKEN(const glm::vec3 &axis, const float rotation) {
    transform.rotation = glm::rotate(transform.rotation, rotation, axis);
    updateViewMatrix();
    cameraMoved = true;
  }

  void rotate(const glm::vec3 &axis, const float angleDegree) {
    // Calculate the new alignment
    glm::quat q =
        glm::angleAxis(glm::radians(angleDegree), glm::normalize(axis));
    transform.rotation = glm::normalize(q * transform.rotation);
    updateViewMatrix();
    cameraMoved = true;
  }

  void limitPitch() {
    glm::vec3 eulerAngles = glm::eulerAngles(
        transform.rotation); // Returns a vec3 (pitch, yaw, roll)
    eulerAngles.x = glm::clamp(eulerAngles.x, -glm::radians(pitchLimit_degree),
                               glm::radians(pitchLimit_degree));
    transform.rotation = glm::quat(eulerAngles);

    updateViewMatrix();
    cameraMoved = true;
  }

  bool isCameraMoved() const { return cameraMoved; }

  void resetCameraMoved() { cameraMoved = false; }

  const glm::vec3 globalUp = {0, 1, 0};

private:
  float pitchLimit_degree = 89.0f; // Limit for up/down rotation (in degrees)
  bool cameraMoved = false;

  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrix;

  uint32_t screen_width = 0;
  uint32_t screen_height = 0;

  float zNear = 0.01;
  float zFar = 1000.0f;

  float fovy_degree = 70.0f; // degree

  void updateProjectionMatrix() {
    assert(screen_width != 0);
    assert(screen_height != 0);

    // float aspect = screen_width / (float)screen_height;
    // float fov_x = atan(tan(90.0f / 2.0f) * aspect) * 2.0f;

    projectionMatrix =
        glm::perspectiveFovRH_ZO(glm::radians(fovy_degree), (float)screen_width,
                                 (float)screen_height, zNear, zFar);
    projectionMatrix[1][1] *= -1.0f;

    // logMat4("projectionMatrix", projectionMatrix);
  }

  void updateViewMatrix() {
    glm::vec3 center = transform.position + transform.getForward() * 10.0f;
    viewMatrix = glm::lookAt(transform.position, center, transform.getUp());
  }
};

} // namespace ltracer
