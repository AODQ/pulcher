#pragma once

namespace pul::core { struct SceneBundle; }
namespace pul::plugin { struct Info; }

namespace plugin::entity {
  void ConstructCursor(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  );

  void RenderCursor(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  );
}
