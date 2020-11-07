#pragma once

#include <pulcher-animation/animation.hpp>

#include <functional>
#include <string>

namespace pul::core {

  struct ComponentParticle {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    bool physicsBound = false;
    bool gravityAffected = false;

    std::function<void(glm::vec2 & velocity)> velocityModifierFn = {};
    std::function<void()> destroyerFn = {};
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
    std::function<bool(pul::animation::Instance & animInstance)> update;
  };

  struct ComponentParticleExploder {
    bool explodeOnDelete = false;
    bool explodeOnCollide = false;
    bool damagePlayer = true;
    pul::animation::Instance animationInstance;
    bool * audioTrigger = nullptr; // really silly
  };
}
