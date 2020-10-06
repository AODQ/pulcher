#pragma once

#include <pulcher-util/enum.hpp>

#include <array>

namespace pulcher::core {

  enum class WeaponType {
    BadFetus
  , DopplerBeam
  , Grannibal
  , Manshredder
  , Pericaliya
  , PMF
  , Unarmed
  , Volnias
  , Wallbanger
  , ZeusStinger
  , Size
  };

  struct WeaponInfo {
    WeaponType type;
    bool pickedUp = false;
    int16_t cooldown = 0;
    uint16_t ammunition = 0;
  };

  struct Inventory {
    std::array<WeaponInfo, Idx(WeaponType::Size)> weapons {{
      { WeaponType::BadFetus }
    , { WeaponType::DopplerBeam }
    , { WeaponType::Grannibal }
    , { WeaponType::Manshredder }
    , { WeaponType::Pericaliya }
    , { WeaponType::PMF }
    , { WeaponType::Unarmed, true } // unarmed always is picked up
    , { WeaponType::Volnias }
    , { WeaponType::Wallbanger }
    , { WeaponType::ZeusStinger }
    }};

    WeaponType
      currentWeapon = WeaponType::Unarmed
    , previousWeapon = WeaponType::Unarmed
    ;

    void ChangeWeapon(WeaponType);
  };

}

char const * ToStr(pulcher::core::WeaponType weaponType);
