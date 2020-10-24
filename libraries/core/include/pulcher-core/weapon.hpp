#pragma once

#include <pulcher-util/enum.hpp>

#include <array>

namespace pul::core {

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

  struct ImmutableWeaponInfo {
    WeaponType type;
    size_t requiredHands = 0;
  };

  std::array<ImmutableWeaponInfo, Idx(WeaponType::Size)> constexpr weaponInfo {{
    { WeaponType::BadFetus, 1 }
  , { WeaponType::DopplerBeam, 2 }
  , { WeaponType::Grannibal, 2 }
  , { WeaponType::Manshredder, 1 }
  , { WeaponType::Pericaliya, 1 }
  , { WeaponType::PMF, 1 }
  , { WeaponType::Unarmed, 0 }
  , { WeaponType::Volnias, 2 }
  , { WeaponType::Wallbanger, 1 }
  , { WeaponType::ZeusStinger, 2 }
  }};

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

char const * ToStr(pul::core::WeaponType weaponType);
