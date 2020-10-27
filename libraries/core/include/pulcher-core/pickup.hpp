#pragma once

#include <array>

namespace pul::core {

  enum class PickupType {
    HealthLarge, HealthMedium, HealthSmall
  , ArmorLarge,  ArmorMedium,  ArmorSmall
  , WeaponBadFetus, WeaponDopplerBeam
  , WeaponGrannibal, WeaponManshredder
  , WeaponPericaliya, WeaponPMF
  , WeaponVolnias, WeaponWallbanger
  , WeaponZeusStinger
  , WeaponAll
  , Size
  };

  // combined with animation
  struct ComponentPickup {
    PickupType type;
    glm::vec2 origin;

    bool spawned;
    size_t spawnTimer;
    size_t spawnTimerSet = 5000ul;
  };
}
