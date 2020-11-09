#pragma once

namespace pul::animation { struct Instance; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct SceneBundle; }
namespace pul::core { struct WeaponInfo; }
namespace pul::plugin { struct Info; }

namespace plugin::entity {
  void PlayerFireVolnias(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireVolniasPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireVolniasSecondary(
    uint8_t shots, uint8_t shotSet
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireGrannibal(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireGrannibalPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireGrannibalSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireDopplerBeam(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireDopplerBeamPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireDopplerBeamSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFirePericaliya(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FirePericaliyaPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FirePericaliyaSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireZeusStinger(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireZeusStingerPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireZeusStingerSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireBadFetus(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireBadFetusPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireBadFetusSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFirePMF(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FirePMFPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FirePMFSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireUnarmed(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireUnarmedPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireUnarmedSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireManshredder(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireManshredderPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  );
  void FireManshredderSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );

  void PlayerFireWallbanger(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireWallbangerPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
  void FireWallbangerSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  );
}
