#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
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

namespace {

struct ComponentLabel {
  std::string label = {};
};

struct ComponentControllable {
};

struct ComponentCamera {
};

} // -- namespace

extern "C" {

PUL_PLUGIN_DECL void Entity_StartScene(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto playerEntity = registry.create();
  registry.emplace<pul::core::ComponentPlayer>(playerEntity);
  registry.emplace<ComponentControllable>(playerEntity);
  registry.emplace<ComponentCamera>(playerEntity);
  registry.emplace<ComponentLabel>(playerEntity, "Player");

  pul::animation::Instance instance;
  plugin.animation.ConstructInstance(
    scene, instance, scene.AnimationSystem(), "nygelstromn"
  );
  registry.emplace<pul::animation::ComponentInstance>(
    playerEntity, std::move(instance)
  );

  {
    auto & player = registry.get<pul::core::ComponentPlayer>(playerEntity);

    // overwrite with player component for persistent reloads
    player = scene.StoredDebugPlayerComponent();

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

PUL_PLUGIN_DECL void Entity_Shutdown(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  // store player
  auto view =
    registry.view<
      ComponentControllable, pul::core::ComponentPlayer, ComponentCamera
    , pul::animation::ComponentInstance
    >();

  for (auto entity : view) {
    // save player component for persistent reloads
    scene.StoredDebugPlayerComponent() =
      std::move(view.get<pul::core::ComponentPlayer>(entity));
  }

  // delete registry
  registry = {};
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
        animation.instance.pieceToState["particle"].animationFinished
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
          explode = true;
        }
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
        particle.origin += particle.velocity;
        animation.instance.origin = particle.origin;
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

  { // -- player
    auto view =
      registry.view<
        ComponentControllable, pul::core::ComponentPlayer, ComponentCamera
      , pul::animation::ComponentInstance
      >();

    for (auto entity : view) {
      plugin::entity::UpdatePlayer(
        plugin, scene
      , view.get<pul::core::ComponentPlayer>(entity)
      , view.get<pul::animation::ComponentInstance>(entity)
      );
    }
  }
}

PUL_PLUGIN_DECL void Entity_UiRender(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  ImGui::Begin("Entity");
  registry.each([&](auto entity) {
    pul::imgui::Text("entity ID {}", static_cast<size_t>(entity));

    ImGui::Separator();

    if (registry.has<ComponentLabel>(entity)) {
      auto & label = registry.get<ComponentLabel>(entity);
      ImGui::Text("'%s'", label.label.c_str());
    }

    if (registry.has<pul::core::ComponentPlayer>(entity)) {
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
          pul::imgui::Text("cooldown {}", weapon.cooldown);

          ImGui::PopID(); // weapon
        }
      }
      pul::imgui::Text(
        "current weapon: '{}'", ToStr(self.inventory.currentWeapon)
      );
      pul::imgui::Text(
        "prev    weapon: '{}'", ToStr(self.inventory.previousWeapon)
      );
      ImGui::PopID(); // player
    }

    if (registry.has<ComponentControllable>(entity)) {
      ImGui::Text("--- controllable ---"); ImGui::SameLine(); ImGui::Separator();
    }

    if (registry.has<ComponentCamera>(entity)) {
      ImGui::Text("--- camera ---"); ImGui::SameLine(); ImGui::Separator();
    }

    if (registry.has<pul::animation::ComponentInstance>(entity)) {
      ImGui::Text("--- animation ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<pul::animation::ComponentInstance>(entity);
      pul::imgui::Text("label: {}", self.instance.animator->label);
      pul::imgui::Text("origin: {}", self.instance.origin);
    }

    ImGui::Separator();
    ImGui::Separator();
  });
  ImGui::End();

  auto view =
    registry.view<
      ComponentControllable, pul::core::ComponentPlayer, ComponentCamera
    , pul::animation::ComponentInstance
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
