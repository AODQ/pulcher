#pragma once

#include <glm/fwd.hpp>

namespace pul::util {

  struct ComponentOrigin {
    glm::vec2 origin = {};
  };

  enum class HitboxTag : uint32_t {
    Invalid    = 0b000,
    Damager    = 0b001,
    Damageable = 0b010,
    Collision  = 0b100,
    Size       = 4,
  };

  struct ComponentHitboxAABB {
    glm::i32vec2 dimensions;
    glm::i32vec2 offset = glm::vec2(0);
    HitboxTag tag = HitboxTag::Invalid;
  };
}

pul::util::HitboxTag operator|(
  pul::util::HitboxTag lhs, pul::util::HitboxTag rhs
);

pul::util::HitboxTag operator&(
  pul::util::HitboxTag lhs, pul::util::HitboxTag rhs
);

pul::util::HitboxTag operator&(
  pul::util::HitboxTag lhs, uint32_t rhs
);
