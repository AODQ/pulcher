#pragma once

namespace pul::core { struct SceneBundle; }
namespace pul::core { struct RenderBundleInstance; }

namespace plugin::map {
  void LoadMap(
    pul::core::SceneBundle & scene
  , char const * filename
  );
  void Shutdown();
  void DebugUiDispatch(pul::core::SceneBundle & scene);
  void Render(
    pul::core::SceneBundle const & scene
  , pul::core::RenderBundleInstance const & renderBundle
  );
}
