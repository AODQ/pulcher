#include <plugin-entity/weapon.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-core/weapon.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <entt/entt.hpp>

namespace {

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

  if (grannibalInfo.primaryMuzzleTrailLeft > 0) {
    if (grannibalInfo.primaryMuzzleTrailTimer <= 0.0f) {
      ::GrannibalMuzzleTrail(plugin, scene, origin, flip, matrix);
      -- grannibalInfo.primaryMuzzleTrailLeft;
      grannibalInfo.primaryMuzzleTrailTimer = 70.0f;
    }
    grannibalInfo.primaryMuzzleTrailTimer -= pul::util::MsPerFrame;
  }

  if (grannibalInfo.dischargingTimer > 0.0f) {
    grannibalInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    grannibalInfo.dischargingTimer = 1000.0f;
    plugin::entity::FireGrannibalPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }

  if (secondary) {
    grannibalInfo.dischargingTimer = 1000.0f;
    plugin::entity::FireGrannibalSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }
}

void plugin::entity::FireGrannibalPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & registry = scene.EnttRegistry();

  auto & grannibalInfo =
    std::get<pul::core::WeaponInfo::WiGrannibal>(weaponInfo.info);

  grannibalInfo.primaryMuzzleTrailLeft = 4;
  grannibalInfo.primaryMuzzleTrailTimer = 70.0f;

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

    instance.origin = origin - glm::vec2(-30.0f, -30.0f)*direction;

    { // emitter
      pul::core::ComponentParticleEmitter emitter;

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
      emitter.originDist = 10.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentParticleEmitter>(
        grannibalProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      grannibalProjectileEntity
    , instance.origin, direction*10.0f, false, true
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
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const &
) {
  auto & registry = scene.EnttRegistry();

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
      pul::core::ComponentParticleEmitter emitter;

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
      emitter.originDist = 11.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentParticleEmitter>(
        grannibalProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      grannibalProjectileEntity, std::move(instance)
    );

    pul::core::ComponentParticleGrenade particle;

    plugin.animation.ConstructInstance(
      scene, particle.animationInstance, scene.AnimationSystem()
    , "grannibal-hit"
    );

    particle
      .animationInstance
      .pieceToState["particle"].Apply("grannibal-hit", true);

    particle.origin = instance.origin;
    particle.velocity = direction*5.0f;
    particle.bounces = 2u;

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
) {
  auto & dopplerBeamInfo =
    std::get<pul::core::WeaponInfo::WiDopplerBeam>(weaponInfo.info);

  if (dopplerBeamInfo.dischargingTimer > 0.0f) {
    dopplerBeamInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    dopplerBeamInfo.dischargingTimer = 150.0f;
    plugin::entity::FireDopplerBeamPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }

  if (secondary) {
    dopplerBeamInfo.dischargingTimer = 1000.0f;
    plugin::entity::FireDopplerBeamSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }
}

void plugin::entity::FireDopplerBeamPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, [[maybe_unused]] pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & registry = scene.EnttRegistry();

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
    , instance.origin, direction * 5.0f, false, true
    );

    { // emitter
      pul::core::ComponentParticleEmitter emitter;

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

      registry.emplace<pul::core::ComponentParticleEmitter>(
        dopplerBeamProjectileEntity, std::move(emitter)
      );
    }

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;

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
) {
  std::array<float, 9> shotPattern {{
    -0.1f,  0.00f, +0.1f
  }};
  for (auto fireAngle : shotPattern) {
    fireAngle += angle;
    auto dir = glm::vec2(glm::sin(fireAngle), glm::cos(fireAngle));
    plugin::entity::FireDopplerBeamPrimary(
      plugin, scene, weaponInfo, origin, dir, fireAngle, flip, matrix
    );
  }
}

// -----------------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void plugin::entity::PlayerFirePericaliya(
  bool const primary, bool const secondary
, glm::vec2 & velocity
, pul::core::WeaponInfo & weaponInfo
, pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & pericaliyaInfo =
    std::get<pul::core::WeaponInfo::WiPericaliya>(weaponInfo.info);

  if (pericaliyaInfo.dischargingTimer > 0.0f) {
    pericaliyaInfo.dischargingTimer -= pul::util::MsPerFrame;
    return;
  }

  if (primary) {
    pericaliyaInfo.dischargingTimer = 1000.0f;
    plugin::entity::FirePericaliyaPrimary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }

  if (secondary) {
    pericaliyaInfo.dischargingTimer = 1000.0f;
    plugin::entity::FirePericaliyaSecondary(
      plugin, scene, weaponInfo, origin, direction, angle, flip, matrix
    );
  }
}

void plugin::entity::FirePericaliyaPrimary(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::WeaponInfo & weaponInfo
, glm::vec2 const & origin, glm::vec2 const & direction, float const angle
, bool const flip, glm::mat3 const & matrix
) {
  auto & registry = scene.EnttRegistry();

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
      pul::core::ComponentParticleEmitter emitter;

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
      emitter.originDist = 5.0f;
      emitter.prevOrigin = instance.origin;

      registry.emplace<pul::core::ComponentParticleEmitter>(
        pericaliyaProjectileEntity, std::move(emitter)
      );
    }

    registry.emplace<pul::animation::ComponentInstance>(
      pericaliyaProjectileEntity, std::move(instance)
    );

    registry.emplace<pul::core::ComponentParticle>(
      pericaliyaProjectileEntity
    , instance.origin, direction, false, false
    , [&direction](glm::vec2 & vel) {
        vel = direction*4.0f;
      }
    );

    pul::core::ComponentParticleExploder exploder;
    exploder.explodeOnDelete = true;
    exploder.explodeOnCollide = true;

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
) {
}

#pragma GCC diagnostic pop
