#pragma once

#include "worldobject.hpp"
#include <vector>
namespace ltracer
{
class Scene
{
  public:
	Scene() = default;
	~Scene() = default;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene(Scene&&) noexcept = delete;
	Scene& operator=(Scene&&) noexcept = delete;

  private:
	std::vector<WorldObject> worldObject;
};

} // namespace ltracer
