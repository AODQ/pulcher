#pragma once

#include <array>

namespace pul::core {

  enum class PickupType {
    HealthLarge, HealthMed, HealthSmall
  , ArmorLarge,  ArmorMed,  ArmorSmall
  , Size
  };

  // combined with animation
  struct ComponentPickup {
    PickupType type;
    glm::vec2 origin;

    bool spawned;
    size_t spawnTimer;
  };
}
