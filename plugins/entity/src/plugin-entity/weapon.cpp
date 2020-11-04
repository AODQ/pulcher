#include <plugin-entity/weapon.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-core/weapon.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <entt/entt.hpp>

void plugin::entity::PlayerFireVolnias(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & volInfo = std::get<pul::core::WeaponInfo::WiVolnias>(weaponInfo.info);
  auto & audioSystem = scene.AudioSystem();

  bool forceCooldown = (primary && secondary) || volInfo.dischargingSecondary;

  // TODO use controller or something
  static bool prevPrim = false;

  if (!primary && prevPrim && volInfo.primaryChargeupTimer >= 400.0f) {
    audioSystem.volniasEndPrimary = true;
    weaponInfo.cooldown = 300.0f;
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
    if (!volInfo.hasChargedPrimary && volInfo.primaryChargeupTimer >= 100.0f) {
      volInfo.hasChargedPrimary = true;
      audioSystem.volniasPrefirePrimary = true;
    }
    if (volInfo.primaryChargeupTimer >= 1000.0f) {
      volInfo.primaryChargeupTimer -= 100.0f;

      plugin::entity::FireVolniasPrimary(
        plugin, scene, origin, direction, angle, flip, matrix
      );

      velocity += -direction*0.7f;
    }
  }

  // apply secondary chargeup
  if (secondary && !forceCooldown) {
    if (volInfo.secondaryChargedShots < 5) {
      volInfo.secondaryChargeupTimer += pul::util::MsPerFrame;
      if (volInfo.secondaryChargeupTimer >= 600.0f) {
        volInfo.secondaryChargeupTimer -= 600.0f;
        audioSystem.volniasChargePrimary = true;
        if (++ volInfo.secondaryChargedShots == 5) {
          audioSystem.volniasChargeSecondary = true;
        }
      }
    } else {
      // check if we have to force shoot

      volInfo.secondaryChargeupTimer += pul::util::MsPerFrame;
      if (
          !volInfo.overchargedSecondary
       && volInfo.secondaryChargeupTimer >= 5500.0f
      ) {
        audioSystem.volniasPrefireSecondary = true;
        volInfo.overchargedSecondary = true;
      }

      if (volInfo.secondaryChargeupTimer >= 6000.0f) {
        forceCooldown = true;
      }
    }
  }

  // secondary fires on release
  if (!secondary || forceCooldown) {
    volInfo.secondaryChargeupTimer = 580.0f;
    volInfo.overchargedSecondary = false;
    if (volInfo.secondaryChargedShots > 0u) {
      if (!volInfo.dischargingSecondary) {
        audioSystem.volniasFire = (volInfo.secondaryChargedShots-1)/2;
      }
      volInfo.dischargingSecondary = true;

      volInfo.dischargingTimer += pul::util::MsPerFrame;
      if (volInfo.dischargingTimer > 40.0f) {
        volInfo.dischargingTimer -= 40.0f;
        plugin::entity::FireVolniasSecondary(
          3, volInfo.secondaryChargedShots-1, plugin
        , scene, origin, angle, flip, matrix
        );
        if (--volInfo.secondaryChargedShots == 0u) {
          volInfo.dischargingSecondary = false;
          volInfo.dischargingTimer = 0.0f;
          weaponInfo.cooldown = 300.0f;
        }
      }
    }
  }
}

void plugin::entity::FireVolniasPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {

  auto & registry = scene.EnttRegistry();

  auto & audioSystem = scene.AudioSystem();

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
    , instance.origin, direction * 20.0f
    );


    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;

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
}

void plugin::entity::FireVolniasSecondary(
  uint8_t shots, uint8_t shotSet
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, float const angle
, bool const flip, glm::mat3 const & matrix
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
      plugin, scene, origin, dir, fireAngle, flip, matrix
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
) {
  auto & grannibalInfo =
    std::get<pul::core::WeaponInfo::WiGrannibal>(weaponInfo.info);

  if (grannibalInfo.dischargingTimer > 0.0f) {
    grannibalInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    grannibalInfo.dischargingTimer = 1000.0f;
    plugin::entity::FireGrannibalPrimary(
      plugin, scene, origin, direction, angle, flip, matrix
    );
  }
}

void plugin::entity::FireGrannibalPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & registry = scene.EnttRegistry();

  {
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
    state.angle = angle;
    state.flip = flip;

    instance.origin = origin + glm::vec2(0.0f, 32.0f);
    instance.automaticCachedMatrixCalculation = false;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(instance, matrix);

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalFireEntity, std::move(instance)
    );
  }

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

    instance.origin = origin - glm::vec2(0.0f, 8.0f);

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      grannibalProjectileEntity
    , instance.origin, direction*5.0f
    );

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;

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
  [[maybe_unused]] uint8_t shots, [[maybe_unused]] uint8_t shotSet
, [[maybe_unused]] pul::plugin::Info const & plugin
, [[maybe_unused]] pul::core::SceneBundle & scene
, [[maybe_unused]] glm::vec2 const & origin
, [[maybe_unused]] float const angle
, [[maybe_unused]] bool const flip
, [[maybe_unused]] glm::mat3 const & matrix
) {
}
