#include <pulcher-core/weapon.hpp>

#include <pulcher-util/log.hpp>

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

void pul::core::Inventory::ChangeWeapon(int32_t offset) {
  // clamp to a sensible amount
  offset =
    glm::clamp(
      offset,
      -Idx(pul::core::WeaponType::Size),
      +Idx(pul::core::WeaponType::Size)
    );

  // iterate to 0 (from - or +), manually to allow non-selectable weapons to be
  // skipped over
  while (glm::abs(offset) != 0) {
    auto const reqType = 
      static_cast<pul::core::WeaponType>(
          (
            Idx(currentWeapon)
          + glm::sign(offset)
          + Idx(pul::core::WeaponType::Size)
          )
        % Idx(pul::core::WeaponType::Size)
      );

    auto const & current = weapons[Idx(currentWeapon)];
    auto const & requested = weapons[Idx(reqType)];

    // iterate to next weapon now as it's no longer referenced

    // check if not possible (no ammo or not picked up), only if possible
    // update the offset
    if (
        reqType == WeaponType::Unarmed
     || (
            requested.pickedUp
         && requested.ammunition > 0
         && current.cooldown <= 0
        )
    ) {
      offset += -glm::sign(offset);
    }

    // save previous weapon, swap to current
    previousWeapon = currentWeapon;
    currentWeapon = reqType;
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
