#pragma once

namespace entt { enum class entity : uint32_t; }
namespace pul::core { struct ComponentCreatureLump; }
namespace pul::core { struct ComponentCreatureMoldWing; }
namespace pul::core { struct ComponentCreatureVapivara; }
namespace pul::core { struct SceneBundle; }

namespace plugin::bot {
  void UpdateCreatureLump(
    pul::core::SceneBundle & scene
  , entt::entity entity
  );

  void UpdateCreatureMoldWing(
    pul::core::SceneBundle & scene
  , entt::entity entity
  );

  void UpdateCreatureVapivara(
    pul::core::SceneBundle & scene
  , entt::entity entity
  );

  void ImGuiCreatureVapivara(
    pul::core::SceneBundle & scene
  , entt::entity entity
  );
}
