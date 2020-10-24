#pragma once

#include <pulcher-core/weapon.hpp>
#include <pulcher-util/consts.hpp>

#include <entt/entt.hpp>

#include <array>

namespace pul::core {
  struct ComponentPlayer {
    glm::vec2 origin = {902.0f, 1120.0f};
    glm::vec2 velocity = {};
    glm::vec2 storedVelocity = {};

    float jumpFallTime = 0.0f;
    float crouchSlideCooldown = 0.0f;
    float slideFrictionTime = 0.0f;
    float dashZeroGravityTime = 0.0f;
    int32_t midairDashesLeft = 0;

    pul::Direction facingDirection = pul::Direction::Left;

    bool jumping = false;
    bool crouchSliding = false;
    bool crouching = false;
    bool hasReleasedJump = false;
    bool grounded = false;
    bool wallClingLeft = false, wallClingRight = false;
    bool landing = false; // landing until animation timer ends

    std::array<float, Idx(pul::Direction::Size)> dashCooldown { 0.0f };
    std::array<bool, Idx(pul::Direction::Size)> dashLock { false };

    // consider having previous frame info too?

    pul::core::Inventory inventory;

    glm::vec2 CalculateProjectedOrigin() {
      return origin + velocity;
    }

    entt::entity weaponAnimation;
  };
}
