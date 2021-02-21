#pragma once

#include <glm/fwd.hpp>

#include <span>
#include <string>
#include <vector>
#include <filesystem>

namespace pul::core { struct RenderBundleInstance; }
namespace pul::core { struct SceneBundle; }
namespace pul::plugin { struct Info; }

namespace pul::plugin {
  struct Info {
    void (*UpdateRenderBundleInstance)(
      pul::core::SceneBundle & scene
    , pul::core::RenderBundleInstance & instance
    );

    void (*Interpolate)(
      float const msDeltaInterp
    , pul::core::RenderBundleInstance const & previousBundle
    , pul::core::RenderBundleInstance const & currentBundle
    , pul::core::RenderBundleInstance & outputBundle
    );

    void (*RenderInterpolated)(
      pul::core::SceneBundle const & scene
    , pul::core::RenderBundleInstance const & interpolatedBundle
    );

    void (*DebugUiDispatch)(pul::core::SceneBundle const & scene);

    void (*LogicUpdate)(pul::core::SceneBundle & scene);

    void (*Initialize)(pul::core::SceneBundle & scene);
    void (*LoadMap)(pul::core::SceneBundle & scene, char const * mapPath);

    void (*Shutdown)(pul::core::SceneBundle & scene);
  };

  bool LoadPlugin(
    pul::plugin::Info & plugin
  , std::filesystem::path const & filename
  );

  // TODO load mod plugins

  void FreePlugins();

  // check if plugins need to be reloaded
  void UpdatePlugins(Info & plugin);
}
