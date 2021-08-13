#pragma once

namespace pul::core { struct RenderBundleInstance; }
namespace pul::core { struct SceneBundle; }

namespace plugin::debug {
  void RenderPoint(glm::vec2 origin, glm::vec3 color);
  void RenderLine(glm::vec2 start, glm::vec2 end, glm::vec3 color);
  void RenderAabbByCorner(
    glm::vec2 upperLeft, glm::vec2 lowerRight, glm::vec3 color
  );
  void RenderAabbByCenter(glm::vec2 center, glm::vec2 bounds, glm::vec3 color);
  void RenderCircle(
    glm::vec2 const origin,
    float const radius,
    glm::vec3 const color
  );

  void ShapesRenderInitialize();
  void ShapesRenderShutdown();

  void ShapesRender(
    pul::core::SceneBundle const & scene
  , pul::core::RenderBundleInstance const & renderBundle
  );
  void ShapesRenderSwap();
  void ShapesUpdateSwap();
}
