#pragma once

#include <pulcher-animation/animation.hpp>

#include <string>

namespace pul::core {

  struct ComponentParticle {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    bool physicsBound = false;
    bool gravityAffected = false;
  };

  struct ComponentParticleGrenade {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};

    uint8_t bounces = 0;

    pul::animation::Instance animationInstance;
  };

  struct ComponentParticleEmitter {
    pul::animation::Instance animationInstance;

    glm::vec2 velocity = glm::vec2();
    glm::vec2 prevOrigin = {};
    float originDist = 1.0f;
  };

  struct ComponentParticleExploder {
    bool explodeOnDelete = false;
    bool explodeOnCollide = false;
    pul::animation::Instance animationInstance;
    bool * audioTrigger = nullptr; // really silly
  };
}
