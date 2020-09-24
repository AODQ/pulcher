#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>

namespace {

struct ComponentMoveable {
  glm::vec2 origin = {};
  glm::vec2 velocity = {};
  size_t frameIntersectionQuery;

  glm::vec2 CalculateProjectedOrigin() {
    return origin + velocity;
  }
};

struct ComponentLabel {
  std::string label = {};
};

struct ComponentControllable {
};

struct ComponentCamera {
};

} // -- namespace

extern "C" {

void StartScene(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle &
) {
  auto & registry = pulcher::core::Registry();

  auto playerEntity = registry.create();
  registry.emplace<ComponentMoveable>(playerEntity);
  registry.emplace<ComponentControllable>(playerEntity);
  registry.emplace<ComponentCamera>(playerEntity);
  registry.emplace<ComponentLabel>(playerEntity, "Player");
}

void Shutdown() {
  pulcher::core::Registry() = {};
}

void EntityUpdate(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = pulcher::core::Registry();

  auto view =
    registry.view<ComponentControllable, ComponentMoveable, ComponentCamera>();

  for (auto entity : view) {

    auto & moveable = view.get<ComponentMoveable>(entity);

    { // -- apply projected movement physics
      pulcher::physics::IntersectionResults previousFrameIntersection;
      if (moveable.frameIntersectionQuery != 0ul) {
        previousFrameIntersection =
          scene.physicsQueries.RetrieveQuery(moveable.frameIntersectionQuery);
      }

      if (previousFrameIntersection.collision) {
        moveable.origin = previousFrameIntersection.origin;
        moveable.velocity = {};
      } else {
        moveable.origin = moveable.CalculateProjectedOrigin();
        moveable.velocity = {};
      }
    }

    // process events probably...

    // check for next frame physics intersection
    moveable.frameIntersectionQuery = 0ul;
    if (moveable.velocity != glm::vec2(0.0f)) {
      pulcher::physics::IntersectorPoint point;
      point.origin = moveable.CalculateProjectedOrigin();
      moveable.frameIntersectionQuery = scene.physicsQueries.AddQuery(point);
    }

    // center camera on this
    scene.cameraOrigin = glm::i32vec2(moveable.origin);

    auto & current = scene.playerController.current;
    moveable.velocity.x = static_cast<float>(current.movementHorizontal);
    moveable.velocity.y = static_cast<float>(current.movementVertical);
  }
}

void UiRender(pulcher::core::SceneBundle & scene) {
  auto & registry = pulcher::core::Registry();

  ImGui::Begin("Entity");
  registry.each([&](auto entity) {
    ImGui::Text("entity ID %lu", static_cast<size_t>(entity));

    ImGui::Separator();

    if (registry.has<ComponentLabel>(entity)) {
      auto & label = registry.get<ComponentLabel>(entity);
      ImGui::Text("'%s'", label.label.c_str());
    }

    if (registry.has<ComponentMoveable>(entity)) {
      ImGui::Text("--- moveable ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<ComponentMoveable>(entity);
      pul::imgui::Text("origin <{:2f}, {:2f}>", self.origin.x, self.origin.y);
      pul::imgui::Text(
        "velocity <{:2f}, {:2f}>", self.velocity.x, self.velocity.y
      );
      pul::imgui::Text("physics query ID {}", self.frameIntersectionQuery);
      if (self.frameIntersectionQuery) {
        auto previousFrameIntersection =
          scene.physicsQueries.RetrieveQuery(self.frameIntersectionQuery);
        pul::imgui::Text(
          " -- collision {}", previousFrameIntersection .collision
        );
      }
    }

    if (registry.has<ComponentControllable>(entity)) {
      ImGui::Text("--- controllable ---"); ImGui::SameLine(); ImGui::Separator();
    }

    if (registry.has<ComponentCamera>(entity)) {
      ImGui::Text("--- camera ---"); ImGui::SameLine(); ImGui::Separator();
    }

    ImGui::Separator();
    ImGui::Separator();
  });
  ImGui::End();
}

} // -- extern C
