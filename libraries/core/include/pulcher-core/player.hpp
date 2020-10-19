#pragma once

#include <pulcher-core/weapon.hpp>

#include <entt/entt.hpp>

namespace pulcher::core {
  struct ComponentPlayer {
    glm::vec2 origin = {902.0f, 1120.0f};
    glm::vec2 velocity = {};
    glm::vec2 storedVelocity = {};

    float dashCooldown = 0.0f;
    float jumpFallTime = 0.0f;
    int32_t midairDashesLeft = 0;

    bool jumping = false;
    bool crouching = false;
    bool hasReleasedJump = false;
    bool grounded = false;
    bool wallClingLeft = false, wallClingRight = false;
    bool landing = false; // landing until animation timer ends

    // consider having previous frame info too?

    pulcher::core::Inventory inventory;

    glm::vec2 CalculateProjectedOrigin() {
      return origin + velocity;
    }

    entt::entity weaponAnimation;
  };
}
