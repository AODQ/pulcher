#pragma once

namespace pul::animation { struct ComponentInstance; }
namespace pul::controls { struct Controller; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct ComponentHitboxAABB; }
namespace pul::core { struct SceneBundle; }
namespace pul::core { struct ComponentDamageable; }
namespace pul::plugin { struct Info; }
namespace entt { enum class entity: uint32_t; }

namespace plugin::entity {

  void ConstructPlayer(
    entt::entity & entityOut
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , bool mainPlayer = false
  );

  void UpdatePlayer(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::controls::Controller const & controller
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::core::ComponentHitboxAABB & hitboxAabb
  , pul::animation::ComponentInstance & playerAnimation
  , pul::core::ComponentDamageable & damageable
  );

  void UiRenderPlayer(
    pul::core::SceneBundle & scene
  , pul::core::ComponentPlayer & player
  , pul::animation::ComponentInstance & playerAnimation
  );
}
