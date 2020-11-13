#pragma once

#include <pulcher-core/weapon.hpp>
#include <pulcher-util/consts.hpp>

#include <entt/entt.hpp>

#include <array>

namespace pul::core {

  struct ComponentDamageable {
    uint16_t health = 100u; // 2^16 to avoid (200+100) overflow
    uint16_t armor = 0u;
  };

  struct ComponentOrigin {
    glm::vec2 origin = {};
  };

  struct ComponentHitboxAABB {
    entt::entity entityOrigin { entt::null };

    glm::vec2 dimensions;
  };

  struct ComponentPlayer {
    glm::vec2 velocity = {};
    glm::vec2 storedVelocity = {};
    float lookAtAngle = 0.0f;
    bool flip = false;

    float prevAirVelocity = 0.0f;

    // used to mix w/ air acceleration when falling off a ledge
    float prevFrameGroundAcceleration = 0.0f;

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

    entt::entity weaponAnimation;
  };
}
