#include <plugin-base/entity/player.hpp>

#include <plugin-base/entity/weapon.hpp>
#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/pickup.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>
#include <pulcher-util/math.hpp>

#include <entt/entt.hpp>
#include <imgui/imgui.hpp>

namespace {
  int32_t maxAirDashes = 3u;
  float inputRunAccelTarget = 5.0f;
  float inputRunAccelTime = 1.5f;
  float slideAccel = 1.0f;
  float slideMinVelocity = 6.0f;
  float slideCooldown   = 300.0f;
  float slideFriction = 0.95f;
  float slideFrictionTransitionTime = 500.0f;
  float slideFrictionTransitionPow = 1.0f;
  float slopeStepUpHeight   = 12.0f;
  float slopeStepDownHeight = 12.0f;

  float inputAirAccelPreThresholdTime = 0.8f;
  float inputAirAccelPostThreshold = 0.02f;
  float inputAirAccelThreshold = 6.0f;

  float inputGravityAccelPreThresholdTime = 1.0f;
  float inputGravityAccelPostThreshold = 0.04f;
  float inputGravityAccelThreshold = 8.0f;

  float inputWalkAccelTarget = 1.0f;
  float inputWalkAccelTime = 0.5f;
  float inputCrouchAccelTarget = 2.0f;
  float inputCrouchAccelTime = 1.0f;
  float jumpAfterFallTime = 200.0f;
  float jumpingHorizontalAccel = 4.0f;
  float jumpingHorizontalAccelMax = 6.0f;
  float jumpingVerticalAccel = 7.0f;
  float jumpingHorizontalTheta = 90.0f;
  float frictionGrounded = 0.8f;
  float dashAccel = 1.0f;
  float dashGravityTime = 200.0f;

  struct TransferPercent {
    float same = 1.0f;
    float reverse = 1.0f;
    float degree90 = 0.75f;
  };
  TransferPercent dashVerticalTransfer;
  TransferPercent dashHorizontalTransfer;
  float dashMinVelocity = 6.0f;
  float dashMinVelocityMultiplier = 2.0f;
  float dashCooldown = 1500.0f;
  float horizontalGroundedVelocityStop = 0.5f;


  std::string debugTransferDetails = {};


/*
  calculate requested dash transfer velocity given a direction and a velocity.
  These must in the range (-1 .. +1). This basically applies some amount of
  velocity to be reduced dependent on the player direction, which is in
  relation to the total magnitude of the velocity. As there are two seperate
  dashes (horizontal and vertical), with some mixture allowed between them, if
  a dash is occuring upwards then vertical would be used. For example, if the
  player is moving right quickly and dashes upwards, it would use
  dashVertical.90degree. However if the player is moving up-right at
  approximately the same velocity, the 90 degree would only apply if they dash
  up-left and down-right; left and right would be considered mixtures of
  90degree and same.

  uses horizontal
                      90 degree    same+90
                         |        /
                         |    /
                         | /
  <<< reverse  ----------*----------- > same       | >>>>>>>>
                         |
                         |
                         |
                      90 degree

  uses (horizontal+vertical)*0.5
            reverse                 same
               \      90d+same  ^^^^
                  \      |       /
                    \    |    /
                      \  | /
  <<< rev+90d  ----------*----------- > 90 degree + same
                         |
                         |
                         |
                      90 degree
*/
float CalculateDashTransferVelocity(
  glm::i32vec2 const direction, glm::i32vec2 const velocity
) {
  PUL_ASSERT_CMP(direction, !=, glm::i32vec2(0), return 0.0f;)

  auto & transferV = dashVerticalTransfer;
  auto & transferH = dashHorizontalTransfer;

  float const angleDirection = std::atan2(direction.y, direction.x);
  float const angleVelocity = std::atan2(velocity.y, velocity.x);

  bool directionAxisX = false, directionAxisY = false;

  directionAxisY = direction.y != 0.0f;
  directionAxisX = direction.x != 0.0f;

  float const angleDiff =
    glm::abs(glm::abs(angleDirection) - glm::abs(angleVelocity));
  float angleSum =
    glm::abs(glm::abs(angleDirection) + glm::abs(angleVelocity));

  if (angleSum >= pul::Pi) { angleSum -= pul::Pi; }

  // if same angle, then just use the axis
  if (angleDirection == angleVelocity) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("same angle | X+Y");
      return transferV.same*0.5f + transferH.same*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("same angle | X");
      return transferH.same;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("same angle | Y");
      return transferV.same;
    }
  }

  // if in 90 degree range (not mix)
  if (
      glm::abs(angleSum - pul::Pi_2) < 0.1f
   || glm::abs(angleDiff - pul::Pi_2) < 0.1f
  ) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("90 degree angle | X+Y");
      return transferV.degree90*0.5f + transferH.degree90*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("90 degree angle | X");
      return transferH.degree90;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("90 degree angle | Y");
      return transferV.degree90;
    }
  }

  // if in between 90 degree range & same angle (45 degrees)
  if (glm::abs(angleDiff - pul::Pi_4) < 0.1f) {
    float const transferVMix = transferV.degree90*0.5f + transferV.same*0.5f;
    float const transferHMix = transferH.degree90*0.5f + transferH.same*0.5f;
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("45 degree angle | X+Y");
      return transferVMix*0.5f + transferHMix*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("45 degree angle | X");
      return transferHMix;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("45 degree angle | Y");
      return transferVMix;
    }
  }

  // if reverse angle
  if (
    glm::abs(glm::abs(angleDirection)+glm::abs(angleVelocity) - pul::Pi) < 0.1f
  ) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse angle | X+Y");
      return transferV.reverse*0.5f + transferH.reverse*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("reverse angle | X");
      return transferH.reverse;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse angle | Y");
      return transferV.reverse;
    }
  }

  // if in between 90 degree range & reverse angle (45 degrees)
  if (glm::abs(angleDiff - (pul::Pi_2+pul::Pi_4)) < 0.1f) {
    float const transferVMix = transferV.degree90*0.5f + transferV.reverse*0.5f;
    float const transferHMix = transferV.degree90*0.5f + transferV.reverse*0.5f;
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | X+Y");
      return transferVMix*0.5f + transferHMix*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | X");
      return transferHMix;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | Y");
      return transferVMix;
    }
  }

  spdlog::critical(
    "could not find an appropiate angle; dir {} vel {} | diff {} sum {}"
  , angleDirection, angleVelocity, angleDiff, angleSum
  );
  return 0.0f;
}

// returns a normalized vector of the velocity, however it is only relative to
// the axis. That is, a value can only be -1, 0, or 1. To normalize velocity,
// we basically want to deprecate any velocity that is not significant; ei for
// vector <2000, 100> we only want to return <1, 0>
glm::i32vec2 NormalizeVelocityAxis(glm::vec2 const & velocity) {
  auto v = glm::normalize(velocity);
  return glm::i32vec2(glm::round(v.x), glm::round(v.y));
}

float CalculateAccelFromTarget(float timeMs, float targetTexels) {
  return targetTexels / timeMs / 90.0f * 2.0f;
}

void ApplyGroundedMovement(
  float const facingDirection, float const playerVelocityX
, float const accelTime, float const accelTarget
, bool & inoutFrictionApplies, float & inoutInputAccel
) {
  inoutInputAccel *= CalculateAccelFromTarget(accelTime, accelTarget);

  // check if we have not reached the target, in which case no
  // friction is applied. However if this frame would reach the target,
  // limit the capacity; which is why we use `<=` check instead of `<`
  if (
      glm::abs(playerVelocityX) <= accelTarget
   && (
        playerVelocityX == 0.0f
          ? true : glm::sign(playerVelocityX) == glm::sign(facingDirection)
      )
  ) {
    inoutFrictionApplies = false;
    if (facingDirection*(playerVelocityX + inoutInputAccel) > accelTarget) {
      inoutInputAccel = facingDirection*accelTarget - playerVelocityX;
    }
  }
}

void PlayerCheckPickups(
  pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, pul::core::ComponentDamageable & damageable
, glm::vec2 & playerOrigin
, pul::core::ComponentHitboxAABB &
) {
  auto & registry = scene.EnttRegistry();

  // TODO this should be accelerated with a structure and possibly use
  // raycast intersection tests too?

  auto view =
    registry.view<
      pul::core::ComponentPickup, pul::animation::ComponentInstance
    >();

  auto const playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);

  for (auto entity : view) {
    auto & pickup = view.get<pul::core::ComponentPickup>(entity);
    auto & animation = view.get<pul::animation::ComponentInstance>(entity);

    auto const pickupOrigin =
      glm::vec2(
        animation.instance.pieceToState["pickups"].cachedLocalSkeletalMatrix
      * glm::vec3(pickup.origin, 1.0f)
      )
    ;

    if (
        pickup.spawned
      && glm::length(playerOriginCenter - pickupOrigin) < 32.0f
    ) {
      pickup.spawned = false;
      pickup.spawnTimer = 0ul;

      scene.AudioSystem().pickup[Idx(pickup.type)] |= true;

      switch (pickup.type) {
        default: spdlog::error("unknown pickup type {}", pickup.type); break;
        case pul::core::PickupType::HealthLarge:
          damageable.health = glm::min(damageable.health+100u, 200u);
          spdlog::info("new health {}", damageable.health);
        break;
        case pul::core::PickupType::HealthMedium:
          damageable.health = glm::min(damageable.health+50u, 200u);
        break;
        case pul::core::PickupType::HealthSmall:
          damageable.health = glm::min(damageable.health+10u, 200u);
        break;
        case pul::core::PickupType::ArmorLarge:
          damageable.armor = glm::min(damageable.armor+200u, 200u);
        break;
        case pul::core::PickupType::ArmorMedium:
          damageable.armor = glm::min(damageable.armor+100u, 200u);
        break;
        case pul::core::PickupType::ArmorSmall:
          damageable.armor = glm::min(damageable.armor+10u, 200u);
        break;
        case pul::core::PickupType::Weapon:
          player.inventory.weapons[Idx(pickup.weaponType)].pickedUp = true;
          player.inventory.weapons[Idx(pickup.weaponType)].ammunition = 100;
        break;
        case pul::core::PickupType::WeaponAll:
          for (size_t i = 0ul; i < Idx(pul::core::WeaponType::Size); ++ i) {
            player.inventory.weapons[i].pickedUp = true;
            player.inventory.weapons[i].ammunition = 100;
          }
        break;
      }
    }
  }
}

void UpdatePlayerPhysics(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::core::ComponentHitboxAABB &
) {
  // if no velocity do nothing
  if (player.velocity == glm::vec2(0.0f)) { return; }

  // have a "diamond" around player to validate its position;
  // first forward pick points, clamping player origin based off
  // the closest intersection

  // then apply connect points to clamp the player off from the environment
  // order:
  //      0      *
  //     / \    1  4
  //    1   3  *    *
  //     \ /    2  3
  //      2      *

  std::array<glm::vec2, 4> constexpr pickPoints = {
    glm::vec2(0.0f, -48.0f)
  , glm::vec2(-10.0f, -22.0f)
  , glm::vec2(0.0f, -4.0f)
  , glm::vec2(+10.0f, -22.0f)
  };

  {
    auto wallPoint =
      pul::physics::IntersectorPoint{
        glm::round(pickPoints[1] + playerOrigin - glm::vec2(3.0f, 0.0f))
      }
    ;

    pul::physics::IntersectionResults results;
    player.wallClingLeft =
      plugin.physics.IntersectionPoint(scene, wallPoint, results);

    if (player.wallClingLeft)
      { player.velocity.x = glm::max(player.velocity.x, 0.0f); }

    if (player.grounded)
      { player.wallClingLeft = false; }
  }

  {
    auto wallPoint =
      pul::physics::IntersectorPoint{
        glm::round(pickPoints[3] + playerOrigin + glm::vec2(3.0f, 0.0f))
      }
    ;

    pul::physics::IntersectionResults results;
    player.wallClingRight =
      plugin.physics.IntersectionPoint(scene, wallPoint, results);
    if (player.wallClingRight)
      { player.velocity.x = glm::min(player.velocity.x, 0.0f); }

    if (player.grounded)
      { player.wallClingRight = false; }
  }

  pul::physics::IntersectionResults static pointResults;
  size_t closestIntersection = -1ul;

  for (size_t i = 0; i < pickPoints.size(); ++ i) {
    auto pointRay =
      pul::physics::IntersectorRay::Construct(
        glm::round(pickPoints[i] + playerOrigin)
      , glm::round(pickPoints[i] + playerOrigin + glm::vec2(player.velocity))
      );
    pul::physics::IntersectionResults static tempPointResults;
    plugin.physics.IntersectionRaycast(scene, pointRay, tempPointResults);

    // TODO pick shortest length
    if (tempPointResults.collision) {
      closestIntersection = i;
      pointResults = std::move(tempPointResults);
    }
  }

  if (closestIntersection == -1ul) {
    playerOrigin += player.velocity;
  } else {
    glm::vec2 intersectionNormal = glm::vec2(0.0f);

    { // calculate normal (TODO this should be precomputed)
      for (auto point : std::vector<glm::vec2>{
        { -1.0f, -1.0f }, { +0.0f, -1.0f }, { +1.0f, -1.0f }
      , { -1.0f, +0.0f },                   { +1.0f, +0.0f }
      , { -1.0f, +1.0f }, { +0.0f, +1.0f }, { +1.0f, +1.0f }
      }) {
        auto pointInt =
          pul::physics::IntersectorPoint{
            glm::i32vec2(glm::vec2(pointResults.origin) + point)
          };
        if (
          pul::physics::IntersectionResults pointResult;
          plugin.physics.IntersectionPoint(scene, pointInt, pointResult)
        ) {
          intersectionNormal += point;
        }
      }
      if (intersectionNormal  == glm::vec2(0.0)) {
        player.velocity = glm::vec2(0.01f);
        intersectionNormal = glm::vec2(1.0f);
      }

      intersectionNormal = glm::normalize(intersectionNormal);
    }

    spdlog::debug("normal: {}", intersectionNormal);

    glm::vec2 const targetDirection = 
      glm::reflect(glm::normalize(player.velocity), -intersectionNormal);

    if (player.jumping && player.hasReleasedJump) {
      player.storedVelocity =
          targetDirection
        * glm::length(player.storedVelocity)
      ;
    }

    player.velocity =
      targetDirection
    * glm::length(player.velocity)
    ;

    if (
        intersectionNormal == glm::vec2(0.0f, -1.0f)
     || intersectionNormal == glm::vec2(0.0f, 1.0f)
    ) {
      player.velocity.y = 0.0f;
      player.storedVelocity.y = 0.0f;
    } else if (
        intersectionNormal == glm::vec2(1.0f, 0.0f)
     || intersectionNormal == glm::vec2(-1.0f, 0.0f)
    ) {
      player.velocity.x = 0.0f;
      player.storedVelocity.x = 0.0f;
    } else {
      player.velocity *= 0.5f;
    }

    playerOrigin =
      glm::vec2(pointResults.origin) - pickPoints[closestIntersection]
    - intersectionNormal
    ;
  }

  // now apply the border
  //      0      *
  //     / \    1  4
  //    1   3  *    * <<<
  //     \ /    2  3
  //      2      *

  /* for (int i = 0; i < 4; ++ i) { */
  /*   glm::vec2 const point0 = pickPoints[i], point1 = pickPoints[(i+1)%4]; */
  /*   auto borderRay = */
  /*     pul::physics::IntersectorRay::Construct( */
  /*       glm::round(point0 + playerOrigin) */
  /*     , glm::round(point1 + playerOrigin) */
  /*     ); */
  /*   pul::physics::IntersectionResults borderResults; */
  /*   plugin.physics.IntersectionRaycast(scene, borderRay, borderResults); */

  /*   if (borderResults.collision) { */
  /*     // get the origin of the intersection in releation to center of player */
  /*     glm::vec2 origin = */
  /*       glm::vec2(borderResults.origin) - playerOrigin - glm::vec2(0.0f, 32.0f); */

  /*     playerOrigin -= origin; */
  /*   } */
  /* } */
}

void UpdatePlayerWeapon(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::controls::Controller const & controls
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::core::ComponentHitboxAABB &
, pul::animation::ComponentInstance & playerAnim
) {
  auto const & registry = scene.EnttRegistry();
  auto const & controller = controls.current;
  /* auto const & controllerPrev = controls.previous; */

  if (controller.weaponSwitch) {
    player.inventory.ChangeWeapon(controller.weaponSwitch);
  }

  if (controller.weaponSwitchToType != -1u) {
    player.inventory.ChangeWeapon(
      static_cast<pul::core::WeaponType>(controller.weaponSwitchToType)
    );
  }

  auto & weapon = player.inventory.weapons[Idx(player.inventory.currentWeapon)];
  (void)weapon;

  auto const & weaponMatrix =
    playerAnim
      .instance
      .pieceToState["weapon-placeholder"]
      .cachedLocalSkeletalMatrix
  ;

  bool const weaponFlip = playerAnim.instance.pieceToState["legs"].flip;

  if (weapon.cooldown > 0.0f) {
    weapon.cooldown -= pul::util::MsPerFrame;
    return; // do not execute weapon code
  } else {
    weapon.cooldown = 0.0f;
  }

  auto playerEntity = entt::to_entity(registry, player);

  switch (player.inventory.currentWeapon) {
    default: break;
    case pul::core::WeaponType::Pericaliya: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFirePericaliya(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::Manshredder: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireManshredder(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix
      , player, playerOrigin, playerAnim.instance, playerEntity
      );
    } break;
    case pul::core::WeaponType::Wallbanger: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireWallbanger(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::Unarmed: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireUnarmed(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::PMF: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFirePMF(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::BadFetus: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireBadFetus(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix
      , player, playerOrigin, playerAnim.instance, playerEntity
      );
    } break;
    case pul::core::WeaponType::ZeusStinger: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireZeusStinger(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix
      , player, playerAnim.instance, playerEntity
      );
    } break;
    case pul::core::WeaponType::Grannibal: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireGrannibal(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::DopplerBeam: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireDopplerBeam(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
    case pul::core::WeaponType::Volnias: {
      auto playerOriginCenter = playerOrigin - glm::vec2(0, 32.0f);
      plugin::entity::PlayerFireVolnias(
        controller.shootPrimary, controller.shootSecondary
      , player.velocity
      , weapon
      , plugin, scene, playerOriginCenter
      , controller.lookDirection, controller.lookAngle
      , weaponFlip, weaponMatrix, playerEntity
      );
    } break;
  }
}

}

void plugin::entity::ConstructPlayer(
  entt::entity & entity
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, bool mainPlayer
) {
  auto & registry = scene.EnttRegistry();
  entity = registry.create();
  registry.emplace<pul::core::ComponentPlayer>(entity);
  auto damageable = pul::core::ComponentDamageable { 100, 0 };
  registry.emplace<pul::core::ComponentDamageable>(entity, damageable);
  registry.emplace<pul::core::ComponentOrigin>(entity);
  registry.emplace<pul::controls::ComponentController>(entity);
  registry.emplace<pul::core::ComponentCamera>(entity);
  registry.emplace<pul::core::ComponentLabel>(entity, "Player");

  pul::animation::Instance instance;
  plugin.animation.ConstructInstance(
    scene, instance, scene.AnimationSystem(), "nygelstromn"
  );
  registry.emplace<pul::animation::ComponentInstance>(
    entity, std::move(instance)
  );

  auto & player = registry.get<pul::core::ComponentPlayer>(entity);
  auto & playerOrigin = registry.get<pul::core::ComponentOrigin>(entity);

  { // hitbox
    pul::core::ComponentHitboxAABB hitbox;
    hitbox.dimensions = glm::i32vec2(15, 50);
    registry.emplace<pul::core::ComponentHitboxAABB>(entity, hitbox);
  }

  // choose map origin
  if (scene.PlayerMetaInfo().playerSpawnPoints.size() > 0ul) {
    registry.get<pul::core::ComponentOrigin>(entity).origin =
      scene.PlayerMetaInfo().playerSpawnPoints[0];
  }

  // load up player weapon animation & state from previous plugin load
  if (mainPlayer) {

    // overwrite with player component for persistent reloads
    player = scene.StoredDebugPlayerComponent();

    if (scene.StoredDebugPlayerOriginComponent().origin != glm::vec2(0.0f))
      { playerOrigin = scene.StoredDebugPlayerOriginComponent(); }

    registry.emplace<pul::core::ComponentPlayerControllable>(entity);
  } else {
    registry.emplace<pul::core::ComponentBotControllable>(entity);
  }

  // load weapon animation
  pul::animation::Instance weaponInstance;
  plugin.animation.ConstructInstance(
    scene, weaponInstance, scene.AnimationSystem(), "weapons"
  );
  player.weaponAnimation = registry.create();

  registry.emplace<pul::animation::ComponentInstance>(
    player.weaponAnimation, std::move(weaponInstance)
  );
}

void plugin::entity::UpdatePlayer(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::controls::Controller const & controls
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::core::ComponentHitboxAABB & hitbox
, pul::animation::ComponentInstance & playerAnim
, pul::core::ComponentDamageable & damageable
) {

  // add/remove 19 while doing calculations as it basically offsets the hitbox
  // to the center
  playerOrigin += glm::vec2(0.0f, 28.0f);

  auto & registry = scene.EnttRegistry();

  auto const & controller = controls.current;
  auto const & controllerPrev = controls.previous;

  // error checking
  if (glm::abs(player.velocity.x) > 1000.0f) {
    spdlog::error("player velocity too high (for now)");
    player.velocity.x = 0.0f;
  }

  if (glm::abs(player.velocity.y) > 1000.0f) {
    spdlog::error("player velocity too high (for now)");
    player.velocity.y = 0.0f;
  }

  if (
      glm::isnan(player.velocity.x) || glm::isnan(player.velocity.y)
   || glm::isinf(player.velocity.x) || glm::isinf(player.velocity.y)
  ) {
    spdlog::error("floating point corruption on player velocity");
    player.velocity.x = 0.0f;
    player.velocity.y = 0.0f;
    player.storedVelocity.x = 0.0f;
    player.storedVelocity.y = 0.0f;
  }

  bool
    frameVerticalJump   = false
  , frameHorizontalJump = false
  , frameVerticalDash   = false
  , frameHorizontalDash = false
  , frameWalljump       = false
  ;

  bool const prevGrounded = player.grounded;

  // gravity/ground check
  if (player.affectedByGravity) {
    pul::physics::IntersectorPoint point;
    point.origin = playerOrigin + glm::vec2(0.0f, -3.0f);
    pul::physics::IntersectionResults results;
    player.grounded = plugin.physics.IntersectionPoint(scene, point, results);
  }

  bool const frameStartGrounded = player.grounded;
  bool const prevCrouchSliding = player.crouchSliding;

  // update damageable
  for (auto & damage : damageable.frameDamageInfos) {
    player.velocity += damage.directionForce;
    damageable.health = glm::max(damageable.health - damage.damage, 0);

    // pop origin up if grounded if there is Y force
    if (player.grounded && damage.directionForce.y < 0.0f) {
      playerOrigin.y -= 2.0f;
    }

    player.grounded = false;
  }
  damageable.frameDamageInfos = {};

  using MovementControl = pul::controls::Controller::Movement;

  { // -- process inputs / events

    // -- gravity

    // apply only when in air & only if dash zero gravity isn't in effect
    if (player.dashZeroGravityTime <= 0.0f && !player.grounded) {
      if (player.velocity.y <= ::inputGravityAccelThreshold) {
        player.velocity.y +=
          ::CalculateAccelFromTarget(
            ::inputGravityAccelPreThresholdTime, ::inputGravityAccelThreshold
          );
      } else {
        player.velocity.y += ::inputGravityAccelPostThreshold;
      }
    }

    // -- process crouching
    player.crouching = controller.crouch;
    playerAnim.instance.pieceToState["legs"].angle = 0.0f;
    playerAnim.instance.pieceToState["body"].angle = 0.0f;

    // if the player is crouch sliding, player remains crouched until the
    // animation is finished, the velocity is below crouch target, or the
    // player is jumping
    if (
        player.crouchSliding
     && !player.jumping
     && glm::abs(player.velocity.x) >= inputCrouchAccelTarget
     && !playerAnim.instance.pieceToState["legs"].animationFinished
    ) {
      player.crouching = true;
    }

    if (
        player.crouchSliding
     && (
          !player.crouching
       || glm::abs(player.velocity.x) < inputCrouchAccelTarget
       || player.jumping
       || !player.grounded
        )
    ) {
      player.crouchSliding = false;
    }

    if (player.crouchSlideCooldown > 0.0f)
      { player.crouchSlideCooldown -= pul::util::MsPerFrame; }

    if (player.slideFrictionTime > 0.0f)
      { player.slideFrictionTime -= pul::util::MsPerFrame; }

    // -- process jumping
    player.jumping = controller.jump;

    // set jump timer if falling this frame
    if (prevGrounded && !player.grounded) {
      player.jumpFallTime = ::jumpAfterFallTime;
    }

    if (player.jumpFallTime > 0.0f)
      { player.jumpFallTime -= pul::util::MsPerFrame; }

    if (!player.jumping) {
      player.storedVelocity = player.velocity;
      player.hasReleasedJump = true;
    }

    // make sure that jumping only happens if player has released jump yet
    if (player.grounded && !player.hasReleasedJump) {
      player.jumping = false;
    }

    if ((player.jumpFallTime > 0.0f || player.grounded) && player.jumping) {
      if (controller.movementHorizontal == MovementControl::None) {
        player.velocity.y = -::jumpingVerticalAccel;
        frameVerticalJump = true;
      } else {

        float thetaRad = glm::radians(::jumpingHorizontalTheta);

        player.velocity.y = -::jumpingHorizontalAccel * glm::sin(thetaRad);
        player.velocity.x = player.storedVelocity.x;

        if (glm::abs(player.velocity.x) < jumpingHorizontalAccelMax) {
          player.velocity.x +=
            + glm::sign(static_cast<float>(controller.movementHorizontal))
            * ::jumpingHorizontalAccel * glm::cos(thetaRad)
          ;
        }

        frameHorizontalJump = true;
      }
      player.grounded = false;
      player.hasReleasedJump = false;
      player.jumpFallTime = 0.0f;
    }

    // -- process walljumping

    // with jump you can hold space, this is not true for walljumps tho
    if (player.jumping && !controllerPrev.jump) {
      if (player.wallClingLeft || player.wallClingRight) {
        player.grounded = false;

        float const totalVel = glm::length(player.velocity) + 4.0f;

        float const thetaRad =
          (player.wallClingRight ? -pul::Pi*0.25f : 0.0f) + glm::radians(-75.0f)
        ;

        player.velocity.y = glm::sin(thetaRad) * totalVel;
        player.velocity.x = glm::cos(thetaRad) * totalVel;

        frameWalljump = true;
      }
    }

    // -- process horizontal movement
    float inputAccel = static_cast<float>(controller.movementHorizontal);
    float const facingDirection = inputAccel;

    bool frictionApplies = true;
    bool applyInputAccel = true;

    if (player.crouchSliding) {
      applyInputAccel = false;
    } else if (player.grounded) {
      if (player.jumping) {
        // this frame the player will jump
        frictionApplies = false;
      } else if (controller.movementHorizontal != MovementControl::None) {
        if (controller.walk) {
          ApplyGroundedMovement(
            facingDirection, player.velocity.x
          , ::inputWalkAccelTime, ::inputWalkAccelTarget
          , frictionApplies, inputAccel
          );
        } else if (controller.crouch) {
          if (
              (prevGrounded ? !controllerPrev.crouch : !prevCrouchSliding)
            && player.crouchSlideCooldown <= 0.0f
          ) {
            player.crouchSliding = true;
            applyInputAccel = false;

            // apply crouch boost, similar to dash boost
            player.velocity.x =
              glm::max(
                ::slideAccel + glm::length(player.velocity.x)
              , ::slideMinVelocity
              ) * facingDirection
            ;

            player.crouchSlideCooldown = ::slideCooldown;
            player.slideFrictionTime = ::slideFrictionTransitionTime;
          } else {
            ApplyGroundedMovement(
              facingDirection, player.velocity.x
            , ::inputCrouchAccelTime, ::inputCrouchAccelTarget
            , frictionApplies, inputAccel
            );
          }
        } else {
          ApplyGroundedMovement(
            facingDirection, player.velocity.x
          , ::inputRunAccelTime, ::inputRunAccelTarget
          , frictionApplies, inputAccel
          );
        }

        // if the target goal is reached then the accel is 0, but we still want
        // to keep it around
        if (inputAccel != 0.0f)
          { player.prevFrameGroundAcceleration = inputAccel; }

        // do not allow this since it has no friction to transition w/
        if (player.crouchSliding)
          { player.prevFrameGroundAcceleration = 0.0f; }
      }
    } else {
      if (facingDirection*player.velocity.x <= ::inputAirAccelThreshold) {
        inputAccel *=
          ::CalculateAccelFromTarget(
            ::inputAirAccelPreThresholdTime
          , ::inputAirAccelThreshold
          );
      } else {
        inputAccel *= ::inputAirAccelPostThreshold;
      }

      // set ground accel to 0 on no input
      if (controller.movementHorizontal == MovementControl::None)
        { player.prevFrameGroundAcceleration = 0.0f; }

      // mix inputAccel with grounded movement to give proper ground-air
      // transition
      if (player.jumpFallTime > 0.0f && !player.crouchSliding) {
        inputAccel =
          glm::mix(
            inputAccel
          , player.prevFrameGroundAcceleration*::frictionGrounded
          , player.jumpFallTime / ::jumpAfterFallTime
          );
      }

      frictionApplies = false;

      // friction applies if player is crouching and moving opposite direction
      // while in air
      if (
          glm::sign(player.velocity.x) != 0.0f
       && glm::sign(inputAccel) != 0.0f
       && player.crouching
       && glm::sign(inputAccel) != glm::sign(player.velocity.x)
       ) {
        frictionApplies = true;
       }
    }

    if (applyInputAccel)
      { player.velocity.x += inputAccel; }

    // -- process friction
    if (frictionApplies) {
      if (player.crouchSliding) {
        float const friction =
          glm::mix(
            ::slideFriction, 1.0f
          , player.slideFrictionTime <= 0.0f
              ? 0.0f
              : glm::pow(
                player.slideFrictionTime / ::slideFrictionTransitionTime
              , ::slideFrictionTransitionPow
              )
          );
        player.velocity.x *= friction;
      } else {
        player.velocity.x *= ::frictionGrounded;
      }
    }

    // -- process horizontal ground stop
    if (
        inputAccel == 0.0f && player.grounded
     && glm::abs(player.velocity.x) < ::horizontalGroundedVelocityStop
    ) {
      /* player.velocity.x = 0.0f; */
    }

    // -- process dashing
    for (auto & playerDashCooldown : player.dashCooldown) {
      if (playerDashCooldown > 0.0f)
        { playerDashCooldown -= pul::util::MsPerFrame; }
    }

    // clear dash lock if either we land, or we are jumping on this frame (thus
    // player will not be grounded)
    if (
        !prevGrounded
     && (frameHorizontalJump || frameVerticalJump || player.grounded)
    ) {
      for (auto & dashLock : player.dashLock)
        { dashLock = false; }
    }
    if (player.dashZeroGravityTime > 0.0f) {
      player.dashZeroGravityTime -= pul::util::MsPerFrame;
      // if grounded then gravity time has been nullified
      if (player.grounded || !controller.dash)
        { player.dashZeroGravityTime = 0.0f; }
    }

    // player has a limited amount of dashes in air, so reset that if grounded
    // at the start of frame
    if (frameStartGrounded)
      { player.midairDashesLeft = ::maxAirDashes; }

    if (
      !controllerPrev.dash && controller.dash
    && player.dashCooldown[Idx(controller.movementDirection)] <= 0.0f
    && player.dashLock[Idx(controller.movementDirection)] == false
    && player.midairDashesLeft > 0
    ) {
      glm::vec2 const scaledVelocity = player.velocity;

      float const transfers =
        CalculateDashTransferVelocity(
          glm::i32vec2(
            static_cast<int32_t>(controller.movementHorizontal)
          , static_cast<int32_t>(controller.movementVertical)
          )
        , NormalizeVelocityAxis(player.velocity)
        );

      float velocityMultiplier =
          transfers*(::dashAccel + glm::length(scaledVelocity));

      // add dashMin
      if (velocityMultiplier < ::dashMinVelocity) {
        velocityMultiplier =
          velocityMultiplier / ::dashMinVelocity * ::dashMinVelocityMultiplier
        + ::dashMinVelocity
        ;
      }

      if (controller.movementVertical != MovementControl::None) {
        frameVerticalDash = true;
      } else {
        frameHorizontalDash = true;
      }

      auto direction =
        glm::vec2(
          controller.movementHorizontal, controller.movementVertical
        );

      // if no keys are pressed just guess where player wants to go
      if (
          controller.movementHorizontal == MovementControl::None
       && controller.movementVertical == MovementControl::None
      ) {
        direction.x = player.velocity.x >= 0.0f ? +1.0f : -1.0f;
      }

      if (player.grounded) {
        playerOrigin.y -= 8.0f;
      }

      player.velocity = velocityMultiplier * glm::normalize(direction);
      player.grounded = false;

      player.dashCooldown[Idx(controller.movementDirection)] = ::dashCooldown;
      player.dashLock[Idx(controller.movementDirection)] = true;
      player.dashZeroGravityTime = ::dashGravityTime;
      -- player.midairDashesLeft;
    }
  }

  ::UpdatePlayerPhysics(plugin, scene, player, playerOrigin, hitbox);

  const float velocityXAbs = glm::abs(player.velocity.x);

  { // -- apply animations

    // -- set leg animation
    auto & legInfo = playerAnim.instance.pieceToState["legs"];
    auto & bodyInfo = playerAnim.instance.pieceToState["body"];

    if (player.grounded) { // grounded animations
      if (!player.crouchSliding) {
        bodyInfo.Apply("center");
        bodyInfo.angle = 0.0f;
      }

      if (!player.crouching && (!prevGrounded || player.landing)) {
        player.landing = true;
        auto & stateInfo = playerAnim.instance.pieceToState["legs"];
        stateInfo.Apply("landing");
        if (stateInfo.animationFinished) { player.landing = false; }
      } else {
        // check walk/run animation turns before applying stand/walk/run
        bool const applyTurning = false;

        bool const moving =
          controller.movementHorizontal != MovementControl::None
        ;

        bool const running = !controller.walk && !player.crouching && moving;
        bool const crouching = player.crouching && moving;
        bool const walking = controller.walk && !player.crouching && moving;

        if (legInfo.label == "run-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("run"); }
        } else if (legInfo.label == "walk-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("walk"); }
        } else if (walking && velocityXAbs <= inputWalkAccelTarget) {
          legInfo.Apply(applyTurning ? "walk-turn" : "walk");
        } else if (crouching && velocityXAbs <= inputCrouchAccelTarget) {
          legInfo.Apply("crouch-walk");
        } else if (running && velocityXAbs <= inputRunAccelTarget) {
          legInfo.Apply(applyTurning ? "run-turn" : "run");
        } else {
          if (player.crouchSliding) {
            legInfo.Apply("crouch-slide");
            if (legInfo.componentIt > 0) {
              legInfo.angle = pul::Pi;
              bodyInfo.Apply("crouch-center");
              bodyInfo.angle =
                (player.velocity.x > 0.0f ? -1.0f : +1.0f) * pul::Pi/1.5f;
            } else {
              // first frame is transition crouch
              legInfo.angle =
                (player.velocity.x > 0.0f ? +1.0f : -1.0f) * pul::Pi/2.0f;
              bodyInfo.Apply("crouch-transition");
            }
          } else if (player.crouching)
            { legInfo.Apply("crouch-idle"); }
          else
            { legInfo.Apply("stand"); }
        }
      }
    } else { // air animations

      playerAnim.instance.pieceToState["body"].Apply("center");

      if (frameVerticalJump) {
        playerAnim.instance.pieceToState["legs"].Apply("jump-high", true);
      } else if (frameHorizontalJump) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "jump-strafe-0" : "jump-strafe-1");
      } else if (frameVerticalDash) {
        playerAnim.instance.pieceToState["legs"].Apply("dash-vertical");
      } else if (frameHorizontalDash) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "dash-horizontal-0" : "dash-horizontal-1");
      } else if (frameWalljump) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "walljump-0" : "walljump-1");
      } else if (prevGrounded) {
        // logically can only have falled down
        playerAnim.instance.pieceToState["legs"].Apply("air-idle");
      } else {
        if (legInfo.label == "dash-vertical" && legInfo.animationFinished) {
          // switch to air idle
          playerAnim.instance.pieceToState["legs"].Apply("air-idle");
        }
      }
    }

    auto const & currentWeaponInfo =
      pul::core::weaponInfo[Idx(player.inventory.currentWeapon)];

    // -- arm animation
    bool playerDirFlip = playerAnim.instance.pieceToState["legs"].flip;
    switch (currentWeaponInfo.requiredHands) {
      case 0:
        if (player.grounded) {
          if (controller.crouch) {
            playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
            playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
          }
          else if (legInfo.label == "walk" || legInfo.label == "walk-turn") {
            playerAnim.instance.pieceToState["arm-back"].Apply("unequip-walk");
            playerAnim.instance.pieceToState["arm-front"].Apply("unequip-walk");
          }
          else if (legInfo.label == "run" || legInfo.label == "run-turn") {
            playerAnim.instance.pieceToState["arm-back"].Apply("unequip-run");
            playerAnim.instance.pieceToState["arm-front"].Apply("unequip-run");
          } else {
            playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
            playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
          }
        } else {
          playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
          playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
        }
      break;
      case 1:
        if (playerDirFlip)
          playerAnim.instance.pieceToState["arm-back"].Apply("equip-1H");
        else
          playerAnim.instance.pieceToState["arm-front"].Apply("equip-1H");
      break;
      case 2:
        playerAnim.instance.pieceToState["arm-back"].Apply("equip-2H");
        playerAnim.instance.pieceToState["arm-front"].Apply("equip-2H");
      break;
    }

    if (
        controller.movementHorizontal
     == pul::controls::Controller::Movement::Right
    ) {
      playerDirFlip = true;
    }
    else if (
        controller.movementHorizontal
     == pul::controls::Controller::Movement::Left
    ) {
      playerDirFlip = false;
    }

    playerAnim.instance.pieceToState["legs"].flip = playerDirFlip;

    float const angle =
      std::atan2(controller.lookDirection.x, controller.lookDirection.y);
    player.lookAtAngle = angle;
    player.flip = playerDirFlip;

    playerAnim.instance.pieceToState["arm-back"].angle
      = playerAnim.instance.pieceToState["arm-front"].angle
      = angle
    ;

    playerAnim.instance.pieceToState["head"].angle = angle;

    playerAnim.instance.origin = playerOrigin;

    // center weapon origin, first have to update cache for this animation to
    // get the hand position
    {
      plugin.animation.UpdateCache(playerAnim.instance);
      auto & handState = playerAnim.instance.pieceToState["weapon-placeholder"];

      char const * weaponStr = ToStr(player.inventory.currentWeapon);

      auto & weaponAnimation =
        registry.get<pul::animation::ComponentInstance>(
          player.weaponAnimation
        ).instance;

      // nothing should render if unarmed
      weaponAnimation.visible =
        player.inventory.currentWeapon != pul::core::WeaponType::Unarmed;

      auto & weaponState = weaponAnimation.pieceToState["weapons"];

      weaponState.Apply(weaponStr);

      weaponAnimation.origin = playerAnim.instance.origin;

      weaponState.angle = playerAnim.instance.pieceToState["arm-front"].angle;
      weaponState.flip = playerAnim.instance.pieceToState["legs"].flip;

      plugin.animation.UpdateCacheWithPrecalculatedMatrix(
        weaponAnimation, handState.cachedLocalSkeletalMatrix
      );

    }
  }

  ::UpdatePlayerWeapon(
    plugin, scene, controls, player, playerOrigin, hitbox, playerAnim
  );
  ::PlayerCheckPickups(scene, player, damageable, playerOrigin, hitbox);

  auto & audioSystem = scene.AudioSystem();
  audioSystem.playerJumped |=
    frameVerticalJump || frameHorizontalJump || frameWalljump;
  audioSystem.playerSlided |= player.crouchSliding && !prevCrouchSliding;
  audioSystem.playerDashed |= frameHorizontalDash || frameVerticalDash;
  audioSystem.playerTaunted |= controller.taunt && !controllerPrev.taunt;

  auto & legInfo = playerAnim.instance.pieceToState["legs"];

  if (player.grounded && legInfo.label == "crouch-walk") {
    static size_t prevComp = 0;
    audioSystem.playerStepped |=
        (prevComp % 5 != 0) && legInfo.componentIt % 5 == 0
     && legInfo.componentIt != 0
    ;
    prevComp = legInfo.componentIt;
  }

  if (player.grounded && legInfo.label == "walk") {
    static size_t prevComp = 0;
    audioSystem.playerStepped |=
        (prevComp % 3 != 0) && legInfo.componentIt % 3 == 0
     && legInfo.componentIt != 0
    ;
    prevComp = legInfo.componentIt;
  }

  if (player.grounded && legInfo.label == "run") {
    static size_t prevComp = 0;
    audioSystem.playerStepped |=
        (prevComp % 3 != 0) && legInfo.componentIt % 3 == 0
     && legInfo.componentIt != 0
    ;
    prevComp = legInfo.componentIt;
  }

  if (audioSystem.envLanded == -1ul && !prevGrounded && frameStartGrounded) {
    audioSystem.envLanded =
      static_cast<size_t>(glm::clamp(player.prevAirVelocity/5.0f, 0.0f, 2.0f));
    audioSystem.playerLanded |= player.prevAirVelocity > 9.0f;
  }

  playerOrigin -= glm::vec2(0.0f, 28.0f);
}

void plugin::entity::UiRenderPlayer(
  pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, pul::animation::ComponentInstance &
) {
  ImGui::Begin("Physics");

  ImGui::Separator();
  ImGui::Separator();

  ImGui::PushItemWidth(74.0f);
  ImGui::DragFloat("run target time", &::inputRunAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while running; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputRunAccelTime, ::inputRunAccelTarget)
  );
  ImGui::DragFloat("run target accel", &::inputRunAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while running, once this has been reached\n"
    "the player will no longer be able to run any faster"
  );
  ImGui::DragFloat("walk target time", &::inputWalkAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while walking; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputWalkAccelTime, ::inputWalkAccelTarget)
  );
  ImGui::DragFloat("walk target accel", &::inputWalkAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while walking, once this has been reached\n"
    "the player will no longer be able to walk any faster"
  );

  ImGui::DragFloat("crouch target time", &::inputCrouchAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while crouching; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputCrouchAccelTime, ::inputCrouchAccelTarget)
  );
  ImGui::DragFloat("crouch target accel", &::inputCrouchAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while crouching, once this has been reached\n"
    "the player will no longer be able to crouch any faster"
  );

  ImGui::DragFloat(
    "horizontal grounded velocity stop"
  , &::horizontalGroundedVelocityStop, 0.005f
  );
  pul::imgui::ItemTooltip(
    "if the player is grounded and below this threshold, the velocity will be\n"
    "instantly set to 0"
  );

  ImGui::DragFloat(
    "gravity accel pre-threshold time", &inputGravityAccelPreThresholdTime, 0.5f
  );
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach gravity threshold from a base velocity\n"
    "of 0 while in mid-air; currently {:.3f} texels/frame will be added;\n"
    "milliseconds"
  , ::CalculateAccelFromTarget(
      ::inputGravityAccelPreThresholdTime, ::inputGravityAccelThreshold
    )
  );
  ImGui::DragFloat(
    "gravity accel post-threshold", &inputGravityAccelPostThreshold, 0.005f
  );
  pul::imgui::ItemTooltip(
    "acceleration added per frame of gravity when velocity Y is post-threshold"
  );
  ImGui::DragFloat(
    "gravity accel threshold", &inputGravityAccelThreshold, 0.05f
  );
  pul::imgui::ItemTooltip(
    "acceleration target while in mid-air, once this has been reached\n"
    "the player will use the gravity accel post-threshold up to 'infinity'"
  );

  ImGui::DragFloat(
    "air accel pre-threshold time", &inputAirAccelPreThresholdTime, 0.005f
  );
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach air threshold from a base velocity of 0\n"
    "while in mid-air; currently {:.3f} texels/frame will be added;\n"
    "milliseconds"
  , ::CalculateAccelFromTarget(
      ::inputAirAccelPreThresholdTime, ::inputAirAccelThreshold
    )
  );
  ImGui::DragFloat(
    "air accel post-threshold", &inputAirAccelPostThreshold, 0.005f
  );
  pul::imgui::ItemTooltip(
    "acceleration added per frame when velocity in air is post-threshold"
  );
  ImGui::DragFloat(
    "air accel threshold", &inputAirAccelThreshold, 0.05f
  );
  pul::imgui::ItemTooltip(
    "acceleration target while in mid-air, once this has been reached\n"
    "the player will use the air accel post-threshold up to 'infinity'"
  );

  pul::imgui::ItemTooltip(
    "acceleration added downwards per frame from gravity; texels"
  );
  ImGui::DragFloat("jump vertical accel", &::jumpingVerticalAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration upwards set the frame a vertical jump is made; texels"
  );
  ImGui::DragFloat("jump hor theta", &::jumpingHorizontalTheta, 0.1f);
  pul::imgui::ItemTooltip(
    "the angle which horizontal jump is directed towards; degrees"
  );
  ImGui::DragFloat("jump hor accel", &::jumpingHorizontalAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration horizontally set the frame a horizontal jump is made; texels"
  );
  ImGui::DragFloat(
    "jump hor accel limit", &::jumpingHorizontalAccelMax, 0.005f
  );
  pul::imgui::ItemTooltip(
    "maximum horizontal velocity before the 'jump hor accel' is removed\n"
    "thus further horizontal jumps do not add a horizontal acceleration; texels"
  );
  ImGui::DragFloat(
    "jump after fall time", &::jumpAfterFallTime, 0.005f
  );
  pul::imgui::ItemTooltip(
    "maximum time after falling off a ledge to allow a jump to occur; ms\n"
  );

  ImGui::DragFloat("friction grounded", &::frictionGrounded, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is grounded"
  );
  ImGui::DragFloat("dash accel", &::dashAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "amount to add to velocity the frame a dash is made; texels"
  );

  ImGui::SliderFloat(
    "dashVerticalTransfer.same", &dashVerticalTransfer.same, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in same direction"
  );
  ImGui::SliderFloat(
    "dashVerticalTransfer.reverse", &dashVerticalTransfer.reverse, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in reverse direction"
  );
  ImGui::SliderFloat(
    "dashVerticalTransfer.degree90", &dashVerticalTransfer.degree90, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in 90-degree direction"
  );

  ImGui::SliderFloat(
    "dashHorizontalTransfer.same", &dashHorizontalTransfer.same, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in same direction"
  );
  ImGui::SliderFloat(
    "dashHorizontalTransfer.reverse", &dashHorizontalTransfer.reverse
  , 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in reverse direction"
  );
  ImGui::SliderFloat(
    "dashHorizontalTransfer.degree90", &dashHorizontalTransfer.degree90
  , 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in 90-degree direction"
  );

  ImGui::DragFloat("dash min velocity", &::dashMinVelocity, 0.01f);
  pul::imgui::ItemTooltip(
    "gives a minimal threshold for velocity on dash, thus ensuring that on\n"
    "dash, the player is moving by at least this velocity; texels"
  );
  ImGui::DragFloat(
    "dash min velocity multiplier", &::dashMinVelocityMultiplier, 0.01f
  );
  pul::imgui::ItemTooltip(
    "multiplies velocity by this amount divided by dashMinVelocity; which\n"
    "allows to conserve some velocity when below the minimum velocity, rather\n"
    "then it just being set to a specific value.\n"
    "\n"
    "For example, if the dashMinVelocity is 4, the\n"
    "dashMinVelocityMultiplier is 2, and the player is moving at a speed\n"
    "of 3, he will get:\n"
    "dashMinVelocity + (speed/dashMinVelocity) * dashMinVelocityMultiplier\n"
    "4               + (3/4) * 2\n"
    "velocity, instead of just 4"
  );
  ImGui::DragInt("max air dashes", &::maxAirDashes, 0.25f, 0, 10);
  pul::imgui::ItemTooltip("maximum amount of dashes that can occur in air");

  ImGui::DragFloat("dash cooldown", &::dashCooldown, 0.1f);
  pul::imgui::ItemTooltip(
    "minimal amount of time needed to transpire between each dash; milliseconds"
  );

  ImGui::DragFloat("dash zero gravity time", &::dashGravityTime, 0.1f);
  pul::imgui::ItemTooltip(
    "amount of time after a dash that no gravity will be applied; ms"
  );

  ImGui::DragFloat("slope step-up height", &::slopeStepUpHeight, 0.005f);
  pul::imgui::ItemTooltip(
    "the amount of texels to check if player can climb up a slope just by\n"
    "traversing it; texels"
  );

  ImGui::DragFloat("slope step-down height", &::slopeStepDownHeight, 0.005f);
  pul::imgui::ItemTooltip(
    "the amount of texels to check if player can climb down a slope just by\n"
    "traversing it; texels"
  );

  ImGui::DragFloat("slide accel", &::slideAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "amount to add to velocity the frame a slide is made; texels"
  );
  ImGui::DragFloat("slide min velocity", &::slideMinVelocity, 0.01f);
  pul::imgui::ItemTooltip(
    "gives a minimal threshold for velocity on slide, thus ensuring that on\n"
    "slide, the player is moving by at least this velocity; texels"
  );
  ImGui::DragFloat("slide cooldown", &::slideCooldown, 0.05f);
  pul::imgui::ItemTooltip("minimal amount of time between sliding; ms");

  ImGui::DragFloat("friction slide", &::slideFriction, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is sliding"
  );
  ImGui::DragFloat(
    "slideFrictionTransitionTime", &::slideFrictionTransitionTime, 0.05f
  );
  pul::imgui::ItemTooltip(
    "time it takes to transition from 0-friction to to slide friction\n"
    "while sliding; ms");
  ImGui::DragFloat(
    "slideFrictionTransitionPow", &::slideFrictionTransitionPow, 0.05f
  );
  pul::imgui::ItemTooltip("the pow exponent/curve in friction transition");

  ImGui::PopItemWidth();

  ImGui::Separator();
  ImGui::Separator();

  ImGui::End();

  ImGui::Begin("HUD (debug/wip)");

  pul::imgui::Text(
    "Speed (horizontal) {}", static_cast<int32_t>(player.velocity.x*90.0f)
  );
  pul::imgui::Text(
    "Speed (vertical) {}", static_cast<int32_t>(player.velocity.y*90.0f)
  );
  pul::imgui::Text(
    "Speed (total) {}", static_cast<int32_t>(glm::length(player.velocity)*90.0f)
  );

  pul::imgui::Text("debug transfer details: {}", ::debugTransferDetails);

  static bool showVel = true;

  ImGui::Checkbox("show velocity (might cause lag)", &showVel);

  if (showVel) {
    static std::vector<float> velocities;

    if (velocities.size() == 0ul) { velocities.resize(512); }

    if (scene.numCpuFrames != 0ul) {
      std::rotate(velocities.begin(), velocities.begin()+1, velocities.end());
      velocities.back() = glm::length(player.velocity)*90.0f;
    }

    ImGui::PlotLines("velocity tracker", velocities.data(), velocities.size());
  }

  ImGui::End();
}
