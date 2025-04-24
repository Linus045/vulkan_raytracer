#pragma once

namespace tracer
{
// forward declarations
class Window;
class Renderer;
class Camera;

namespace ui
{
struct UIData;
}

namespace rt
{

void registerButtonFunctions(Window& window,
                             Renderer& renderer,
                             Camera& camera,
                             ui::UIData& uiData);
}
} // namespace tracer
