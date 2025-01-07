#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/quaternion.hpp"

namespace ltracer {

struct Transform {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation = glm::quat();
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  glm::vec3 getForward() const {
    return glm::normalize(glm::rotate(rotation, glm::vec3(0, 0, -1)));
  }

  glm::vec3 getUp() const { //
    return glm::normalize(glm::rotate(rotation, glm::vec3(0, 1, 0)));
  }

  glm::vec3 getRight() const {
    return glm::normalize(glm::rotate(rotation, glm::vec3(1, 0, 0)));
  }
};

} // namespace ltracer
