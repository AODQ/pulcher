#pragma once

#include <pulcher-animation/animation.hpp>

#include <string>

namespace pul::core {

  struct ComponentParticle {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    bool physicsBound = false;
  };

  struct ComponentParticleExploder {
    bool explodeOnDelete = false;
    bool explodeOnCollide = false;
    pul::animation::Instance animationInstance;
  };
}
