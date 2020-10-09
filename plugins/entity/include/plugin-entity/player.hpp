#pragma once

namespace pulcher::animation { struct ComponentInstance; }
namespace pulcher::core { struct ComponentPlayer; }
namespace pulcher::core { struct SceneBundle; }
namespace pulcher::plugin { struct Info; }

namespace plugin::entity {
  void UpdatePlayer(
    pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
  , pulcher::core::ComponentPlayer & player
  , pulcher::animation::ComponentInstance & playerAnimation
  );
}
