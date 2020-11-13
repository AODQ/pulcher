// entity plugin

#include <plugin-entity/cursor.hpp>
#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/hud.hpp>
#include <pulcher-core/particle.hpp>
#include <pulcher-core/pickup.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-core/weapon.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <entt/entt.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/transform2.hpp>
#include <imgui/imgui.hpp>

#include <random>

namespace {

bool botPlays = false;

struct ComponentLabel {
  std::string label = {};
};

struct ComponentController {
  pul::controls::Controller controller;
};

struct ComponentPlayerControllable { };
struct ComponentBotControllable { };

struct ComponentCamera {
};

} // -- namespace

extern "C" {

PUL_PLUGIN_DECL void Entity_StartScene(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {

  plugin::entity::ConstructCursor(plugin, scene);

  auto & registry = scene.EnttRegistry();

  // player
  {
    auto playerEntity = registry.create();
    registry.emplace<pul::core::ComponentPlayer>(playerEntity);
    registry.emplace<ComponentController>(playerEntity);
    registry.emplace<ComponentPlayerControllable>(playerEntity);
    registry.emplace<ComponentCamera>(playerEntity);
    registry.emplace<ComponentLabel>(playerEntity, "Player");

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "nygelstromn"
    );
    registry.emplace<pul::animation::ComponentInstance>(
      playerEntity, std::move(instance)
    );

    // load up player weapon animation & state from previous plugin load
    {
      auto & player = registry.get<pul::core::ComponentPlayer>(playerEntity);

      // overwrite with player component for persistent reloads
      player = scene.StoredDebugPlayerComponent();

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
  }

  // bot/AI
  {
    auto botEntity = registry.create();
    registry.emplace<pul::core::ComponentPlayer>(botEntity);
    registry.emplace<ComponentController>(botEntity);
    registry.emplace<ComponentBotControllable>(botEntity);
    registry.emplace<ComponentLabel>(botEntity, "Bot");

    pul::animation::Instance instance;
    plugin.animation.ConstructInstance(
      scene, instance, scene.AnimationSystem(), "nygelstromn"
    );
    registry.emplace<pul::animation::ComponentInstance>(
      botEntity, std::move(instance)
    );

    {
      auto & bot = registry.get<pul::core::ComponentPlayer>(botEntity);

      // load weapon animation
      pul::animation::Instance weaponInstance;
      plugin.animation.ConstructInstance(
        scene, weaponInstance, scene.AnimationSystem(), "weapons"
      );
      bot.weaponAnimation = registry.create();

      registry.emplace<pul::animation::ComponentInstance>(
        bot.weaponAnimation, std::move(weaponInstance)
      );
    }
  }
}

PUL_PLUGIN_DECL void Entity_Shutdown(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  // store player
  auto view =
    registry.view<
      ComponentPlayerControllable, pul::core::ComponentPlayer
    , ComponentCamera, pul::animation::ComponentInstance
    >();

  for (auto entity : view) {
    // save player component for persistent reloads
    scene.StoredDebugPlayerComponent() =
      std::move(view.get<pul::core::ComponentPlayer>(entity));
  }

  // delete registry
  registry = {};
}

PUL_PLUGIN_DECL void Entity_EntityRender(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {
  plugin::entity::RenderCursor(plugin, scene);
}

PUL_PLUGIN_DECL void Entity_EntityUpdate(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  { // -- particle explode
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
          plugin.physics.IntersectionRaycast(scene, ray, results)
        ) {
          explodeOrigin = results.origin;
          explode |= exploder.explodeOnCollide;
        }
      }

      // check for player
      // TODO
      /* auto playerView = */
      /*   registry.view< */
      /*     pul::core::ComponentPlayer */
      /*   >(); */

      /* for (auto playerEntity : playerView) { */
      /*   auto & player = registry.get<pul::core::ComponentPlayer>(playerEntity); */
      /*   auto origin = player.origin - glm::vec2(0.0f, 32.0f); */
      /*   if (glm::length(origin - animation.instance.origin) < 8.0f) { */
      /*     explode = true; */
      /*   } */
      /* } */

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
          { particle.velocity.y += 0.15f; }

        auto ray =
          pul::physics::IntersectorRay::Construct(
            animation.instance.origin,
            animation.instance.origin + particle.velocity
          );

        if (
          pul::physics::IntersectionResults results;
          plugin.physics.IntersectionRaycast(scene, ray, results)
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
              plugin.physics.IntersectionPoint(scene, pointInt, pointResult)
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
            plugin.animation.ConstructInstance(
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

        // TODO fix this
        particle.origin += particle.velocity;
        animation.instance.origin += particle.velocity;
      }

      animation.instance.pieceToState["particle"].angle =
        std::atan2(particle.velocity.x, particle.velocity.y);

      if (destroyInstance) {

        auto finAnimation = registry.create();
        particle.animationInstance.origin = animation.instance.origin;
        registry.emplace<pul::animation::ComponentInstance>(
          finAnimation, std::move(particle.animationInstance)
        );
        registry.emplace<pul::core::ComponentParticle>(
          finAnimation, animation.instance.origin
        );

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
        ComponentController, ComponentBotControllable
      , pul::core::ComponentPlayer, pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & bot = view.get<pul::core::ComponentPlayer>(entity);

      auto & controller = view.get<ComponentController>(entity).controller;

      static std::random_device device;
      static std::mt19937 generator(device());
      static std::uniform_int_distribution<int> distribution(1, 1000);

      controller.previous = std::move(controller.current);
      controller.current = {};

      if (::botPlays)
      { // update controls

        bool dir =
            controller.previous.movementHorizontal
         == pul::controls::Controller::Movement::Right
        ;

        static float angle = 0.0f;
        if (distribution(generator) < 15) {
          dir ^= 1;
          controller.previous.movementHorizontal =
            dir
              ? pul::controls::Controller::Movement::Right
              : pul::controls::Controller::Movement::Left
          ;
        }
        if (distribution(generator) < 25) {
          controller.current.jump = true;
        }
        if (distribution(generator) < 5) {
          controller.current.crouch = true;
        }
        if (distribution(generator) < 10) {
          controller.current.dash = true;
        }
        controller.current.lookAngle = dir ? -4 : +3;
        controller.current.lookDirection =
          glm::vec2(glm::cos(angle), glm::sin(angle));

        {
          controller.current.movementHorizontal =
            dir
              ? pul::controls::Controller::Movement::Right
              : pul::controls::Controller::Movement::Left
          ;
        }
      }

      plugin::entity::UpdatePlayer(
        plugin, scene
      , controller, bot
      , view.get<pul::animation::ComponentInstance>(entity)
      );
    }
  }

  { // -- player
    auto view =
      registry.view<
        ComponentController, ComponentPlayerControllable
      , pul::core::ComponentPlayer, ComponentCamera
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      auto & player = view.get<pul::core::ComponentPlayer>(entity);
      plugin::entity::UpdatePlayer(
        plugin, scene, scene.PlayerController()
      , player
      , view.get<pul::animation::ComponentInstance>(entity)
      );

      // -- tracking camera
      // center camera on this
      glm::vec2 L = scene.PlayerController().current.lookOffset;
      if (glm::length(L) > 500.0f) {
        L = glm::normalize(L) * 500.0f;
      }
      auto const offset = glm::pow(glm::length(L) / 500.0f, 0.7f) * L * 0.5f;

      scene.playerOrigin = origin.origin - glm::vec2(0.0f, 40.0f);
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

      plugin
        .animation
        .UpdateCacheWithPrecalculatedMatrix(animation.instance, weaponMatrix);
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
        destroy |= beam.update(animation.instance);
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
        glm::ceil(glm::length(animation.instance.origin - emitter.prevOrigin))
      ;
      emitter.prevOrigin = animation.instance.origin;

      if (glm::ceil(emitter.distanceTravelled) >= emitter.originDist) {
        emitter.distanceTravelled -= emitter.originDist;
        pul::animation::Instance animationInstance;
        plugin.animation.ConstructInstance(
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

PUL_PLUGIN_DECL void Entity_UiRender(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  ImGui::Begin("Entity");
  ImGui::Checkbox("allow bot to move around", &::botPlays);
  registry.each([&](auto entity) {
    pul::imgui::Text("entity ID {}", static_cast<size_t>(entity));

    ImGui::Separator();

    if (registry.has<ComponentLabel>(entity)) {
      auto & label = registry.get<ComponentLabel>(entity);
      ImGui::Text("'%s'", label.label.c_str());
    }

    if (registry.has<pul::core::ComponentPlayer>(entity)) {
      ImGui::Begin("Player");
      ImGui::Text("--- player ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<pul::core::ComponentPlayer>(entity);
      ImGui::PushID(&self);
      pul::imgui::Text(
        "weapon anim ID {}", static_cast<size_t>(self.weaponAnimation)
      );
      ImGui::DragFloat2("origin", &self.origin.x, 16.125f);
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
      }
      pul::imgui::Text(
        "current weapon: '{}'", ToStr(self.inventory.currentWeapon)
      );
      pul::imgui::Text(
        "prev    weapon: '{}'", ToStr(self.inventory.previousWeapon)
      );
      ImGui::PopID();
      ImGui::End();
    }

    ImGui::Separator();
    ImGui::Separator();
  });
  ImGui::End();

  auto view =
    registry.view<
      pul::core::ComponentPlayer
    , pul::animation::ComponentInstance
    , ComponentPlayerControllable
    >();

  for (auto entity : view) {
    plugin::entity::UiRenderPlayer(
      scene
    , view.get<pul::core::ComponentPlayer>(entity)
    , view.get<pul::animation::ComponentInstance>(entity)
    );
  }
}

} // -- extern C
