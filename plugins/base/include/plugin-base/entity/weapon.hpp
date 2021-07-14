#pragma once

namespace pul::animation { struct Instance; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct SceneBundle; }
namespace pul::core { struct WeaponInfo; }

namespace plugin::entity {
  void PlayerFireVolnias(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireVolniasPrimary(
    pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireVolniasSecondary(
    uint8_t shots, uint8_t shotSet
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireGrannibal(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireGrannibalPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireGrannibalSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireDopplerBeam(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireDopplerBeamPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireDopplerBeamSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFirePericaliya(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePericaliyaPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePericaliyaSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireZeusStinger(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireZeusStingerPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireZeusStingerSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireBadFetus(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireBadFetusPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireBadFetusSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFirePMF(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePMFPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FirePMFSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireUnarmed(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireUnarmedPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireUnarmedSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireManshredder(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireManshredderPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , pul::core::ComponentPlayer & player
  , glm::vec2 & playerOrigin
  , pul::animation::Instance & playerAnim
  , entt::entity playerEntity
  );
  void FireManshredderSecondary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );

  void PlayerFireWallbanger(
    bool const primary, bool const secondary
  , glm::vec2 & velocity
  , pul::core::WeaponInfo & weaponInfo
  , pul::core::SceneBundle & scene
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireWallbangerPrimary(
    pul::core::SceneBundle & scene
  , pul::core::WeaponInfo & weaponInfo
  , glm::vec2 const & origin, glm::vec2 const & direction, float const angle
  , bool const flip, glm::mat3 const & matrix
  , entt::entity playerEntity
  );
  void FireWallbangerSecondary(
    pul::core::SceneBundle & scene
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
    pul::core::SceneBundle & scene
  , glm::vec2 const & originBegin, glm::vec2 const & originEnd
  , float const damage, float const force
  , entt::entity playerEntity
  );

  // ignoreEntity - can be null, describes which entity to be ignored
  // TODO make this a more generic damage hitbox
  bool WeaponDamageCircle(
    pul::core::SceneBundle & scene
  , glm::vec2 const & origin, float const radius
  , float const damage, float const force
  , entt::entity const ignoredEntity
  , glm::vec2 const overrideTargetDamageDirection = glm::vec2(0.0f)
  );

  // TODO make a more generic hitbox structure that can be chained
  // (as in multiple hitboxes)
}
