#pragma once

#include <pulcher-animation/animation.hpp>

#include <entt/entt.hpp>

#include <functional>
#include <string>

// TODO rename this from particle to projectile

namespace pul::core {

  struct ProjectileDamageInfo {
    bool damagePlayer = false;
    entt::entity ignoredPlayer = entt::null;
    float explosionRadius = 0.0f;
    float explosionForce = 0.0f;
    int32_t playerDirectDamage = 0;
    int32_t playerSplashDamage = 0;
  };

  struct ComponentParticle {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    bool physicsBound = false;
    bool gravityAffected = false;

    std::function<void(glm::vec2 & velocity)> velocityModifierFn = {};
  };

  struct ComponentHitscanProjectile {
    pul::animation::Instance * playerAnimation = nullptr;

    std::function<bool(ComponentHitscanProjectile &)> update = {};
  };

  struct ComponentParticleGrenade {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    float velocityFriction = 1.0f;

    bool gravityAffected = false;

    bool useBounces = false;
    uint8_t bounces = 0;

    float timer = 50000.0f; // ms

    pul::animation::Instance animationInstance = {};
    std::string bounceAnimation = "";

    ProjectileDamageInfo damage = {};
  };

  // emits particle based on a distance
  struct ComponentDistanceParticleEmitter {
    pul::animation::Instance animationInstance;

    glm::vec2 velocity = glm::vec2();
    glm::vec2 prevOrigin = {};
    float originDist = 1.0f;

    float distanceTravelled = 0.0f;
  };

  struct ComponentParticleBeam {
    // returns true if entity should be destroyed
    std::function<
      bool(pul::animation::Instance & animInstance, float & cooldown)
    > update;

    float hitCooldown = 0.0f;
  };

  struct ComponentParticleExploder {
    bool explodeOnDelete = false;
    bool explodeOnCollide = false;
    pul::animation::Instance animationInstance;
    bool * audioTrigger = nullptr; // really silly

    ProjectileDamageInfo damage = {};
  };
}
