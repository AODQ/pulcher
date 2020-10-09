#include <pulcher-core/weapon.hpp>

void pulcher::core::Inventory::ChangeWeapon(pulcher::core::WeaponType type) {
  auto & current = weapons[Idx(currentWeapon)];
  auto & requested = weapons[Idx(type)];

  // check if it is even possible; must be picked up, must have ammo for it,
  // and current weapon must not have any cooldown. The latter might be
  // buffered in the future
  if (
      type != WeaponType::Unarmed
   && (
          !requested.pickedUp || requested.ammunition == 0
       || current.cooldown > 0
      )
  ) {
    return;
  }

  // save previous weapon
  previousWeapon = currentWeapon;

  // if requested same type, switch to unarmed, otherwise switch to type
  if (type == currentWeapon) {
    currentWeapon = WeaponType::Unarmed;
  } else {
    currentWeapon = type;
  }
}

char const * ToStr(pulcher::core::WeaponType weaponType) {
  switch (weaponType) {
    default: return "N/A";
    case pulcher::core::WeaponType::BadFetus:    return "bad-fetus";
    case pulcher::core::WeaponType::DopplerBeam: return "doppler-beam";
    case pulcher::core::WeaponType::Grannibal:   return "grannibal";
    case pulcher::core::WeaponType::Manshredder: return "manshredder";
    case pulcher::core::WeaponType::Pericaliya:  return "pericaliya";
    case pulcher::core::WeaponType::PMF:         return "pmf";
    case pulcher::core::WeaponType::Unarmed:     return "unarmed";
    case pulcher::core::WeaponType::Volnias:     return "volnias";
    case pulcher::core::WeaponType::Wallbanger:  return "wallbanger";
    case pulcher::core::WeaponType::ZeusStinger: return "zeus-stinger";
  }
}


