#pragma once

#include <pulcher-core/weapon.hpp>

#include <entt/entt.hpp>

namespace pulcher::core {
  struct ComponentPlayer {
    glm::vec2 origin = {902.0f, 1120.0f};
    glm::vec2 velocity = {};
    glm::vec2 storedVelocity = {};

    float dashCooldown = 0.0f;
    int32_t midairDashesLeft = 0;
    float runTimer = 0.0f;

    bool jumping = false;
    bool grounded = false;

    pulcher::core::Inventory inventory;

    glm::vec2 CalculateProjectedOrigin() {
      return origin + velocity;
    }

    entt::entity weaponAnimation;
  };
}
