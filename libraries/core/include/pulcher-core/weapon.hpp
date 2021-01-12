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

  // must be orderd by WeaponType
  std::array<ImmutableWeaponInfo, Idx(WeaponType::Size)> constexpr weaponInfo {{
    { WeaponType::Manshredder, 1 }
  , { WeaponType::DopplerBeam, 2 }
  , { WeaponType::Volnias,     2 }
  , { WeaponType::Grannibal,   1 }
  , { WeaponType::ZeusStinger, 1 }
  , { WeaponType::BadFetus,    1 }
  , { WeaponType::Pericaliya,  0 }
  , { WeaponType::Wallbanger,  2 }
  , { WeaponType::PMF,         1 }
  , { WeaponType::Unarmed,     2 }
  }};

  struct WeaponInfo {
    WeaponType type;

    struct WiBadFetus {
      float dischargingTimer = 0.0f;
      bool primaryActive = false;
    };
    struct WiDopplerBeam {
      float dischargingTimer = 0.0f;
    };
    struct WiGrannibal {
      float dischargingTimer = 0.0f;
      float primaryMuzzleTrailTimer = 0.0f;
      uint8_t primaryMuzzleTrailLeft = 0u;
    };
    struct WiManshredder {
      float dischargingTimer = 0.0f;
      bool isPrimaryActive = false;
    };
    struct WiPericaliya {
      float dischargingTimer = 0.0f;
      bool isPrimaryActive = false;
      bool isSecondaryActive = false;
    };
    struct WiPMF {
      float dischargingTimer = 0.0f;
    };
    struct WiUnarmed {
      float dischargingTimer = 0.0f;
    };
    struct WiVolnias {
      float primaryChargeupTimer = 0.0f;
      uint8_t secondaryChargedShots = 0;
      bool overchargedSecondary = false;
      bool hasChargedPrimary = false;
      float secondaryChargeupTimer = 0.0f;
      bool dischargingSecondary = false;
      float dischargingTimer = 0.0f;
    };
    struct WiWallbanger {
      float dischargingTimer = 0.0f;
    };
    struct WiZeusStinger {
      float dischargingTimer = 0.0f;
    };

    // Must be ordered by WeaponType
    std::variant<
      WiManshredder
    , WiDopplerBeam
    , WiVolnias
    , WiGrannibal
    , WiZeusStinger
    , WiBadFetus
    , WiPericaliya
    , WiWallbanger
    , WiPMF
    , WiUnarmed
    > info { WiUnarmed{} };

    bool pickedUp = false;
    float cooldown = 0;
    uint16_t ammunition = 0;
  };

  struct Inventory {
    // Must be ordered by WeaponType
    std::array<WeaponInfo, Idx(WeaponType::Size)> weapons {{
      { WeaponType::Manshredder, WeaponInfo::WiManshredder{} }
    , { WeaponType::DopplerBeam, WeaponInfo::WiDopplerBeam{} }
    , { WeaponType::Volnias,     WeaponInfo::WiVolnias{}     }
    , { WeaponType::Grannibal,   WeaponInfo::WiGrannibal{}   }
    , { WeaponType::ZeusStinger, WeaponInfo::WiZeusStinger{} }
    , { WeaponType::BadFetus,    WeaponInfo::WiBadFetus{}    }
    , { WeaponType::Pericaliya,  WeaponInfo::WiPericaliya{}  }
    , { WeaponType::Wallbanger,  WeaponInfo::WiWallbanger{}  }
    , { WeaponType::PMF,         WeaponInfo::WiPMF{}         }
    , { WeaponType::Unarmed,     WeaponInfo::WiUnarmed{},    true }
    }};

    WeaponType
      currentWeapon = WeaponType::Unarmed
    , previousWeapon = WeaponType::Unarmed
    ;

    void ChangeWeapon(WeaponType);
    void ChangeWeapon(int32_t offset);
  };
}

char const * ToStr(pul::core::WeaponType weaponType);
