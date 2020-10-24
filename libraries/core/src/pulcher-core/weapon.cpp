#include <pulcher-core/weapon.hpp>

void pul::core::Inventory::ChangeWeapon(pul::core::WeaponType type) {
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

char const * ToStr(pul::core::WeaponType weaponType) {
  switch (weaponType) {
    default: return "N/A";
    case pul::core::WeaponType::BadFetus:    return "bad-fetus";
    case pul::core::WeaponType::DopplerBeam: return "doppler-beam";
    case pul::core::WeaponType::Grannibal:   return "grannibal";
    case pul::core::WeaponType::Manshredder: return "manshredder";
    case pul::core::WeaponType::Pericaliya:  return "pericaliya";
    case pul::core::WeaponType::PMF:         return "pmf";
    case pul::core::WeaponType::Unarmed:     return "unarmed";
    case pul::core::WeaponType::Volnias:     return "volnias";
    case pul::core::WeaponType::Wallbanger:  return "wallbanger";
    case pul::core::WeaponType::ZeusStinger: return "zeus-stinger";
  }
}
