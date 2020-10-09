#pragma once

#include <pulcher-core/weapon.hpp>

#include <entt/entt.hpp>

namespace pulcher::core {
  struct ComponentPlayer {
    glm::vec2 origin = {};
    glm::vec2 velocity = {};

    bool sleeping = false;

    bool jumping = false;

    pulcher::core::Inventory inventory;

    glm::vec2 CalculateProjectedOrigin() {
      return origin + velocity;
    }

    entt::entity weaponAnimation;
  };
}
