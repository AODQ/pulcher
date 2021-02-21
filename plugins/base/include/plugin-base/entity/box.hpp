#pragma once

namespace entt { enum class entity : uint32_t; }
namespace pul::core { struct SceneBundle; }

namespace plugin::entity {
  void ConstructDebugBox(
    entt::entity & entityOut
  , pul::core::SceneBundle & scene
  );

  struct DebugBox {
    glm::vec2 origin = {};
    glm::vec2 dimensions = {};
    glm::vec2 velocity = {};
    float angleVelocity = 0.0f;
    float angle = 1.5f/2.0f;
  };

  void BoxUpdate(pul::core::SceneBundle & scene);
}
