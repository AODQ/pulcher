#pragma once

#include <pulcher-core/enum.hpp>

#include <array>

namespace pul::core {


  // combined with animation
  struct ComponentPickup {
    pul::core::PickupType type;
    pul::core::WeaponType weaponType; // only valid if type == Weapon
    glm::vec2 origin;

    bool spawned;
    size_t spawnTimer;
    size_t spawnTimerSet = 5000ul;
  };
}
