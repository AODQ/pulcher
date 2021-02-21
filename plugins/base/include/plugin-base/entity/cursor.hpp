#pragma once

namespace pul::core { struct RenderBundleInstance; }
namespace pul::core { struct SceneBundle; }

namespace plugin::entity {
  void ConstructCursor(pul::core::SceneBundle & scene);

  void RenderCursor(
    pul::core::SceneBundle const & scene
  , pul::core::RenderBundleInstance const & renderBundle
  );
}
