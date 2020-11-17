#include <plugin-entity/config.hpp>
#include <plugin-entity/weapon.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-core/weapon.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/transform2.hpp>

#include <entt/entt.hpp>

// TODO
// we have many unused parameters, probably have to change this to a
// struct-based API in order to not have to deal with this anymore
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace {

struct ComponentZeusStingerSecondary {};
struct ComponentBadFetusSecondary {};

void CreateBadFetusLinkedBeam(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::animation::Instance & playerAnim
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 hitOrigin
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  auto origin = playerOrigin + glm::vec2(0, 28.0f);

  auto const & weaponState =
    playerAnim
      .pieceToState["weapon-placeholder"];
  auto const & weaponMatrix = weaponState.cachedLocalSkeletalMatrix;

  namespace config = plugin::config::badFetus::combo;

  { // muzzle
    auto badFetusMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      badFetusMuzzleEntity, playerOrigin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "bad-fetus-link-muzzle-flash"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-link-muzzle-flash", true);
    state.angle = player.lookAtAngle;
    state.flip = weaponState.flip;

    instance.origin = origin + glm::vec2(0.0f, 28.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, weaponMatrix);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusMuzzleEntity, std::move(instance)
    );
  }

  auto badFetusBallEntity = registry.create();
  { // linked secondary ball
    registry.emplace<pul::core::ComponentParticle>(
      badFetusBallEntity, playerOrigin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "bad-fetus-linked-ball-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-linked-ball-projectile", true);
    state.angle = 0.0f;
    state.flip = false;

    instance.origin = hitOrigin;

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusBallEntity, std::move(instance)
    );
  }

  // projectile
  auto badFetusBeamEntity = registry.create();

  { // animation
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "bad-fetus-link-beam"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-link-beam", true);
    instance.origin = playerOrigin + glm::vec2(0.0f, 28.0f);
    state.flip = weaponState.flip;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, weaponMatrix);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusBeamEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      badFetusBeamEntity
    , instance.origin, glm::vec2{}, false, false
    );
  }

  { // particle beam
    pul::core::ComponentParticleBeam particle;
    particle.update =
      [
        badFetusBallEntity, &registry, &scene, &plugin, &playerAnim
      , &weaponInfo, &playerOrigin, playerEntity
      ](
        pul::animation::Instance & animInstance, float & /*cooldown*/
      ) -> bool {

        // TODO rename
        auto & animComponent =
          registry.get<pul::animation::ComponentInstance>(badFetusBallEntity);

        // TODO move this out
        static glm::vec2 accel = glm::vec2(0.0f);

        { // check if beam should be destroyed
          auto const * const badFetusInfo =
            std::get_if<pul::core::WeaponInfo::WiBadFetus>(&weaponInfo.info);

          if (!badFetusInfo || !badFetusInfo->primaryActive) {

            // shoot ball again
            { // projectile
              auto badFetusProjectileEntity = registry.create();

              pul::animation::Instance instance;
              plugin.animation.ConstructInstance(
                scene, instance, scene.AnimationSystem()
              , "bad-fetus-linked-ball-projectile"
              );
              auto & state = instance.pieceToState["particle"];
              state.Apply("bad-fetus-linked-ball-projectile", true);
              state.angle = 0.0f;
              state.flip = false;

              instance.origin = animComponent.instance.origin;

              registry.emplace<pul::animation::ComponentInstance>(
                badFetusProjectileEntity, std::move(instance)
              );

              {
                pul::core::ComponentParticleGrenade particle;

                plugin.animation.ConstructInstance(
                  scene, particle.animationInstance, scene.AnimationSystem()
                , "bad-fetus-explosion"
                );

                particle
                  .animationInstance
                  .pieceToState["particle"]
                  .Apply("bad-fetus-explosion", true);

                particle.origin = animComponent.instance.origin;
                particle.velocity = accel;
                particle.velocityFriction = config::VelocityFriction();
                particle.gravityAffected = false;
                particle.useBounces = true;
                particle.bounces = 0;
                particle.bounceAnimation = "bad-fetus-explosion";

                particle.damage.damagePlayer = true;
                particle.damage.ignoredPlayer = playerEntity;
                particle.damage.explosionRadius    = config::ExplosionRadius();
                particle.damage.explosionForce     = config::ExplosionForce();
                particle.damage.playerSplashDamage =
                  config::ProjectileSplashDamageMax();
                particle.damage.playerDirectDamage =
                  config::ProjectileDirectDamage();

                registry.emplace<pul::core::ComponentParticleGrenade>(
                  badFetusProjectileEntity, std::move(particle)
                );
              }
            }


            registry.destroy(badFetusBallEntity);
            return true;
          }
        }

        // -- update animation origin/direction
        auto const & weaponState =
          playerAnim
            .pieceToState["weapon-placeholder"];

        bool const weaponFlip = playerAnim.pieceToState["legs"].flip;

        animInstance.origin = playerOrigin + glm::vec2(0.0f, 28.0f);

        auto & animState = animInstance.pieceToState["particle"];
        animState.flip = weaponFlip;

        auto const & weaponMatrix = weaponState.cachedLocalSkeletalMatrix;
        plugin
          .animation
          .UpdateCacheWithPrecalculatedMatrix(animInstance, weaponMatrix);

        // -- update animation clipping
        animState.uvCoordWrap.x = 1.0f;
        animState.vertWrap.x = 1.0f;
        animState.flipVertWrap = false;

        auto const beginOrigin =
            animInstance.origin
          + glm::vec2(
                weaponMatrix
              * glm::vec3(0.0f, 0.0f, 1.0f)
            )
        ;

        // I could maybe use the animState matrix here instead of weapon
        auto endOrigin = animComponent.instance.origin;

        bool intersection = false;

        auto beamRay =
          pul::physics::IntersectorRay::Construct(
            beginOrigin
          , endOrigin
          );
        if (
          pul::physics::IntersectionResults resultsBeam;
          plugin.physics.IntersectionRaycast(scene, beamRay, resultsBeam)
        ) {
          intersection = true;
          endOrigin = resultsBeam.origin;
        }


        // choose between either endOrigin or controls i guess
        {
          auto controlCurrent = scene.PlayerController().current;

          auto controlOrigin =
            playerOrigin + controlCurrent.lookOffset - glm::vec2(0.0f, 28.0f)
          ;

          if (
              glm::length(endOrigin - beginOrigin)
           >= glm::length(animComponent.instance.origin - beginOrigin)
          ) {
            endOrigin = controlOrigin;
            intersection = false;
          } else {
            intersection = true;
          }
        }


        {
          // apply clipping
          animState.uvCoordWrap.x =
            glm::length(glm::vec2(beginOrigin) - glm::vec2(endOrigin)) / 384.0f;
          animState.vertWrap.x = animState.uvCoordWrap.x;
          if (!weaponFlip) {
            animState.flipVertWrap = true;
          }
        }


        float len = glm::length(endOrigin - animComponent.instance.origin);
        glm::vec2 dir =
          glm::normalize(endOrigin - animComponent.instance.origin);

        accel =
          glm::clamp(
            accel + dir*len*0.003f, glm::vec2(-15.0f), glm::vec2(15.0f)
          )
        ;

        accel +=
          glm::vec2(
            0.0f, 0.05f*glm::clamp(1.0f - glm::length(accel), 0.0f, 1.0f)
          );

        accel *= 0.95f;

        animComponent.instance.origin += accel;

        if (intersection) {
          animComponent.instance.origin =
            mix(animComponent.instance.origin, endOrigin, 0.7f);
          accel *= -1.0f;
        }


        return false;
      }
    ;

    registry.emplace<pul::core::ComponentParticleBeam>(
      badFetusBeamEntity, std::move(particle)
    );
  }
}

void GrannibalMuzzleTrail(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin
, bool const flip, glm::mat3 const & matrix
) {
  auto & registry = scene.EnttRegistry();
  auto grannibalFireEntity = registry.create();

  registry.emplace<pul::core::ComponentParticle>(
    grannibalFireEntity, origin, glm::vec2(0.0f, -1.0f)
  );

  pul::animation::Instance instance;
  plugin.animation.ConstructInstance(
    scene, instance, scene.AnimationSystem(), "grannibal-fire"
  );
  auto & state = instance.pieceToState["particle"];
  state.Apply("grannibal-fire", true);
  state.angle = 0.0f;
  state.flip = flip;

  instance.origin = origin + glm::vec2(0.0f, 32.0f);
  instance.automaticCachedMatrixCalculation = false;

  plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

  registry.emplace<pul::animation::ComponentInstance>(
    grannibalFireEntity, std::move(instance)
  );
}

}

void plugin::entity::PlayerFireVolnias(
  bool const primary, bool const secondary
, glm::vec2 & velocity // TODO remove this
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & volInfo = std::get<pul::core::WeaponInfo::WiVolnias>(weaponInfo.info);
  auto & audioSystem = scene.AudioSystem();

  bool forceCooldown = (primary && secondary) || volInfo.dischargingSecondary;

  namespace config = plugin::config::volnias::primary;
  namespace configSec = plugin::config::volnias::secondary;

  // TODO use controller or something
  static bool prevPrim = false;

  if (
      !primary && prevPrim
   && volInfo.primaryChargeupTimer >= config::ChargeupTimerEnd()
  ) {
    audioSystem.volniasEndPrimary = true;
    weaponInfo.cooldown = config::DischargeCooldown();
  }

  prevPrim = primary;

  // apply cooldown
  if (!primary || forceCooldown) {
    volInfo.primaryChargeupTimer =
      glm::max(0.0f, volInfo.primaryChargeupTimer - pul::util::MsPerFrame*2.0f);
    volInfo.hasChargedPrimary = false;
  }

  // apply primary shooting
  if (primary && !forceCooldown) {
    volInfo.primaryChargeupTimer += pul::util::MsPerFrame;
    if (
        !volInfo.hasChargedPrimary
     && volInfo.primaryChargeupTimer >= config::ChargeupPreBeginThreshold()
    ) {
      volInfo.hasChargedPrimary = true;
      audioSystem.volniasPrefirePrimary = true;
    }

    if (volInfo.primaryChargeupTimer >= config::ChargeupBeginThreshold()) {
      volInfo.primaryChargeupTimer -= config::ChargeupDelta();

      plugin::entity::FireVolniasPrimary(
        plugin, scene, origin, direction, angle, flip, matrix, playerEntity
      );
    }
  }

  // apply secondary chargeup
  if (secondary && !forceCooldown) {
    if (volInfo.secondaryChargedShots < configSec::MaxChargedShots()) {
      volInfo.secondaryChargeupTimer += pul::util::MsPerFrame;
      if (volInfo.secondaryChargeupTimer >= configSec::ChargeupDelta()) {
        volInfo.secondaryChargeupTimer -= configSec::ChargeupDelta();
        audioSystem.volniasChargePrimary = true;

        ++ volInfo.secondaryChargedShots;
        if (volInfo.secondaryChargedShots == configSec::MaxChargedShots()) {
          audioSystem.volniasChargeSecondary = true;
        }
      }
    } else {
      // check if we have to force shoot

      volInfo.secondaryChargeupTimer += pul::util::MsPerFrame;
      if (
          !volInfo.overchargedSecondary
       && volInfo.secondaryChargeupTimer
       >= configSec::ChargeupMaxThreshold() - 500.0f
      ) {
        audioSystem.volniasPrefireSecondary = true;
        volInfo.overchargedSecondary = true;
      }

      if (volInfo.secondaryChargeupTimer >= configSec::ChargeupMaxThreshold()) {
        forceCooldown = true;
      }
    }
  }

  // secondary fires on release
  if (!secondary || forceCooldown) {
    volInfo.secondaryChargeupTimer = configSec::ChargeupTimerStart();
    volInfo.overchargedSecondary = false;
    if (volInfo.secondaryChargedShots > 0u) {
      if (!volInfo.dischargingSecondary) {
        audioSystem.volniasFire = (volInfo.secondaryChargedShots-1)/2;
      }
      volInfo.dischargingSecondary = true;

      volInfo.dischargingTimer += pul::util::MsPerFrame;
      if (volInfo.dischargingTimer > configSec::DischargeDelta()) {
        volInfo.dischargingTimer -= configSec::DischargeDelta();
        plugin::entity::FireVolniasSecondary(
          3, volInfo.secondaryChargedShots-1, plugin
        , scene, origin, angle, flip, matrix
        , playerEntity
        );
        if (--volInfo.secondaryChargedShots == 0u) {
          volInfo.dischargingSecondary = false;
          volInfo.dischargingTimer = 0.0f;
          weaponInfo.cooldown = config::DischargeCooldown();
        }
      }
    }
  }
}

void plugin::entity::FireVolniasPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {

  auto & registry = scene.EnttRegistry();
  auto & audioSystem = scene.AudioSystem();

  namespace config = plugin::config::volnias::primary;

  if (audioSystem.volniasFire == -1ul) { audioSystem.volniasFire = 0ul; }

  {
    auto volniasFireEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      volniasFireEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "volnias-fire"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("volnias-fire", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      volniasFireEntity, std::move(instance)
    );
  }

  {
    auto volniasProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "volnias-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("volnias-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    registry.emplace<pul::animation::ComponentInstance>(
      volniasProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      volniasProjectileEntity
    , instance.origin, direction * config::ProjectileVelocity()
    );


    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;
    exploder.damage.damagePlayer = true;
    exploder.damage.ignoredPlayer = playerEntity;
    exploder.damage.explosionRadius = 0.0f;
    exploder.damage.explosionForce     = config::ProjectileForce();
    exploder.damage.playerSplashDamage = 0.0f;
    exploder.damage.playerDirectDamage = config::ProjectileDamage();

    plugin.animation.ConstructInstance(
      scene, exploder.animationInstance, scene.AnimationSystem()
    , "volnias-hit"
    );

    exploder
      .animationInstance
      .pieceToState["particle"].Apply("volnias-hit", true);

    exploder.audioTrigger = &scene.AudioSystem().volniasHit;

    registry.emplace<pul::core::ComponentParticleExploder>(
      volniasProjectileEntity, std::move(exploder)
    );
  }

  // knockback player if they are in air
  auto & player = registry.get<pul::core::ComponentPlayer>(playerEntity);
  if (!player.grounded)
    { player.velocity += -direction*config::Knockback(); }
}

void plugin::entity::FireVolniasSecondary(
  uint8_t shots, uint8_t shotSet
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  std::array<std::array<float, 3>, 5> shotPattern {{
    { -0.04f,  0.00f, +0.02f }
  , { -0.15f,  0.10f, +0.05f }
  , { -0.09f, -0.04f, +0.01f }
  , {  0.02f,  0.10f, +0.15f }
  , { -0.01f,  0.04f, +0.09f }
  }};
  for (auto fireAngle : shotPattern[shotSet]) {
    if (shots -- == 0) { break; }
    fireAngle += angle;
    auto dir = glm::vec2(glm::sin(fireAngle), glm::cos(fireAngle));
    plugin::entity::FireVolniasPrimary(
      plugin, scene, origin, dir, fireAngle, flip, matrix, playerEntity
    );
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireGrannibal(
  bool const primary, [[maybe_unused]] bool const secondary
, [[maybe_unused]] glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & grannibalInfo =
    std::get<pul::core::WeaponInfo::WiGrannibal>(weaponInfo.info);

  namespace config = plugin::config::grannibal::primary;
  namespace configSec = plugin::config::grannibal::secondary;

  if (grannibalInfo.primaryMuzzleTrailLeft > 0) {
    if (grannibalInfo.primaryMuzzleTrailTimer <= 0.0f) {
      ::GrannibalMuzzleTrail(plugin, scene, origin, flip, matrix);
      -- grannibalInfo.primaryMuzzleTrailLeft;
      grannibalInfo.primaryMuzzleTrailTimer = config::MuzzleTrailTimer();
    }
    grannibalInfo.primaryMuzzleTrailTimer -= pul::util::MsPerFrame;
  }

  if (grannibalInfo.dischargingTimer > 0.0f) {
    grannibalInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    grannibalInfo.dischargingTimer = config::DischargeCooldown();
    plugin::entity::FireGrannibalPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }

  if (secondary) {
    grannibalInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireGrannibalSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireGrannibalPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  auto & grannibalInfo =
    std::get<pul::core::WeaponInfo::WiGrannibal>(weaponInfo.info);

  namespace config = plugin::config::grannibal::primary;

  grannibalInfo.primaryMuzzleTrailLeft = config::MuzzleTrailParticles();
  grannibalInfo.primaryMuzzleTrailTimer = config::MuzzleTrailTimer();

  ::GrannibalMuzzleTrail(plugin, scene, origin, flip, matrix);

  {
    auto grannibalProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "grannibal-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("grannibal-projectile", true);
    state.angle = angle;
    state.flip = flip;

    // TODO ? use weapon holder or sometihg
    instance.origin = origin - glm::vec2(-30.0f, -30.0f)*direction;

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "grannibal-primary-projectile-trail"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("grannibal-primary-projectile-trail", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 16.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        grannibalProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      grannibalProjectileEntity
    , instance.origin, direction*config::ProjectileVelocity(), false, true
    );

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;
    exploder.damage.damagePlayer = true;
    exploder.damage.ignoredPlayer = playerEntity;
    exploder.damage.explosionRadius    = config::ProjectileExplosionRadius();
    exploder.damage.explosionForce     = config::ProjectileExplosionForce();
    exploder.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
    exploder.damage.playerDirectDamage = config::ProjectileDirectDamage();

    plugin.animation.ConstructInstance(
      scene, exploder.animationInstance, scene.AnimationSystem()
    , "grannibal-hit"
    );

    exploder
      .animationInstance
      .pieceToState["particle"].Apply("grannibal-hit", true);

    registry.emplace<pul::core::ComponentParticleExploder>(
      grannibalProjectileEntity, std::move(exploder)
    );
  }
}

void plugin::entity::FireGrannibalSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const &
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::grannibal::secondary;

  [[maybe_unused]]
  auto & grannibalInfo =
    std::get<pul::core::WeaponInfo::WiGrannibal>(weaponInfo.info);

  {
    auto grannibalProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "grannibal-secondary-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("grannibal-secondary-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(-30.0f, -30.0f)*direction;

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "grannibal-secondary-projectile-trail"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("grannibal-secondary-projectile-trail", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 16.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        grannibalProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalProjectileEntity, std::move(instance)
    );

    pul::core::ComponentParticleGrenade particle;
    particle.damage.damagePlayer = true;
    particle.damage.ignoredPlayer = playerEntity;
    particle.damage.explosionRadius    = config::ProjectileExplosionRadius();
    particle.damage.explosionForce     = config::ProjectileExplosionForce();
    particle.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
    particle.damage.playerDirectDamage = config::ProjectileDirectDamage();

    plugin.animation.ConstructInstance(
      scene, particle.animationInstance, scene.AnimationSystem()
    , "grannibal-hit"
    );

    particle
      .animationInstance
      .pieceToState["particle"].Apply("grannibal-hit", true);

    particle.origin = instance.origin;
    particle.velocity = direction*config::ProjectileVelocity();
    particle.velocityFriction = config::ProjectileVelocityFriction();
    particle.gravityAffected = true;
    particle.bounces = config::Bounces();
    particle.useBounces = true;

    registry.emplace<pul::core::ComponentParticleGrenade>(
      grannibalProjectileEntity, std::move(particle)
    );
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireDopplerBeam(
  bool const primary, bool const secondary
, [[maybe_unused]] glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & dopplerBeamInfo =
    std::get<pul::core::WeaponInfo::WiDopplerBeam>(weaponInfo.info);

  namespace config = plugin::config::dopplerBeam::primary;
  namespace configSec = plugin::config::dopplerBeam::secondary;

  if (dopplerBeamInfo.dischargingTimer > 0.0f) {
    dopplerBeamInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    dopplerBeamInfo.dischargingTimer = config::DischargeCooldown();
    plugin::entity::FireDopplerBeamPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }

  if (secondary) {
    dopplerBeamInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireDopplerBeamSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireDopplerBeamPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, [[maybe_unused]] pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::dopplerBeam::primary;

  {
    auto dopplerBeamFireEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      dopplerBeamFireEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "doppler-beam-fire"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("doppler-beam-fire", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      dopplerBeamFireEntity, std::move(instance)
    );
  }

  {
    auto dopplerBeamProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "doppler-beam-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("doppler-beam-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    registry.emplace<pul::animation::ComponentInstance>(
      dopplerBeamProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      dopplerBeamProjectileEntity
    , instance.origin, direction * config::ProjectileVelocity(), false, true
    );

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "doppler-beam-projectile-trail"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("doppler-beam-projectile-trail", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 1.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        dopplerBeamProjectileEntity, std::move(emitter)
      );
    }

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;
    exploder.damage.damagePlayer = true;
    exploder.damage.ignoredPlayer = playerEntity;
    exploder.damage.explosionRadius = 0.0f;
    exploder.damage.explosionForce = config::ProjectileForce();
    exploder.damage.playerSplashDamage = 0.0f;
    exploder.damage.playerDirectDamage = config::ProjectileDamage();

    plugin.animation.ConstructInstance(
      scene, exploder.animationInstance, scene.AnimationSystem()
    , "doppler-beam-hit"
    );

    exploder
      .animationInstance
      .pieceToState["particle"].Apply("doppler-beam-hit", true);

    registry.emplace<pul::core::ComponentParticleExploder>(
      dopplerBeamProjectileEntity, std::move(exploder)
    );
  }
}

void plugin::entity::FireDopplerBeamSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin
, [[maybe_unused]] glm::vec2 const & direction
, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {

  namespace config = plugin::config::dopplerBeam::secondary;

  for (auto fireAngle : config::ShotPattern()) {
    fireAngle += angle;
    auto dir = glm::vec2(glm::sin(fireAngle), glm::cos(fireAngle));
    plugin::entity::FireDopplerBeamPrimary(
      plugin, scene, weaponInfo, origin, dir, fireAngle, flip, matrix
    , playerEntity
    );
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFirePericaliya(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & pericaliyaInfo =
    std::get<pul::core::WeaponInfo::WiPericaliya>(weaponInfo.info);

  namespace config = plugin::config::pericaliya::primary;
  namespace configSec = plugin::config::pericaliya::secondary;

  if (pericaliyaInfo.dischargingTimer > 0.0f) {
    pericaliyaInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (pericaliyaInfo.isPrimaryActive) {
    if (!primary) {
      pericaliyaInfo.isPrimaryActive = false;
      pericaliyaInfo.dischargingTimer = config::DischargeCooldown();
    }
    return;
  }

  if (pericaliyaInfo.isSecondaryActive) {
    if (!secondary) {
      pericaliyaInfo.isSecondaryActive = false;
      pericaliyaInfo.dischargingTimer = configSec::DischargeCooldown();
    }
    return;
  }

  if (primary) {
    pericaliyaInfo.isPrimaryActive = true;
    plugin::entity::FirePericaliyaPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }

  if (secondary) {
    pericaliyaInfo.isSecondaryActive = true;
    plugin::entity::FirePericaliyaSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FirePericaliyaPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::pericaliya::primary;

  auto & pericaliyaInfo =
    std::get<pul::core::WeaponInfo::WiPericaliya>(weaponInfo.info);

  {
    auto pericaliyaMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      pericaliyaMuzzleEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "pericaliya-muzzle"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("pericaliya-muzzle", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      pericaliyaMuzzleEntity, std::move(instance)
    );
  }

  {
    auto pericaliyaProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "pericaliya-primary-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("pericaliya-primary-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(-20.0f, -20.0f)*direction;

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "pericaliya-primary-projectile-trail"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("pericaliya-primary-projectile-trail", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 16.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        pericaliyaProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      pericaliyaProjectileEntity, std::move(instance)
    );

    bool hasBeenActive = false;

    registry.emplace<pul::core::ComponentParticle>(
      pericaliyaProjectileEntity
    , instance.origin, direction, false, false
    , [&direction, &pericaliyaInfo, hasBeenActive]
        (glm::vec2 & vel) mutable -> void
      {
        if (pericaliyaInfo.isPrimaryActive && !hasBeenActive) {
          vel = direction*config::ProjectileVelocity();
        }

        // disable for this projectile
        if (!hasBeenActive && !pericaliyaInfo.isPrimaryActive) {
          hasBeenActive = true;
        }
      }
    );

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;
    exploder.damage.damagePlayer = true;
    exploder.damage.ignoredPlayer = playerEntity;
    exploder.damage.explosionRadius    = config::ProjectileExplosionRadius();
    exploder.damage.explosionForce     = config::ProjectileExplosionForce();
    exploder.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
    exploder.damage.playerDirectDamage = config::ProjectileDirectDamage();

    plugin.animation.ConstructInstance(
      scene, exploder.animationInstance, scene.AnimationSystem()
    , "pericaliya-primary-explosion"
    );

    exploder
      .animationInstance
      .pieceToState["particle"].Apply("pericaliya-primary-explosion", true);

    registry.emplace<pul::core::ComponentParticleExploder>(
      pericaliyaProjectileEntity, std::move(exploder)
    );
  }
}

void plugin::entity::FirePericaliyaSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::pericaliya::secondary;

  auto & pericaliyaInfo =
    std::get<pul::core::WeaponInfo::WiPericaliya>(weaponInfo.info);

  for (auto fireAngle : config::ShotPattern()) {
    float localFireAngle = fireAngle;
    fireAngle += angle;
    auto dir = glm::vec2(glm::sin(fireAngle), glm::cos(fireAngle));

    {
      auto pericaliyaMuzzleEntity = registry.create();
      registry.emplace<pul::core::ComponentParticle>(
        pericaliyaMuzzleEntity, origin
      );

      pul::animation::Instance instance;
      plugin.animation.ConstructInstance(
        scene, instance, scene.AnimationSystem(), "pericaliya-muzzle"
      );
      auto & state = instance.pieceToState["particle"];
      state.Apply("pericaliya-muzzle", true);
      state.angle = fireAngle;
      state.flip = flip;

      instance.origin = origin + glm::vec2(0.0f, 32.0f);
      instance.automaticCachedMatrixCalculation = false;

      plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

      registry.emplace<pul::animation::ComponentInstance>(
        pericaliyaMuzzleEntity, std::move(instance)
      );
    }

    {
      auto pericaliyaProjectileEntity = registry.create();
      pul::animation::Instance instance;
      plugin.animation.ConstructInstance(
        scene, instance, scene.AnimationSystem()
      , "pericaliya-secondary-projectile"
      );
      auto & state = instance.pieceToState["particle"];
      state.Apply("pericaliya-secondary-projectile", true);
      state.angle = fireAngle;
      state.flip = flip;

      instance.origin = origin - glm::vec2(-20.0f, -20.0f)*dir;

      { // emitter
        pul::core::ComponentDistanceParticleEmitter emitter;

        // -- animation
        plugin.animation.ConstructInstance(
          scene, emitter.animationInstance, scene.AnimationSystem()
        , "pericaliya-secondary-projectile-trail"
        );

        emitter
          .animationInstance
          .pieceToState["particle"]
          .Apply("pericaliya-secondary-projectile-trail", true);

        // -- timer
        emitter.velocity = glm::vec2();
        emitter.originDist = 16.0f;
        emitter.prevOrigin = instance.origin;

        registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
          pericaliyaProjectileEntity, std::move(emitter)
        );
      }

      registry.emplace<pul::animation::ComponentInstance>(
        pericaliyaProjectileEntity, std::move(instance)
      );

      bool hasBeenActive = false;
      float activeTimer = 0.0f;
      registry.emplace<pul::core::ComponentParticle>(
        pericaliyaProjectileEntity
      , instance.origin, dir*config::ProjectileVelocity(), false, false
      , [
          &pericaliyaInfo, hasBeenActive, fireAngle, localFireAngle, activeTimer
        ](
          glm::vec2 & velocity
        ) mutable -> void {
          // check when no longer shooting
          if (!hasBeenActive && !pericaliyaInfo.isSecondaryActive) {
            hasBeenActive = true;

            // only do redirection after 200ms
            if (activeTimer >= config::RedirectionMinimumThreshold()) {
              // redirect so that particles meet in 'middle'
              glm::vec2 newDir =
                glm::vec2(
                  std::sin(fireAngle + localFireAngle*-2.0f)
                , std::cos(fireAngle + localFireAngle*-2.0f)
                );

              velocity = glm::length(velocity) * newDir;
            }
            // TODO add the ring thing
          }

          activeTimer += pul::util::MsPerFrame;
        }
      );

      pul::core::ComponentParticleExploder exploder;
      exploder.explodeOnDelete = true;
      exploder.explodeOnCollide = true;
      exploder.damage.damagePlayer = true;
      exploder.damage.ignoredPlayer = playerEntity;
      exploder.damage.explosionRadius    = config::ProjectileExplosionRadius();
      exploder.damage.explosionForce     = config::ProjectileExplosionForce();
      exploder.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
      exploder.damage.playerDirectDamage = config::ProjectileDirectDamage();

      plugin.animation.ConstructInstance(
        scene, exploder.animationInstance, scene.AnimationSystem()
      , "pericaliya-secondary-explosion"
      );

      exploder
        .animationInstance
        .pieceToState["particle"].Apply("pericaliya-secondary-explosion", true);

      registry.emplace<pul::core::ComponentParticleExploder>(
        pericaliyaProjectileEntity, std::move(exploder)
      );
    }
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireZeusStinger(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, pul::core::ComponentPlayer & player
, pul::animation::Instance & playerAnim
, entt::entity playerEntity
) {
  auto & zeusStingerInfo =
    std::get<pul::core::WeaponInfo::WiZeusStinger>(weaponInfo.info);

  namespace config = plugin::config::zeusStinger::primary;
  namespace configSec = plugin::config::zeusStinger::secondary;

  if (zeusStingerInfo.dischargingTimer > 0.0f) {
    zeusStingerInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    zeusStingerInfo.dischargingTimer = config::DischargeCooldown();
    plugin::entity::FireZeusStingerPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , player, playerAnim
    , playerEntity
    );
  }

  if (secondary) {
    zeusStingerInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireZeusStingerSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireZeusStingerPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, pul::core::ComponentPlayer & player
, pul::animation::Instance & playerAnim
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::zeusStinger::primary;

  auto zeusStingerMuzzleEntity = registry.create();
  { // muzzle
    pul::animation::Instance animInstance;
    plugin.animation.ConstructInstance(
      scene, animInstance, scene.AnimationSystem()
    , "zeus-stinger-primary-beam-muzzle-flash"
    );
    auto & animState = animInstance.pieceToState["particle"];
    animState.Apply("zeus-stinger-primary-beam-muzzle-flash", true);
    animState.flip = flip;
    animInstance.origin = origin + glm::vec2(0.0f, 32.0f);
    animInstance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(animInstance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      zeusStingerMuzzleEntity, std::move(animInstance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      zeusStingerMuzzleEntity
    , animInstance.origin, glm::vec2{}, false, false
    );
  }

  // keep track of begin/end origin for collision detection
  glm::vec2 beginOrigin, endOrigin;

  // end origin before being corrected by secondary hit origin, thus allowing
  // us to determine which side of the projectile was hit
  glm::vec2 originalEndOrigin;

  bool intersection = false;
  bool scatterIntersection = false;

  auto zeusStingerBeamEntity = registry.create();

  { // projectile
    { // animation
      pul::animation::Instance animInstance;
      plugin.animation.ConstructInstance(
        scene, animInstance, scene.AnimationSystem()
      , "zeus-stinger-primary-beam"
      );
      auto & animState = animInstance.pieceToState["particle"];
      animState.Apply("zeus-stinger-primary-beam", true);

      // -- update animation origin/direction
      animState.flip = flip;
      animInstance.origin = origin + glm::vec2(0.0f, 32.0f);
      animInstance.automaticCachedMatrixCalculation = false;

      plugin.animation.UpdateCacheWithPrecalculatedMatrix(animInstance, matrix);

      registry.emplace<pul::animation::ComponentInstance>(
        zeusStingerBeamEntity, std::move(animInstance)
      );

      registry.emplace<pul::core::ComponentParticle>(
        zeusStingerBeamEntity
      , animInstance.origin, glm::vec2{}, false, false
      );
    }

    { // particle beam
      pul::core::ComponentParticleBeam particle;

      // can't use the instance used to create animation above since its
      // memory has been moved to the entity
      auto & animInstance =
        registry.get<
          pul::animation::ComponentInstance
        >(zeusStingerBeamEntity).instance
      ;

      // -- update animation clipping
      beginOrigin =
          animInstance.origin
        + glm::vec2(
              matrix
            * glm::vec3(0.0f, 0.0f, 1.0f)
          )
      ;

      endOrigin =
          animInstance.origin
        + glm::vec2(
              matrix * glm::vec3(flip ? 15000.0f : -15000.0f, 0.0f, 1.0f)
          )
      ;

      auto beamRay =
        pul::physics::IntersectorRay::Construct(
          beginOrigin
        , endOrigin
        );
      if (
        pul::physics::IntersectionResults resultsBeam;
        plugin.physics.IntersectionRaycast(scene, beamRay, resultsBeam)
      ) {
        intersection = true;
        endOrigin = resultsBeam.origin;
      }

      registry.emplace<pul::core::ComponentParticleBeam>(
        zeusStingerBeamEntity, std::move(particle)
      );
    }
  }

  // collision detection with nearest entity (including zeus stinger secondary)
  {
    auto view =
      registry.view<
        pul::animation::ComponentInstance
      , ::ComponentZeusStingerSecondary
      >();

    entt::entity nearestEntity;

    // find the nearest sphere, and then record the entity/distance which can
    // then be compared to nearest hitbox entity & the sphere removed if a
    // collision occured
    float nearestDist = 50000.0f;
    auto const rayDirection = glm::normalize(endOrigin - beginOrigin);
    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);

      float dist;
      if (
        glm::intersectRaySphere(
          beginOrigin, rayDirection
        , animation.instance.origin, 20.0f*20.0f
        , dist
        )
      && dist < glm::length(endOrigin - beginOrigin)
      && dist < nearestDist
      ) {
        nearestEntity = entity;
        nearestDist = dist;

        endOrigin = animation.instance.origin; // endOrigin center of ball
        originalEndOrigin = beginOrigin + rayDirection*dist;
        intersection = true;
        scatterIntersection = true;
      }
    }

    // apply weapon damage
    auto weaponDamageInfo =
      plugin::entity::WeaponDamageRaycast(
        plugin, scene
      , beginOrigin
      , endOrigin
      , config::ProjectileDamage()
      , config::ProjectileForce()
      , playerEntity // ignored player
      )
    ;

    if (weaponDamageInfo.entity != entt::null) {
      // no scatter intersection as player was hit before the nearest sphere
      scatterIntersection = false;
      intersection = true;

      endOrigin = originalEndOrigin = weaponDamageInfo.origin;
    }

    // if intersection, destroy entity & reorient the beam
    if (scatterIntersection) {
      registry.destroy(nearestEntity);

      // TODO reorient beam to align
    }
  }

  // apply clipping if required by intersection; make sure all intersection
  // tests happen before this
  if (intersection)
  {
    auto & animInstance =
      registry.get<
        pul::animation::ComponentInstance
      >(zeusStingerBeamEntity).instance
    ;
    auto & animState = animInstance.pieceToState["particle"];

    // -- apply clipping
    float clipLength =
        glm::length(glm::vec2(beginOrigin) - glm::vec2(endOrigin));

    // clip a bit out of projectile
    if (scatterIntersection) { clipLength -= 16.0f; }

    // TODO don't hardcode
    // we are offsetted by muzzle width / 128
    animState.uvCoordWrap.x = glm::max(0.0f, (clipLength - 128.0f) / 544.0f);
    animState.vertWrap.x = animState.uvCoordWrap.x;
    if (!flip) {
      animState.flipVertWrap = true;
    }

    if (clipLength < 128.0f)
    { // muzzle clipping
      auto & muzzleAnimInstance =
        registry.get<
          pul::animation::ComponentInstance
        >(zeusStingerMuzzleEntity).instance
      ;
      auto & muzzleAnimState = muzzleAnimInstance.pieceToState["particle"];

      // TODO don't hardcode
      muzzleAnimState.uvCoordWrap.x = clipLength / 128.0f;
      muzzleAnimState.vertWrap.x = muzzleAnimState.uvCoordWrap.x;
      if (!flip) { muzzleAnimState.flipVertWrap = true; }
    }

    { // explosion
      auto zeusStingerExplosionEntity = registry.create();
      registry.emplace<pul::core::ComponentParticle>(
        zeusStingerExplosionEntity, endOrigin
      );

      auto const explosionStr =
          scatterIntersection
        ? "zeus-stinger-secondary-beam-explosion"
        : "zeus-stinger-primary-explosion"
      ;

      pul::animation::Instance instance;
      plugin.animation.ConstructInstance(
        scene, instance, scene.AnimationSystem(), explosionStr
      );
      auto & state = instance.pieceToState["particle"];
      state.Apply(explosionStr, true);
      state.angle = 0.0f;
      state.flip = flip;

      instance.origin = endOrigin;

      registry.emplace<pul::animation::ComponentInstance>(
        zeusStingerExplosionEntity, std::move(instance)
      );
    }
  }

  // apply scattered beam iff the secondary scatter ball was hit
  if (scatterIntersection)
  { // scatter beam
    auto scatterBeamEntity = registry.create();
    { // animation
      pul::animation::Instance animInstance;
      plugin.animation.ConstructInstance(
        scene, animInstance, scene.AnimationSystem()
      , "zeus-stinger-scatter-beam"
      );
      auto & animState = animInstance.pieceToState["particle"];
      animState.Apply("zeus-stinger-scatter-beam", true);

      // -- update animation origin/direction
      animState.flip = flip;
      animState.angle = angle + pul::Pi;
      animInstance.origin = endOrigin;

      registry.emplace<pul::animation::ComponentInstance>(
        scatterBeamEntity, std::move(animInstance)
      );

      registry.emplace<pul::core::ComponentParticle>(
        scatterBeamEntity
      , animInstance.origin, glm::vec2{}, false, false
      );
    }

    // reflect ray on the 'normal' of the circle; the normal is the plane at
    // which we intersect the ray, thus we only have  right-angle intersections.
    // By taking the difference between the 'corrected' intersection origin
    // (center of circle) and the original intersection origin (border of
    // circle), we can determine which side of the plane it is facing
    float const cosTheta =
      glm::dot(
        glm::normalize(originalEndOrigin - endOrigin)
      , glm::vec2(glm::cos(-angle), glm::sin(-angle))
      );
    bool reflect90 = cosTheta < 0.0f;
    float scatterAngle = reflect90 ? -angle : -angle + pul::Pi;
    auto dir = glm::vec2(glm::cos(scatterAngle), glm::sin(scatterAngle));

    // have to encode reflection into matrix
    glm::mat3 const scatterMatrix =
      glm::rotate(glm::mat3(1.0f), scatterAngle);

    plugin::entity::FireZeusStingerPrimary(
      plugin, scene, weaponInfo
    , endOrigin - glm::vec2(0.0f, 32.0f), dir, scatterAngle, false
    , scatterMatrix
    , player, playerAnim
    , playerEntity
    );
  }
}

void plugin::entity::FireZeusStingerSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::zeusStinger::secondary;

  {
    auto zeusStingerProjectileEntity = registry.create();

    // tag this as zeus stinger
    registry.emplace<ComponentZeusStingerSecondary>(
      zeusStingerProjectileEntity
    );

    pul::animation::Instance instance;

    { // create animation
      plugin.animation.ConstructInstance(
        scene, instance, scene.AnimationSystem()
      , "zeus-stinger-secondary-projectile"
      );
      auto & state = instance.pieceToState["particle"];
      state.Apply("zeus-stinger-secondary-projectile", true);
      state.angle = angle;
      state.flip = flip;

      instance.origin = origin - glm::vec2(0.0f, 8.0f);

      registry.emplace<pul::animation::ComponentInstance>(
        zeusStingerProjectileEntity, std::move(instance)
      );
      }

    {
      pul::core::ComponentParticleGrenade particle;
      particle.damage.damagePlayer = true;
      particle.damage.ignoredPlayer = playerEntity;
      particle.damage.explosionRadius    = config::ProjectileExplosionRadius();
      particle.damage.explosionForce     = config::ProjectileExplosionForce();
      particle.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
      particle.damage.playerDirectDamage = config::ProjectileDirectDamage();

      plugin.animation.ConstructInstance(
        scene, particle.animationInstance, scene.AnimationSystem()
      , "zeus-stinger-secondary-explosion"
      );

      particle
        .animationInstance
        .pieceToState["particle"]
        .Apply("zeus-stinger-secondary-explosion", true);

      particle.origin = instance.origin;
      particle.velocity = direction*config::ProjectileVelocity();
      particle.velocityFriction = config::ProjectileVelocityFriction();
      particle.gravityAffected = false;
      particle.useBounces = false;
      particle.timer = config::ProjectileLifetime();
      particle.bounceAnimation = "zeus-stinger-secondary-projectile-trail";

      registry.emplace<pul::core::ComponentParticleGrenade>(
        zeusStingerProjectileEntity, std::move(particle)
      );
    }
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireBadFetus(
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
) {
  auto & badFetusInfo =
    std::get<pul::core::WeaponInfo::WiBadFetus>(weaponInfo.info);

  namespace config = plugin::config::badFetus::primary;
  namespace configSec = plugin::config::badFetus::secondary;

  if (badFetusInfo.dischargingTimer > 0.0f) {
    badFetusInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (badFetusInfo.primaryActive) {
    if (primary) { return; }

    badFetusInfo.primaryActive = false;
    badFetusInfo.dischargingTimer = config::DischargeCooldown();
  }

  if (primary) {
    badFetusInfo.primaryActive = true;
    plugin::entity::FireBadFetusPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix, player
    , playerOrigin, playerAnim
    , playerEntity
    );
  }

  if (secondary) {
    badFetusInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireBadFetusSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireBadFetusPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::animation::Instance & playerAnim
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::badFetus::primary;

  { // muzzle
    auto badFetusMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      badFetusMuzzleEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "bad-fetus-primary-muzzle-flash"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-primary-muzzle-flash", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusMuzzleEntity, std::move(instance)
    );
  }

  // projectile
  auto badFetusBeamEntity = registry.create();

  { // animation
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "bad-fetus-primary-beam"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-primary-beam", true);
    instance.origin = origin;
    state.flip = flip;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusBeamEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      badFetusBeamEntity
    , instance.origin, glm::vec2{}, false, false
    );
  }

  { // particle beam
    pul::core::ComponentParticleBeam particle;
    particle.update =
      [&registry, &scene, &plugin, &player, &playerOrigin, &playerAnim
      , &weaponInfo, playerEntity
      ](
        pul::animation::Instance & animInstance, float & weaponCooldown
      ) -> bool {

        { // check if beam should be destroyed
          auto const * const badFetusInfo =
            std::get_if<pul::core::WeaponInfo::WiBadFetus>(&weaponInfo.info);

          if (!badFetusInfo || !badFetusInfo->primaryActive)
            { return true; }
        }

        // -- update animation origin/direction
        auto const & weaponState =
          playerAnim
            .pieceToState["weapon-placeholder"];

        bool const weaponFlip = playerAnim.pieceToState["legs"].flip;

        animInstance.origin = playerOrigin + glm::vec2(0.0f, 32.0f);

        auto & animState = animInstance.pieceToState["particle"];
        animState.flip = weaponFlip;

        auto const & weaponMatrix = weaponState.cachedLocalSkeletalMatrix;
        plugin
          .animation
          .UpdateCacheWithPrecalculatedMatrix(animInstance, weaponMatrix);

        // -- update animation clipping
        animState.uvCoordWrap.x = 1.0f;
        animState.vertWrap.x = 1.0f;
        animState.flipVertWrap = false;

        auto const beginOrigin =
            animInstance.origin
          + glm::vec2(
                weaponMatrix
              * glm::vec3(0.0f, 0.0f, 1.0f)
            )
        ;

        // I could maybe use the animState matrix here instead of weapon
        auto endOrigin =
            animInstance.origin
          + glm::vec2(
                weaponMatrix
              * glm::vec3(weaponFlip ? 384.0f : -384.0f, 0.0f, 1.0f)
            )
        ;

        bool hasHit = false;
        auto beamRay =
          pul::physics::IntersectorRay::Construct(
            beginOrigin
          , endOrigin
          );
        if (
          pul::physics::IntersectionResults resultsBeam;
          plugin.physics.IntersectionRaycast(scene, beamRay, resultsBeam)
        ) {
          endOrigin = resultsBeam.origin;
          hasHit = true;
        }

        // when applying direct damage, we only apply damage whenever
        // weaponCooldown is finished
        auto weaponDamageInfo =
          plugin::entity::WeaponDamageRaycast(
            plugin, scene
          , beginOrigin, endOrigin
          , weaponCooldown <= 0.0f ? config::ProjectileDamage() : 0.0f
          , config::ProjectileForce() // force
          , playerEntity // ignored player
          )
        ;

        if (weaponDamageInfo.entity != entt::null) {
          endOrigin = weaponDamageInfo.origin;
          hasHit = true;

          // only reset when direct damage was done, which we know based off
          // the same conditions that were used to apply direct damage
          if (weaponCooldown <= 0.0f) {
            weaponCooldown = config::ProjectileCooldown();
          }
        }

        weaponCooldown -= pul::util::MsPerFrame;

        if (hasHit) {
          // apply clipping
          animState.uvCoordWrap.x =
            glm::length(
              glm::vec2(beginOrigin)
            - glm::vec2(endOrigin)
            ) / 384.0f;
          animState.vertWrap.x = animState.uvCoordWrap.x;
          if (!weaponFlip) {
            animState.flipVertWrap = true;
          }

          { // hit trail
            auto bigFetusTrailEntity = registry.create();
            registry.emplace<pul::core::ComponentParticle>(
              bigFetusTrailEntity, endOrigin
            );

            pul::animation::Instance instance;
            plugin.animation.ConstructInstance(
              scene, instance, scene.AnimationSystem()
            , "bad-fetus-primary-hit-trail"
            );
            auto & state = instance.pieceToState["particle"];
            state.Apply("bad-fetus-primary-hit-trail", true);

            // origin is where we collided but a few pixels towards player

            auto const dir =
                glm::vec2(beginOrigin)
              - glm::vec2(endOrigin)
            ;

            instance.origin =
              glm::vec2(endOrigin) + 2.0f*(dir/glm::length(dir))
            ;

            registry.emplace<pul::animation::ComponentInstance>(
              bigFetusTrailEntity, std::move(instance)
            );
          }
        }

        // collision detection with nearest bad fetus secondary
        bool intersection = false;
        {
          auto view =
            registry.view<
              pul::animation::ComponentInstance
            , ::ComponentBadFetusSecondary
            >();
          entt::entity nearestEntity;
          float nearestDist = 5000.0f;
          for (auto entity : view) {
            auto & animation =
              view.get<pul::animation::ComponentInstance>(entity);

            float dist;
            auto const rayDirection = glm::normalize(endOrigin - beginOrigin);
            if (
              glm::intersectRaySphere(
                beginOrigin, rayDirection
              , animation.instance.origin, 20.0f*20.0f
              , dist
              )
            && dist < glm::length(endOrigin - beginOrigin)
            && dist < nearestDist
            ) {
              nearestEntity = entity;
              nearestDist = dist;

              endOrigin = animation.instance.origin; // endOrigin center of ball
              intersection = true;
            }
          }

          // if intersection, destroy both entities & create linkedball entity
          if (intersection) {
            CreateBadFetusLinkedBeam(
              plugin, scene, player, playerOrigin, playerAnim, weaponInfo
            , endOrigin, playerEntity
            );

            registry.destroy(nearestEntity);
            return true;
          }
        }

        return false;
      };

    registry.emplace<pul::core::ComponentParticleBeam>(
      badFetusBeamEntity, std::move(particle)
    );
  }
}

void plugin::entity::FireBadFetusSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::badFetus::secondary;

  { // muzzle
    auto badFetusMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      badFetusMuzzleEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "bad-fetus-primary-muzzle-flash"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-primary-muzzle-flash", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusMuzzleEntity, std::move(instance)
    );
  }

  { // projectile
    auto badFetusProjectileEntity = registry.create();

    registry.emplace<ComponentBadFetusSecondary>(
      badFetusProjectileEntity
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "bad-fetus-secondary-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("bad-fetus-secondary-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    registry.emplace<pul::animation::ComponentInstance>(
      badFetusProjectileEntity, std::move(instance)
    );

    {
      pul::core::ComponentParticleGrenade particle;
      particle.damage.damagePlayer = true;
      particle.damage.ignoredPlayer = playerEntity;
      particle.damage.explosionRadius    = config::ProjectileExplosionRadius();
      particle.damage.explosionForce     = config::ProjectileExplosionForce();
      particle.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
      particle.damage.playerDirectDamage = config::ProjectileDirectDamage();

      plugin.animation.ConstructInstance(
        scene, particle.animationInstance, scene.AnimationSystem()
      , "bad-fetus-explosion"
      );

      particle
        .animationInstance
        .pieceToState["particle"]
        .Apply("bad-fetus-explosion", true);

      particle.origin = instance.origin;
      particle.velocity = direction*config::ProjectileVelocity();
      particle.velocityFriction = config::ProjectileVelocityFriction();
      particle.gravityAffected = true;
      particle.useBounces = false;
      particle.timer = config::ProjectileLifetime();
      particle.bounceAnimation = "bad-fetus-secondary-projectile-bounce";

      registry.emplace<pul::core::ComponentParticleGrenade>(
        badFetusProjectileEntity, std::move(particle)
      );
    }

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "bad-fetus-secondary-projectile-trail"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("bad-fetus-secondary-projectile-trail", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 32.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        badFetusProjectileEntity, std::move(emitter)
      );
    }
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFirePMF(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

void plugin::entity::FirePMFPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

void plugin::entity::FirePMFSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireUnarmed(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

void plugin::entity::FireUnarmedPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

void plugin::entity::FireUnarmedSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireManshredder(
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
) {
  auto & manshredderInfo =
    std::get<pul::core::WeaponInfo::WiManshredder>(weaponInfo.info);

  namespace config = plugin::config::manshredder::primary;
  namespace configSec = plugin::config::manshredder::secondary;

  if (manshredderInfo.dischargingTimer > 0.0f) {
    manshredderInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (manshredderInfo.isPrimaryActive) {
    if (!primary) {
      manshredderInfo.isPrimaryActive = false;
      manshredderInfo.dischargingTimer = config::DischargeCooldown();
    }
    return;
  }

  if (primary) {
    manshredderInfo.isPrimaryActive = true;
    manshredderInfo.dischargingTimer = config::DischargeCooldown();
    plugin::entity::FireManshredderPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , player, playerOrigin, playerAnim
    , playerEntity
    );
  }

  if (secondary) {
    manshredderInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireManshredderSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireManshredderPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, pul::core::ComponentPlayer & player
, glm::vec2 & playerOrigin
, pul::animation::Instance & playerAnim
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::manshredder::primary;

  auto & manshredderInfo =
    std::get<pul::core::WeaponInfo::WiManshredder>(weaponInfo.info);

  {
    auto manshredderProjectileEntity = registry.create();

    { // animation
      pul::animation::Instance instance;
      plugin.animation.ConstructInstance(
        scene, instance, scene.AnimationSystem(), "manshredder-primary-fire"
      );
      auto & state = instance.pieceToState["particle"];
      state.Apply("manshredder-primary-fire", true);
      state.angle = angle;
      state.flip = flip;

      instance.origin = origin - glm::vec2(-20.0f, -20.0f)*direction;

      registry.emplace<pul::animation::ComponentInstance>(
        manshredderProjectileEntity, std::move(instance)
      );
    }

    // TODO this should probably be its own component
    registry.emplace<pul::core::ComponentHitscanProjectile>(
      manshredderProjectileEntity
    , &playerAnim
    , [
        &plugin, &scene, manshredderProjectileEntity, &registry
      , &manshredderInfo, &player, &playerOrigin, playerEntity
      ]
        (pul::core::ComponentHitscanProjectile & projectile) -> bool
      {
        if (!manshredderInfo.isPrimaryActive) { return true; }

        glm::vec2 origin;
        glm::vec2 direction;

        auto & animation =
          registry.get<pul::animation::ComponentInstance>(
            manshredderProjectileEntity
          ).instance;
        auto & state = animation.pieceToState.at("particle");

        { // update origin/animation
          animation.origin = playerOrigin + glm::vec2(0.0f, 28.0f);
          state.flip = player.flip;

          origin = playerOrigin - glm::vec2(0.0f, 12.0f);
          direction =
            glm::vec2(
              glm::sin(player.lookAtAngle), glm::cos(player.lookAtAngle)
            );
        }

        if (state.label != "manshredder-primary-hit")
        { // update hit
          auto ray =
            pul::physics::IntersectorRay::Construct(
              origin, origin+direction*config::ProjectileDistance()
            );
          float dist = config::ProjectileDistance();
          bool hasHit = false;
          if (
            pul::physics::IntersectionResults results;
            plugin.physics.IntersectionRaycast(scene, ray, results)
          ) {
            hasHit = true;
            dist = glm::length(glm::vec2(results.origin) - origin);
          }

          // apply weapon damage, clamped by previous environment check
          hasHit |=
            plugin::entity::WeaponDamageRaycast(
              plugin, scene
            , origin
            , origin + direction*dist
            , config::ProjectileDamage()
            , config::ProjectileForce()
            , playerEntity // ignored player
            ).entity != entt::null
          ;

          state.Apply(
            hasHit ? "manshredder-primary-hit" : "manshredder-primary-fire"
          );

        } else {
          if (state.animationFinished) {
            state.Apply("manshredder-primary-fire");
          }
        }

        return false;
      }
    );
  }
}

void plugin::entity::FireManshredderSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::manshredder::secondary;

  { // muzzle flash
    auto manshredderFireEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      manshredderFireEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "manshredder-secondary-fire"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("manshredder-secondary-fire", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      manshredderFireEntity, std::move(instance)
    );
  }

  { // projectile
    auto manshredderProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "manshredder-secondary-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("manshredder-secondary-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    { // emitter
      pul::core::ComponentDistanceParticleEmitter emitter;

      // -- animation
      plugin.animation.ConstructInstance(
        scene, emitter.animationInstance, scene.AnimationSystem()
      , "manshredder-secondary-projectile"
      );

      emitter
        .animationInstance
        .pieceToState["particle"]
        .Apply("manshredder-secondary-projectile", true);

      // -- timer
      emitter.velocity = glm::vec2();
      emitter.originDist = 6.0f;

      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentDistanceParticleEmitter>(
        manshredderProjectileEntity, std::move(emitter)
      );
    }


    registry.emplace<pul::animation::ComponentInstance>(
      manshredderProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      manshredderProjectileEntity
    , instance.origin, direction * config::ProjectileVelocity(), false, false
    );

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = false;
    exploder.explodeOnCollide = true;
    exploder.damage.damagePlayer = true;
    exploder.damage.ignoredPlayer = playerEntity;
    exploder.damage.explosionRadius    = config::ProjectileExplosionRadius();
    exploder.damage.explosionForce     = config::ProjectileExplosionForce();
    exploder.damage.playerSplashDamage = config::ProjectileSplashDamageMax();
    exploder.damage.playerDirectDamage = config::ProjectileDirectDamage();

    plugin.animation.ConstructInstance(
     scene, exploder.animationInstance, scene.AnimationSystem()
    , "manshredder-secondary-hit"
    );

    exploder
      .animationInstance
      .pieceToState["particle"].Apply("manshredder-secondary-hit", true);

    registry.emplace<pul::core::ComponentParticleExploder>(
      manshredderProjectileEntity, std::move(exploder)
    );
  }
}

// -----------------------------------------------------------------------------

void plugin::entity::PlayerFireWallbanger(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & wallbangerInfo =
    std::get<pul::core::WeaponInfo::WiWallbanger>(weaponInfo.info);

  namespace config = plugin::config::wallbanger::primary;
  namespace configSec = plugin::config::wallbanger::secondary;

  if (wallbangerInfo.dischargingTimer > 0.0f) {
    wallbangerInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    wallbangerInfo.dischargingTimer = config::DischargeCooldown();
    plugin::entity::FireWallbangerPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }

  if (secondary) {
    wallbangerInfo.dischargingTimer = configSec::DischargeCooldown();
    plugin::entity::FireWallbangerSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    , playerEntity
    );
  }
}

void plugin::entity::FireWallbangerPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::wallbanger::primary;

  { // muzzle
    auto wallbangerMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      wallbangerMuzzleEntity, origin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "wallbanger-primary-muzzle"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("wallbanger-primary-muzzle", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerMuzzleEntity, std::move(instance)
    );
  }

  { // projectile
    auto wallbangerProjectileEntity = registry.create();
    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "wallbanger-primary-projectile"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("wallbanger-primary-projectile", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerProjectileEntity, std::move(instance)
    );

    {
      pul::core::ComponentParticleGrenade particle;
      particle.damage.damagePlayer = true;
      particle.damage.ignoredPlayer = playerEntity;
      particle.damage.explosionRadius = 0.0f;
      particle.damage.explosionForce = config::ProjectileForce();
      particle.damage.playerSplashDamage = 0.0f;
      particle.damage.playerDirectDamage = config::ProjectileDirectDamage();

      plugin.animation.ConstructInstance(
        scene, particle.animationInstance, scene.AnimationSystem()
      , "wallbanger-primary-explosion"
      );

      particle
        .animationInstance
        .pieceToState["particle"]
        .Apply("wallbanger-primary-explosion", true);

      particle.origin = instance.origin;
      particle.velocity = direction*config::ProjectileVelocity();
      particle.velocityFriction = 1.0f;
      particle.gravityAffected = false;
      particle.bounces = 1u;
      particle.useBounces = true;
      particle.bounceAnimation = "wallbanger-primary-projectile-bounce";

      registry.emplace<pul::core::ComponentParticleGrenade>(
        wallbangerProjectileEntity, std::move(particle)
      );
    }
  }
}

void plugin::entity::FireWallbangerSecondary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
, entt::entity playerEntity
) {
  auto & registry = scene.EnttRegistry();

  namespace config = plugin::config::wallbanger::secondary;

  { // big muzzle
    auto wallbangerMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      wallbangerMuzzleEntity, origin, direction*3.0f
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "wallbanger-secondary-muzzle-big"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("wallbanger-secondary-muzzle-big", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerMuzzleEntity, std::move(instance)
    );
  }

  { // small muzzle
    auto wallbangerMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      wallbangerMuzzleEntity, origin, direction
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem()
    , "wallbanger-secondary-muzzle-small"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("wallbanger-secondary-muzzle-small", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerMuzzleEntity, std::move(instance)
    );
  }

  // keep track of begin/end origin for collision detection
  glm::vec2 beginOrigin, endOrigin;

  auto wallbangerBeamEntity = registry.create();

  // -- find wall intersection
  beginOrigin =
      origin + glm::vec2(0.0f, 32.0f)
    + glm::vec2(
          matrix
        * glm::vec3(0.0f, 0.0f, 1.0f)
      )
  ;

  // -- intersect wall
  // intersect wall then air/non-wall, but if the gap is not large then skip it
  for (size_t i = 0; i < 3u; ++ i) {
    endOrigin = beginOrigin + direction*5000.0f;
    auto beamRay =
      pul::physics::IntersectorRay::Construct(beginOrigin, endOrigin);
    if (
      pul::physics::IntersectionResults resultsBeam;
      !plugin.physics.IntersectionRaycast(scene, beamRay, resultsBeam)
    ) {
      return;
    } else {
      beginOrigin = glm::vec2(resultsBeam.origin) + direction;
    }

    endOrigin = beginOrigin + direction*5000.0f;

    // -- intersect air/non-wall
    beamRay = pul::physics::IntersectorRay::Construct(beginOrigin, endOrigin);
    if (
      pul::physics::IntersectionResults resultsBeam;
      plugin.physics.InverseSceneIntersectionRaycast(scene, beamRay, resultsBeam)
    ) {
      endOrigin = glm::vec2(resultsBeam.origin);
    }

    if (glm::length(beginOrigin - endOrigin) > 8.0f) { break; }

    beginOrigin = endOrigin + direction;
  }

  { // wall muzzle
    auto wallbangerMuzzleEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      wallbangerMuzzleEntity, beginOrigin
    );

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "wallbanger-wall-muzzle"
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply("wallbanger-wall-muzzle", true);
    state.angle = angle;
    state.flip = flip;

    instance.origin = beginOrigin + direction*4.0f;

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerMuzzleEntity, std::move(instance)
    );
  }

  { // projectile
    { // animation
      pul::animation::Instance animInstance;
      plugin.animation.ConstructInstance(
        scene, animInstance, scene.AnimationSystem()
      , "wallbanger-secondary-wall-beam"
      );
      auto & animState = animInstance.pieceToState["particle"];
      animState.Apply("wallbanger-secondary-wall-beam", true);

      // -- update animation origin/direction
      animState.flip = flip;
      animInstance.origin = beginOrigin;
      animState.angle = angle;
      animInstance.automaticCachedMatrixCalculation = true;

      registry.emplace<pul::animation::ComponentInstance>(
        wallbangerBeamEntity, std::move(animInstance)
      );

      registry.emplace<pul::core::ComponentParticle>(
        wallbangerBeamEntity
      , animInstance.origin, glm::vec2{}, false, false
      );
    }

    { // particle beam
      pul::core::ComponentParticleBeam particle;

      registry.emplace<pul::core::ComponentParticleBeam>(
        wallbangerBeamEntity, std::move(particle)
      );
    }
  }

  // apply clipping if required by intersection; make sure all intersection
  // tests happen before this
  auto & animInstance =
    registry.get<
      pul::animation::ComponentInstance
    >(wallbangerBeamEntity).instance
  ;
  auto & animState = animInstance.pieceToState["particle"];

  // -- apply clipping
  float clipLength =
      glm::length(glm::vec2(beginOrigin) - glm::vec2(endOrigin));

  // TODO don't hardcode
  animState.uvCoordWrap.x = glm::max(0.0f, clipLength / 168.0f);
  animState.vertWrap.x = animState.uvCoordWrap.x;
  if (!flip) {
    animState.flipVertWrap = true;
  }

  // damage player
  plugin::entity::WeaponDamageCircle(
    plugin, scene
  , endOrigin, 128.0f
  , config::ProjectileDamage(), config::ProjectileForce()
  , entt::null
  );

  { // explosion
    auto wallbangerExplosionEntity = registry.create();
    registry.emplace<pul::core::ComponentParticle>(
      wallbangerExplosionEntity, endOrigin
    );

    auto const explosionStr = "wallbanger-secondary-explosion";

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), explosionStr
    );
    auto & state = instance.pieceToState["particle"];
    state.Apply(explosionStr, true);
    state.angle = 0.0f;
    state.flip = flip;

    instance.origin = endOrigin;

    registry.emplace<pul::animation::ComponentInstance>(
      wallbangerExplosionEntity, std::move(instance)
    );
  }
}

plugin::entity::WeaponDamageRaycastReturnInfo
plugin::entity::WeaponDamageRaycast(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & originBegin, glm::vec2 const & originEnd
, float const damage, float const force
, entt::entity ignoredEntity
) {
  auto & registry = scene.EnttRegistry();

  pul::physics::IntersectorRay ray;
  ray.beginOrigin = glm::i32vec2(glm::round(originBegin));
  ray.endOrigin = glm::i32vec2(glm::round(originEnd));
  pul::physics::EntityIntersectionResults results;

  plugin::entity::WeaponDamageRaycastReturnInfo ri = {};

  // iterate thru all entity intersections, and if damageable record
  // the damage
  plugin.physics.EntityIntersectionRaycast(scene, ray, results);
  for (auto & entityIntersection : results.entities) {
    auto * damageable =
      registry.try_get<pul::core::ComponentDamageable>(
        std::get<1>(entityIntersection)
      );
    if (!damageable) { continue; }
    if (std::get<1>(entityIntersection) == ignoredEntity) { continue; }

    pul::core::DamageInfo damageInfo;
    { // calculate damage info
      glm::vec2 const dir = glm::normalize(originEnd - originBegin);

      damageInfo.directionForce = dir * force;
      damageInfo.damage = damage;
    }

    damageable->frameDamageInfos.emplace_back(damageInfo);

    // only one entity can be hit with ray (at least for now)
    ri.entity = std::get<1>(entityIntersection);
    ri.origin = std::get<0>(entityIntersection);

    break;
  }

  return ri;
}

bool plugin::entity::WeaponDamageCircle(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, float radius
, float const damage, float const force
, entt::entity ignoredEntity
) {
  auto & registry = scene.EnttRegistry();

  pul::physics::IntersectorCircle circle;
  circle.origin = origin;
  circle.radius = radius;
  pul::physics::EntityIntersectionResults results;

  // iterate thru all entity intersections, and if damageable record
  // the damage
  plugin.physics.EntityIntersectionCircle(scene, circle, results);
  bool hasHit = false;
  for (auto & entityIntersection : results.entities) {
    auto * damageable =
      registry.try_get<pul::core::ComponentDamageable>(
        std::get<1>(entityIntersection)
      );
    if (!damageable) { continue; }

    if (std::get<1>(entityIntersection) == ignoredEntity) { continue; }

    hasHit = true;

    pul::core::DamageInfo damageInfo;
    { // calculate damage info
      glm::vec2 const dir =
        glm::vec2(std::get<0>(entityIntersection)) - origin;

      float const forceRatio = 1.0f - glm::length(dir) / radius;

      damageInfo.directionForce =
        glm::normalize(dir) * forceRatio * force
      ;

      damageInfo.damage = forceRatio * damage;
    }

    damageable->frameDamageInfos.emplace_back(damageInfo);
  }

  return hasHit;
}

#pragma GCC diagnostic pop
