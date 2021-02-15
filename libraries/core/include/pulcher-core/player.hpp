#pragma once

#include <pulcher-core/weapon.hpp>
#include <pulcher-util/consts.hpp>

#include <entt/entt.hpp>

#include <array>
#include <vector>

namespace pul::core {

  struct DamageInfo {
    glm::vec2 directionForce;
    int32_t damage;
  };

  struct ComponentDamageable {
    int16_t health = 100; // 2^16 to avoid (200+100) overflow
    int16_t armor = 0;

    std::vector<DamageInfo> frameDamageInfos = {};
  };

  struct ComponentOrigin {
    glm::vec2 origin = {};
  };

  struct ComponentHitboxAABB {
    glm::i32vec2 dimensions;
    glm::i32vec2 offset = glm::vec2(0);
  };

  struct ComponentPlayer {
    glm::vec2 velocity = {};
    glm::vec2 storedVelocity = {};
    glm::vec2 prevOrigin = {};
    float lookAtAngle = 0.0f;
    bool flip = false;

    bool affectedByGravity = true;

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
    bool grounded = false, prevGrounded = false;
    bool wallClingLeft = false, wallClingRight = false;
    bool prevWallClingLeft = false, prevWallClingRight = false;
    bool landing = false; // landing until animation timer ends

    std::array<float, Idx(pul::Direction::Size)> dashCooldown { 0.0f };
    std::array<bool, Idx(pul::Direction::Size)> dashLock { false };

    // consider having previous frame info too?

    pul::core::Inventory inventory;

    entt::entity weaponAnimation;
  };

  struct PlayerMetaInfo {
    std::vector<glm::i32vec2> playerSpawnPoints = {};
  };

  struct ComponentLabel {
    std::string label = {};
  };

  struct ComponentPlayerControllable { };
  struct ComponentBotControllable { };

  struct ComponentCamera {
  };

}
