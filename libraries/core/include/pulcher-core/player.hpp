#pragma once

#include <pulcher-core/weapon.hpp>

#include <entt/entt.hpp>

namespace pulcher::core {
  struct ComponentPlayer {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};
    glm::vec2 acceleration = {};

    bool sleeping = false;

    bool jumping = false;
    bool grounded = false;

    pulcher::core::Inventory inventory;

    glm::vec2 CalculateProjectedOrigin() {
      return origin + velocity;
    }

    entt::entity weaponAnimation;
  };
}
