#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
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

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
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
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto playerEntity = registry.create();
  registry.emplace<pulcher::core::ComponentPlayer>(playerEntity);
  registry.emplace<ComponentControllable>(playerEntity);
  registry.emplace<ComponentCamera>(playerEntity);
  registry.emplace<ComponentLabel>(playerEntity, "Player");

  pulcher::animation::Instance instance;
  plugin.animation.ConstructInstance(
    instance, scene.AnimationSystem(), "nygelstromn"
  );
  registry.emplace<pulcher::animation::ComponentInstance>(
    playerEntity, instance
  );

  {
    auto & player = registry.get<pulcher::core::ComponentPlayer>(playerEntity);

    pulcher::animation::Instance weaponInstance;
    plugin.animation.ConstructInstance(
      weaponInstance, scene.AnimationSystem(), "weapons"
    );

    player.weaponAnimation = registry.create();
    registry.emplace<pulcher::animation::ComponentInstance>(
      player.weaponAnimation, weaponInstance
    );
  }
}

PUL_PLUGIN_DECL void Entity_Shutdown(pulcher::core::SceneBundle & scene) {

  // delete registry
  scene.EnttRegistry() = {};
}

PUL_PLUGIN_DECL void Entity_EntityUpdate(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto view =
    registry.view<
      ComponentControllable, pulcher::core::ComponentPlayer, ComponentCamera
    , pulcher::animation::ComponentInstance
    >();

  for (auto entity : view) {
    plugin::entity::UpdatePlayer(
      plugin, scene
    , view.get<pulcher::core::ComponentPlayer>(entity)
    , view.get<pulcher::animation::ComponentInstance>(entity)
    );
  }
}

PUL_PLUGIN_DECL void Entity_UiRender(pulcher::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  ImGui::Begin("Entity");
  registry.each([&](auto entity) {
    pul::imgui::Text("entity ID {}", static_cast<size_t>(entity));

    ImGui::Separator();

    if (registry.has<ComponentLabel>(entity)) {
      auto & label = registry.get<ComponentLabel>(entity);
      ImGui::Text("'%s'", label.label.c_str());
    }

    if (registry.has<pulcher::core::ComponentPlayer>(entity)) {
      ImGui::Text("--- player ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<pulcher::core::ComponentPlayer>(entity);
      ImGui::PushID(&self);
      pul::imgui::Text(
        "weapon anim ID {}", static_cast<size_t>(self.weaponAnimation)
      );
      ImGui::DragFloat2("origin", &self.origin.x, 16.125f);
      ImGui::DragFloat2("velocity", &self.velocity.x, 0.25f);
      ImGui::DragFloat2("stored velocity", &self.storedVelocity.x, 0.025f);
      pul::imgui::Text("midair dashes left {}", self.midairDashesLeft);
      pul::imgui::Text("dash cooldown {:.2f}", self.dashCooldown);
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

          ImGui::Checkbox("picked up", &weapon.pickedUp);
          pul::imgui::Text("cooldown {}", weapon.cooldown);

          ImGui::SetNextItemWidth(128.0f);
          pul::imgui::SliderInt("ammo", &weapon.ammunition, 0, 1000);

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

    if (registry.has<pulcher::animation::ComponentInstance>(entity)) {
      ImGui::Text("--- animation ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<pulcher::animation::ComponentInstance>(entity);
      pul::imgui::Text("label: {}", self.instance.animator->label);
      pul::imgui::Text("origin: {}", self.instance.origin);
    }

    ImGui::Separator();
    ImGui::Separator();
  });
  ImGui::End();

  auto view =
    registry.view<
      ComponentControllable, pulcher::core::ComponentPlayer, ComponentCamera
    , pulcher::animation::ComponentInstance
    >();

  for (auto entity : view) {
    plugin::entity::UiRenderPlayer(
      scene
    , view.get<pulcher::core::ComponentPlayer>(entity)
    , view.get<pulcher::animation::ComponentInstance>(entity)
    );
  }
}

} // -- extern C
