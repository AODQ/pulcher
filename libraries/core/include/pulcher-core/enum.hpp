#pragma once

namespace pul::core {
  enum class PickupType {
    Ammo
  , ArmorLarge,  ArmorMedium,  ArmorSmall
  , HealthLarge, HealthMedium, HealthSmall
  , Powerup
  , Weapon, WeaponAll
  , Size
  };

  // ordered by preferred "weapon switch/swap"
  enum class WeaponType {
    Manshredder
  , DopplerBeam
  , Volnias
  , Grannibal
  , ZeusStinger
  , BadFetus
  , Pericaliya
  , Wallbanger
  , PMF
  , Unarmed
  , Size
  };
}
