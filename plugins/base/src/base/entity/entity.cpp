#include <plugin-base/entity/entity.hpp>

#include <plugin-base/animation/animation.hpp>
#include <plugin-base/bot/bot.hpp>
#include <plugin-base/bot/creature.hpp>
#include <plugin-base/debug/renderer.hpp>
#include <plugin-base/entity/config.hpp>
#include <plugin-base/entity/cursor.hpp>
#include <plugin-base/entity/player.hpp>
#include <plugin-base/entity/weapon.hpp>
#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>

#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/hud.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/pickup.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-core/weapon.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <entt/entt.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/transform2.hpp>
#include <imgui/imgui.hpp>

namespace {

bool botPlays = false;
bool showHitboxRendering = true;

} // -- namespace

void plugin::entity::StartScene(pul::core::SceneBundle & scene) {

  // load config
  plugin::config::LoadConfig();

  plugin::entity::ConstructCursor(scene);

  // player
  entt::entity playerEntity;
  plugin::entity::ConstructPlayer(playerEntity, scene, true);

  // bot/AI
  for (size_t i = 0; i < 1; ++ i) {
    entt::entity botEntity;
    plugin::entity::ConstructPlayer(botEntity, scene, false);
  }
}

void plugin::entity::Shutdown(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  // save config
  plugin::config::SaveConfig();

  // store player
  auto view =
    registry.view<
      pul::core::ComponentPlayerControllable, pul::core::ComponentPlayer
    , pul::core::ComponentCamera, pul::animation::ComponentInstance
    , pul::core::ComponentOrigin
    >();

  for (auto entity : view) {
    // save player component for persistent reloads
    scene.StoredDebugPlayerComponent() =
      std::move(view.get<pul::core::ComponentPlayer>(entity));
    scene.StoredDebugPlayerOriginComponent() =
      std::move(view.get<pul::core::ComponentOrigin>(entity));
  }

  // delete registry
  registry = {};
}

void plugin::entity::Update(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  { // -- projectile exploder
    auto view =
      registry.view<
        pul::animation::ComponentInstance
      , pul::core::ComponentParticleExploder
      , pul::core::ComponentParticle
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & exploder = view.get<pul::core::ComponentParticleExploder>(entity);
      auto & particle = view.get<pul::core::ComponentParticle>(entity);

      bool explode =
          exploder.explodeOnDelete
       && animation.instance.pieceToState["particle"].animationFinished
      ;

      entt::entity playerDirectHit = entt::null;

      glm::vec2 explodeOrigin = particle.origin;

      // check if physics bound
      if (!explode) {
        auto ray =
          pul::physics::IntersectorRay::Construct(
            animation.instance.origin,
            animation.instance.origin + particle.velocity
          );

        if (
          pul::physics::IntersectionResults results;
          plugin::physics::IntersectionRaycast(scene, ray, results)
        ) {
          explodeOrigin = results.origin;
          explode |= exploder.explodeOnCollide;
        }
      }

      // check for player
      if (!explode && exploder.damage.damagePlayer) {
        playerDirectHit =
          plugin::entity::WeaponDamageRaycast(
            scene
          , animation.instance.origin
          , animation.instance.origin + particle.velocity
          , exploder.damage.playerDirectDamage
          , exploder.damage.explosionForce
          , exploder.damage.ignoredPlayer
          ).entity
        ;

        explode |= playerDirectHit != entt::null;
      }

      if (explode) {
        PUL_ASSERT(exploder.animationInstance.animator , continue;);

        auto finAnimation = registry.create();
        exploder.animationInstance.origin = explodeOrigin;
        registry.emplace<pul::animation::ComponentInstance>(
          finAnimation, std::move(exploder.animationInstance)
        );
        registry.emplace<pul::core::ComponentParticle>(
          finAnimation, animation.instance.origin
        );

        if (
            exploder.damage.damagePlayer
         && exploder.damage.explosionRadius > 0.0f
        ) {
          pul::physics::IntersectorCircle circle;
          circle.origin = explodeOrigin;
          circle.radius = exploder.damage.explosionRadius;
          pul::physics::EntityIntersectionResults results;

          plugin::entity::WeaponDamageCircle(
            scene
          , explodeOrigin
          , exploder.damage.explosionRadius
          , exploder.damage.playerSplashDamage
          , exploder.damage.explosionForce
          , playerDirectHit // fine if it's null; don't want to hit player 2x
          );
        }

        if (exploder.audioTrigger) *exploder.audioTrigger = true;

        registry.destroy(entity);
      }
    }
  }

  { // -- particle grenades
    auto view =
      registry.view<
        pul::core::ComponentParticleGrenade
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & particle = view.get<pul::core::ComponentParticleGrenade>(entity);

      bool destroyInstance =
        animation.instance.pieceToState["particle"].animationFinished
      ;

      // negate before comparison so that physics are ran on frame of
      // destruction
      particle.timer -= pul::util::MsPerFrame;
      if (particle.timer <= 0.0f) {
        destroyInstance = true;
      }

      // check physics bounce
      if (particle.velocity != glm::vec2()) {

        if (particle.gravityAffected)
          { particle.velocity.y += 0.05f; }

        auto ray =
          pul::physics::IntersectorRay::Construct(
            animation.instance.origin,
            animation.instance.origin + particle.velocity
          );

        if (
          pul::physics::IntersectionResults results;
          plugin::physics::IntersectionRaycast(scene, ray, results)
        ) {
          // calculate normal (TODO this should be precomputed)
          glm::vec2 normal = glm::vec2(0.0f);

          for (auto point : std::vector<glm::vec2>{
            { -1.0f, -1.0f }, { +0.0f, -1.0f }, { +1.0f, -1.0f }
          , { -1.0f, +0.0f },                   { +1.0f, +0.0f }
          , { -1.0f, +1.0f }, { +0.0f, +1.0f }, { +1.0f, +1.0f }
          }) {
            auto pointInt =
              pul::physics::IntersectorPoint{
                glm::i32vec2(glm::vec2(results.origin) + point)
              };
            if (
              pul::physics::IntersectionResults pointResult;
              plugin::physics::IntersectionPoint(scene, pointInt, pointResult)
            ) {
              normal += point;
            }
          }
          normal = glm::normalize(normal);

          // TODO have to detect normal of wall...
          animation.instance.origin = results.origin;
          particle.origin = results.origin;
          // reflect velocity
          glm::vec2 const targetDirection =
            glm::reflect(glm::normalize(particle.velocity), -normal);
          particle.velocity =
              glm::length(particle.velocity)
            * targetDirection
            * particle.velocityFriction
          ;

          if (particle.useBounces && particle.bounces == 0) {
            particle.velocity = {};
            destroyInstance = true;
          }

          if (
              (!particle.useBounces || particle.bounces != 0)
           && particle.bounceAnimation != ""
          ) {

            pul::animation::Instance bounceAnimation;
            plugin::animation::ConstructInstance(
              scene, bounceAnimation, scene.AnimationSystem()
            , particle.bounceAnimation.c_str()
            );

            bounceAnimation
              .pieceToState["particle"]
              .Apply(particle.bounceAnimation, true);

            bounceAnimation.pieceToState["particle"].angle
              = animation.instance.pieceToState["particle"].angle;

            bounceAnimation.origin = animation.instance.origin;

            auto bounceAnimationEntity = registry.create();

            registry.emplace<pul::animation::ComponentInstance>(
              bounceAnimationEntity, std::move(bounceAnimation)
            );

            registry.emplace<pul::core::ComponentParticle>(
              bounceAnimationEntity, bounceAnimation.origin
            );
          }

          -- particle.bounces;
        }
      }

      entt::entity playerDirectHit = entt::null;

      if (!destroyInstance && particle.damage.damagePlayer) {
        playerDirectHit =
          plugin::entity::WeaponDamageRaycast(
            scene
          , animation.instance.origin
          , animation.instance.origin + particle.velocity
          , particle.damage.playerDirectDamage
          , particle.damage.explosionForce
          , particle.damage.ignoredPlayer
          ).entity
        ;

        destroyInstance |= playerDirectHit != entt::null;
      }


      // TODO fix this
      particle.origin += particle.velocity;
      animation.instance.origin += particle.velocity;

      animation.instance.pieceToState["particle"].angle =
        std::atan2(particle.velocity.x, particle.velocity.y);

      if (destroyInstance) {

        // -- create explosion animation
        auto finAnimation = registry.create();
        particle.animationInstance.origin = animation.instance.origin;
        registry.emplace<pul::animation::ComponentInstance>(
          finAnimation, std::move(particle.animationInstance)
        );
        registry.emplace<pul::core::ComponentParticle>(
          finAnimation, animation.instance.origin
        );

        // -- apply weapon damage
        if (
            particle.damage.damagePlayer
         && particle.damage.explosionRadius > 0.0f
        ) {
          pul::physics::IntersectorCircle circle;
          circle.origin = animation.instance.origin;
          circle.radius = particle.damage.explosionRadius;
          pul::physics::EntityIntersectionResults results;

          plugin::entity::WeaponDamageCircle(
            scene
          , animation.instance.origin
          , particle.damage.explosionRadius
          , particle.damage.playerSplashDamage
          , particle.damage.explosionForce
          , playerDirectHit // fine if it's null; don't want to hit player 2x
          );
        }

        animation.instance = {};
        registry.destroy(entity);
      }
    }
  }

  { // -- particles
    auto view =
      registry.view<
        pul::core::ComponentParticle
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & particle = view.get<pul::core::ComponentParticle>(entity);

      if (particle.velocity != glm::vec2()) {
        if (particle.gravityAffected) {
          if (particle.velocity.y < 8.0f) {
            particle.velocity.y += 0.05f;
          }
        }

        if (particle.velocityModifierFn)
          { particle.velocityModifierFn(particle.velocity); }

        // TODO fix this
        particle.origin += particle.velocity;
        animation.instance.origin += particle.velocity;

        animation.instance.pieceToState["particle"].angle =
          std::atan2(particle.velocity.x, particle.velocity.y);
      }

      if (animation.instance.pieceToState["particle"].animationFinished) {
        animation.instance = {};
        registry.destroy(entity);
      }
    }
  }

  { // -- pickups
    auto view =
      registry.view<
        pul::core::ComponentPickup
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & pickup = view.get<pul::core::ComponentPickup>(entity);
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);

      if (!pickup.spawned) {
        pickup.spawnTimer += pul::util::MsPerFrame;
        if (pickup.spawnTimer >= pickup.spawnTimerSet) {
          pickup.spawnTimer = 0ul;
          pickup.spawned = true;

          pul::audio::EventInfo audioEvent;
          audioEvent.event = pul::audio::event::Type::PickupSpawn;
          audioEvent.params = {{ "type", Idx(pickup.type) }};
          audioEvent.origin = pickup.origin;
          scene.AudioSystem().DispatchEventOneOff(audioEvent);
        }
      }

      animation.instance.pieceToState["pickups"].visible = pickup.spawned;
      if (
          animation.instance.pieceToState.find("pickup-bg")
       != animation.instance.pieceToState.end()
      ) {
        animation.instance.pieceToState["pickup-bg"].visible = pickup.spawned;
      }

      animation.instance.origin = pickup.origin;
    }
  }

  { // -- bot
    auto view =
      registry.view<
        pul::controls::ComponentController, pul::core::ComponentBotControllable
      , pul::core::ComponentPlayer, pul::animation::ComponentInstance
      , pul::core::ComponentDamageable
      >();

    for (auto entity : view) {
      auto & bot = view.get<pul::core::ComponentPlayer>(entity);
      auto & damageable = view.get<pul::core::ComponentDamageable>(entity);
      auto & controller =
        view.get<pul::controls::ComponentController>(entity).controller;
      auto & origin = registry.get<pul::core::ComponentOrigin>(entity);
      auto & hitbox = registry.get<pul::core::ComponentHitboxAABB>(entity);

      controller.previous = std::move(controller.current);
      controller.current = {};

      // update bot control input
      if (::botPlays) {
        plugin::bot::ApplyInput(scene, controller, bot, origin.origin);
      }

      plugin::entity::UpdatePlayer(
        scene, controller, bot, origin.origin, hitbox
      , view.get<pul::animation::ComponentInstance>(entity)
      , damageable
      );
    }
  }

  if (showHitboxRendering)
  { // -- debug hitbox lines
    auto view =
      registry.view<
        pul::core::ComponentHitboxAABB, pul::core::ComponentOrigin
      >();

    for (auto & entity : view) {
      // get origin/dimensions, for dimensions multiply by half in order to
      // get its "radius" or whatever
      auto const & dim =
        glm::vec2(
          view.get<pul::core::ComponentHitboxAABB>(entity).dimensions
        ) * 0.5f
      ;
      auto const & origin =
        view.get<pul::core::ComponentOrigin>(entity).origin;

      plugin::debug::RenderAabbByCenter(
        origin, dim, glm::vec3(0.7f, 0.8f, 1.0f)
      );
    }
  }

  { // -- player
    auto view =
      registry.view<
        pul::controls::ComponentController
      , pul::core::ComponentPlayerControllable
      , pul::core::ComponentPlayer,pul::core::ComponentCamera
      , pul::animation::ComponentInstance
      , pul::core::ComponentDamageable
      , pul::core::ComponentOrigin
      , pul::core::ComponentHitboxAABB
      >();

    for (auto entity : view) {
      auto & player = view.get<pul::core::ComponentPlayer>(entity);
      auto & origin = registry.get<pul::core::ComponentOrigin>(entity);
      auto & hitbox = registry.get<pul::core::ComponentHitboxAABB>(entity);
      auto & damageable = view.get<pul::core::ComponentDamageable>(entity);

      plugin::entity::UpdatePlayer(
        scene
      , scene.PlayerController()
      , player
      , origin.origin
      , hitbox
      , view.get<pul::animation::ComponentInstance>(entity)
      , damageable
      );

      // -- tracking camera
      // center camera on this
      glm::vec2 L = scene.PlayerController().current.lookOffset;
      if (glm::length(L) > 500.0f) {
        L = glm::normalize(L) * 500.0f;
      }
      auto const offset = glm::pow(glm::length(L) / 500.0f, 0.7f) * L * 0.5f;

      scene.playerOrigin = origin.origin - glm::vec2(0.0f, 20.0f);
      scene.cameraOrigin =
        glm::mix(
          scene.cameraOrigin
        , glm::i32vec2(scene.playerOrigin + offset)
        , 0.5f
        );

      // -- hud
      auto & hud = scene.Hud();
      hud.player.health = damageable.health;
      hud.player.armor = damageable.armor;
    }
  }

  { // -- hitscan projectile
    auto view =
      registry.view<
        pul::core::ComponentHitscanProjectile
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & projectile =
        view.get<pul::core::ComponentHitscanProjectile>(entity);

      if (projectile.update && projectile.update(projectile)) {
        registry.destroy(entity);
        continue;
      }

      auto const & playerAnim = *projectile.playerAnimation;
      auto const & weaponState =
        playerAnim.pieceToState.at("weapon-placeholder");
      auto const & weaponMatrix = weaponState.cachedLocalSkeletalMatrix;

      plugin::animation::UpdateCacheWithPrecalculatedMatrix(
        animation.instance, weaponMatrix
      );
    }
  }

  { // -- creature lump
    auto view =
      registry.view<
        pul::core::ComponentCreatureLump
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & creature = view.get<pul::core::ComponentCreatureLump>(entity);

      plugin::bot::UpdateCreatureLump(scene, creature, animation);
    }
  }

  { // -- beams
    auto view =
      registry.view<
        pul::animation::ComponentInstance
      , pul::core::ComponentParticleBeam
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & beam = view.get<pul::core::ComponentParticleBeam>(entity);

      bool destroy = false;

      if (beam.update) {
        destroy |= beam.update(animation.instance, beam.hitCooldown);
      }

      if (destroy) {
        registry.destroy(entity);
      }
    }
  }

  { // -- distance particle emitter
    auto view =
      registry.view<
        pul::animation::ComponentInstance
      , pul::core::ComponentDistanceParticleEmitter
      >();

    for (auto entity : view) {
      auto & animation = view.get<pul::animation::ComponentInstance>(entity);
      auto & emitter =
        view.get<pul::core::ComponentDistanceParticleEmitter>(entity);

      // emit a particle over a given distance; the delta of distance must be
      // calculated every frame in case the particle is moving a non-constant
      // speed (for example, if it is affected by gravity)

      emitter.distanceTravelled +=
        glm::length(animation.instance.origin - emitter.prevOrigin)
      ;
      emitter.prevOrigin = animation.instance.origin;

      if (emitter.distanceTravelled >= emitter.originDist) {
        emitter.distanceTravelled -= emitter.originDist;
        pul::animation::Instance animationInstance;
        plugin::animation::ConstructInstance(
          scene, animationInstance, scene.AnimationSystem()
        , emitter.animationInstance.animator->label.c_str()
        );

        animationInstance
          .pieceToState["particle"]
          .Apply(emitter.animationInstance.animator->label.c_str(), true);

        animationInstance.pieceToState["particle"].angle
          = animation.instance.pieceToState["particle"].angle;

        animationInstance.origin = animation.instance.origin;

        auto particleEntity = registry.create();

        registry.emplace<pul::animation::ComponentInstance>(
          particleEntity, std::move(animationInstance)
        );
        registry.emplace<pul::core::ComponentParticle>(
          particleEntity
        , animationInstance.origin
        , emitter.velocity
        );
      }
    }
  }
}

void plugin::entity::DebugUiDispatch(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  plugin::config::RenderImGui();

  ImGui::Begin("Entity");
  ImGui::Checkbox("allow bot to move around", &::botPlays);
  if (ImGui::Button("give all weapons")) {
    auto view = registry.view<pul::core::ComponentPlayer>();
    for (auto & entity : view) {
      auto & player = view.get<pul::core::ComponentPlayer>(entity);
      for (auto & weapon : player.inventory.weapons) {
        weapon.pickedUp = true;
        weapon.ammunition = 500u;
      }
    }
  }
  registry.each([&](auto entity) {

    std::string label = fmt::format("{}", static_cast<size_t>(entity));
    if (registry.has<pul::core::ComponentLabel>(entity)) {
      label = registry.get<pul::core::ComponentLabel>(entity).label;
    }

    ImGui::PushID(static_cast<size_t>(entity));

    // used to only show entities that exist
    bool hasStart = false;
    bool hasTreeNode = false;
    auto treeStart = [&hasStart, &hasTreeNode, label]() -> bool {
      if (!hasStart) {
        hasStart = true;
        hasTreeNode = ImGui::TreeNode(label.c_str());
      }

      return hasTreeNode;
    };

    if (registry.has<pul::core::ComponentDamageable>(entity) && treeStart()) {
      auto & comp = registry.get<pul::core::ComponentDamageable>(entity);
      ImGui::Text("--- damageable ---");
      pul::imgui::SliderInt("health", &comp.health, 0u, 200u);
      pul::imgui::SliderInt("armor", &comp.armor, 0u, 200u);
    }

    if (registry.has<pul::core::ComponentOrigin>(entity) && treeStart()) {
      auto & comp = registry.get<pul::core::ComponentOrigin>(entity);
      ImGui::Text("--- origin ---");
      ImGui::DragFloat2("origin", &comp.origin.x, 1.0f);
    }

    if (registry.has<pul::core::ComponentCreatureLump>(entity) && treeStart()) {
      auto & comp = registry.get<pul::core::ComponentCreatureLump>(entity);
      ImGui::Text("--- creature lump ---");
      ImGui::DragFloat2("origin", &comp.origin.x, 1.0);
    }

    if (registry.has<pul::core::ComponentHitboxAABB>(entity) && treeStart()) {
      auto & comp = registry.get<pul::core::ComponentHitboxAABB>(entity);
      ImGui::Text("--- hitbox AABB ---");
      pul::imgui::DragInt2("dimensions", &comp.dimensions.x, 1.0f);
    }

    if (registry.has<pul::core::ComponentPlayer>(entity) && treeStart()) {
      ImGui::Text("--- player ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<pul::core::ComponentPlayer>(entity);
      ImGui::PushID(&self);
      pul::imgui::Text(
        "weapon anim ID {}", static_cast<size_t>(self.weaponAnimation)
      );
      ImGui::DragFloat2("velocity", &self.velocity.x, 0.25f);
      ImGui::DragFloat2("stored velocity", &self.storedVelocity.x, 0.025f);
      pul::imgui::Text("midair dashes left {}", self.midairDashesLeft);
      pul::imgui::Text("jump fall time {}", self.jumpFallTime);
      pul::imgui::Text(
        "prev frame ground accel {}", self.prevFrameGroundAcceleration
      );
      pul::imgui::Text("dash gravity time {:.2f}\n", self.dashZeroGravityTime);
      pul::imgui::Text(
        "dash cooldown\n"
        "   {:.2f}|{} {:.2f}|{} {:.2f}|{}\n"
        "   {:.2f}|{}        {:.2f}|{}\n"
        "   {:.2f}|{} {:.2f}|{} {:.2f}|{}"
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::UpperLeft)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::UpperLeft)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::Up)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::Up)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::UpperRight)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::UpperRight)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::Left)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::Left)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::Right)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::Right)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::LowerLeft)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::LowerLeft)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::Down)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::Down)])
      , glm::max(0.f, self.dashCooldown[Idx(pul::Direction::LowerRight)]/1000.f)
      , static_cast<int>(self.dashLock[Idx(pul::Direction::LowerRight)])
      );

      pul::imgui::Text("slide cooldown {:.2f}", self.crouchSlideCooldown);
      pul::imgui::Text("slide friction time {:.2f}", self.slideFrictionTime);
      ImGui::Checkbox("crouchSliding", &self.crouchSliding);
      ImGui::SameLine();
      ImGui::Checkbox("crouching", &self.crouching);
      ImGui::Checkbox("jumping", &self.jumping);
      ImGui::SameLine();
      ImGui::Checkbox("hasReleasedJump", &self.hasReleasedJump);
      ImGui::Checkbox("grounded", &self.grounded);
      ImGui::Checkbox("prev ground", &self.prevGrounded);
      ImGui::SameLine();
      ImGui::Checkbox("use gravity", &self.affectedByGravity);
      ImGui::Checkbox("wall cling left", &self.wallClingLeft);
      ImGui::SameLine();
      ImGui::Checkbox("wall cling right", &self.wallClingRight);
      if (ImGui::Button("Give all weapons")) {
        for (auto & weapon : self.inventory.weapons) {
          weapon.pickedUp = true;
          weapon.ammunition = 500u;
        }
      }
      if (ImGui::TreeNode("inventory")) {
        for (auto & weapon : self.inventory.weapons) {
          ImGui::PushID(Idx(weapon.type));

          ImGui::Separator();

          if (ImGui::Button(ToStr(weapon.type)))
            { self.inventory.ChangeWeapon(weapon.type); }

          if (weapon.type == self.inventory.currentWeapon) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "equipped");
          }

          ImGui::SameLine();
          ImGui::Checkbox("picked up", &weapon.pickedUp);
          ImGui::SetNextItemWidth(128.0f);
          pul::imgui::SliderInt("ammo", &weapon.ammunition, 0, 1000);
          ImGui::SameLine();
          pul::imgui::Text("| cooldown {}", weapon.cooldown);

          switch (weapon.type) {
            default: break;
            case pul::core::WeaponType::Grannibal: {
              auto & grannibalInfo =
                std::get<pul::core::WeaponInfo::WiGrannibal>(weapon.info);

              pul::imgui::Text(
                "discharge timer {}", grannibalInfo.dischargingTimer
              );
              pul::imgui::Text(
                "primary muzzle trail timer {}"
              , grannibalInfo.primaryMuzzleTrailTimer
              );
              pul::imgui::Text(
                "muzzle trail left {}", grannibalInfo.primaryMuzzleTrailLeft
              );
            } break;
            case pul::core::WeaponType::DopplerBeam: {
              auto & volInfo =
                std::get<pul::core::WeaponInfo::WiDopplerBeam>(weapon.info);

              pul::imgui::Text(
                "chargeup timer {}", volInfo.dischargingTimer
              );
            } break;
            case pul::core::WeaponType::Volnias: {
              auto & volInfo =
                std::get<pul::core::WeaponInfo::WiVolnias>(weapon.info);

              pul::imgui::Text(
                "chargeup timer {}", volInfo.primaryChargeupTimer
              );
              pul::imgui::Text(
                "secondary charged shots {}", volInfo.secondaryChargedShots
              );
              pul::imgui::Text(
                "secondary chargeup timer {}", volInfo.secondaryChargeupTimer
              );
            } break;
          }

          ImGui::PopID(); // weapon
        }

        ImGui::TreePop();
      }
      pul::imgui::Text(
        "current weapon: '{}'", ToStr(self.inventory.currentWeapon)
      );
      pul::imgui::Text(
        "prev    weapon: '{}'", ToStr(self.inventory.previousWeapon)
      );
      ImGui::PopID();
    }

    if (hasStart && hasTreeNode) {
      ImGui::TreePop();
    }

    ImGui::PopID();
  });
  ImGui::End();

  ImGui::Begin("Diagnostics");
    ImGui::Checkbox("show hitbox rendering", &::showHitboxRendering);
  ImGui::End();

  auto view =
    registry.view<
      pul::core::ComponentPlayer
    , pul::animation::ComponentInstance
    , pul::core::ComponentPlayerControllable
    >();

  for (auto entity : view) {
    plugin::entity::DebugUiDispatchPlayer(
      scene
    , view.get<pul::core::ComponentPlayer>(entity)
    , view.get<pul::animation::ComponentInstance>(entity)
    );
  }
}
