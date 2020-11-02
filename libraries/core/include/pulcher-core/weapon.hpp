#pragma once

#include <pulcher-core/enum.hpp>
#include <pulcher-util/enum.hpp>

#include <array>
#include <variant>

namespace pul::core {

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

    struct WiBadFetus { };
    struct WiDopplerBeam { };
    struct WiGrannibal { };
    struct WiManshredder { };
    struct WiPericaliya { };
    struct WiPMF { };
    struct WiUnarmed { };
    struct WiVolnias {
      float primaryChargeupTimer = 0.0f;
      uint8_t secondaryChargedShots = 0;
      bool overchargedSecondary = false;
      bool hasChargedPrimary = false;
      float secondaryChargeupTimer = 0.0f;
      bool dischargingSecondary = false;
      float dischargingTimer = 0.0f;
    };
    struct WiWallbanger { };
    struct WiZeusStinger { };

    std::variant<
      WiBadFetus, WiDopplerBeam, WiGrannibal, WiManshredder
    , WiPericaliya, WiPMF, WiUnarmed, WiVolnias, WiWallbanger
    , WiZeusStinger
    > info {};

    bool pickedUp = false;
    int16_t cooldown = 0;
    uint16_t ammunition = 0;
  };

  struct Inventory {
    std::array<WeaponInfo, Idx(WeaponType::Size)> weapons {{
      { WeaponType::BadFetus,    WeaponInfo::WiBadFetus{} }
    , { WeaponType::DopplerBeam, WeaponInfo::WiDopplerBeam{} }
    , { WeaponType::Grannibal,   WeaponInfo::WiGrannibal{} }
    , { WeaponType::Manshredder, WeaponInfo::WiManshredder{} }
    , { WeaponType::Pericaliya,  WeaponInfo::WiPericaliya{} }
    , { WeaponType::PMF,         WeaponInfo::WiPMF{} }
    , { WeaponType::Unarmed,     WeaponInfo::WiUnarmed{}, true }
    , { WeaponType::Volnias,     WeaponInfo::WiVolnias{} }
    , { WeaponType::Wallbanger,  WeaponInfo::WiWallbanger{} }
    , { WeaponType::ZeusStinger, WeaponInfo::WiZeusStinger{} }
    }};

    WeaponType
      currentWeapon = WeaponType::Unarmed
    , previousWeapon = WeaponType::Unarmed
    ;

    void ChangeWeapon(WeaponType);
  };
}

char const * ToStr(pul::core::WeaponType weaponType);
