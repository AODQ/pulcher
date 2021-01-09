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
  , entt::entity playerEntity
  );
  void FireVolniasPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireVolniasSecondary(
    uint8_t shots, uint8_t shotSet
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireGrannibal(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireGrannibalPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireGrannibalSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireDopplerBeam(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireDopplerBeamPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireDopplerBeamSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFirePericaliya(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePericaliyaPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePericaliyaSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
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
  , entt::entity playerEntity
  );
  void FireZeusStingerPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireZeusStingerSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireBadFetus(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireBadFetusPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireBadFetusSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFirePMF(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePMFPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePMFSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireUnarmed(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireUnarmedPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireUnarmedSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireManshredder(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireManshredderPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireManshredderSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireWallbanger(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireWallbangerPrimary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireWallbangerSecondary(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  struct WeaponDamageRaycastReturnInfo {
    entt::entity entity = entt::null;
    glm::vec2 origin = {};
  };

  WeaponDamageRaycastReturnInfo WeaponDamageRaycast(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & originBegin, glm::vec2 const & originEnd
  , float const damage, float const force
  , entt::entity playerEntity
  );

  // ignoreEntity - can be null, describes which entity to be ignored
  bool WeaponDamageCircle(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , glm::vec2 const & origin, float const radius
  , float const damage, float const force
  , entt::entity const ignoredEntity
  );
}
