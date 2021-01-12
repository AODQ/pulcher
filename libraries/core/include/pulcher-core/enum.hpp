#pragma once

namespace pul::core {
  enum class PickupType {
    HealthLarge, HealthMedium, HealthSmall
  , ArmorLarge,  ArmorMedium,  ArmorSmall
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
