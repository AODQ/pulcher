#pragma once

namespace pul::animation { struct ComponentInstance; }
namespace pul::controls { struct Controller; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct SceneBundle; }
namespace pul::plugin { struct Info; }

namespace plugin::entity {
  void UpdatePlayer(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::controls::Controller const & controller
  , pul::core::ComponentPlayer & player
  , pul::animation::ComponentInstance & playerAnimation
  );

  void UiRenderPlayer(
    pul::core::SceneBundle & scene
  , pul::core::ComponentPlayer & player
  , pul::animation::ComponentInstance & playerAnimation
  );
}
