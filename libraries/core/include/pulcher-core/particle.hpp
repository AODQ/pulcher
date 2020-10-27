#pragma once

namespace pul::core {

  struct ComponentParticle {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    bool physicsBound = false;
  };
}
