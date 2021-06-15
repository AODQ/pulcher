#pragma once

namespace pul::core { struct ComponentCreatureLump; }
namespace pul::core { struct SceneBundle; }
namespace pul::animation { struct ComponentInstance; }

namespace plugin::bot {
  void UpdateCreatureLump(
    pul::core::SceneBundle & scene
  , pul::core::ComponentCreatureLump & self
  , pul::animation::ComponentInstance & animation
  );
}
