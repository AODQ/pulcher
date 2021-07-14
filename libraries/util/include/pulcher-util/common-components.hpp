#pragma once

#include <glm/fwd.hpp>

namespace pul::util {

  struct ComponentOrigin {
    glm::vec2 origin = {};
  };

  struct ComponentHitboxAABB {
    glm::i32vec2 dimensions;
    glm::i32vec2 offset = glm::vec2(0);
  };

}
