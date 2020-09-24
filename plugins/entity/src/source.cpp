#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>

namespace {

// probably should be renamed to Player or similar
struct ComponentMoveable {
  glm::vec2 origin = {};
  glm::vec2 velocity = {};

  size_t physxQueryGravity = -1ul;
  size_t physxQueryVelocity = -1ul;
  size_t physxQuerySlope = -1ul;
  bool sleeping = false;

  bool jumping = false;

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

    pulcher::physics::IntersectionResults previousFrameGravityIntersection;
    if (moveable.physxQueryGravity != -1ul) {
      previousFrameGravityIntersection =
        scene.physicsQueries.RetrieveQuery(
          moveable.physxQueryGravity
        );
    }

    pulcher::physics::IntersectionResults intersectionSlope;
    if (moveable.physxQuerySlope != -1ul) {
      intersectionSlope =
        scene.physicsQueries.RetrieveQuery(moveable.physxQuerySlope);
    }

    { // -- apply projected movement physics
      pulcher::physics::IntersectionResults previousFrameIntersection;
      if (moveable.physxQueryVelocity != -1ul) {
        previousFrameIntersection =
          scene.physicsQueries.RetrieveQuery(moveable.physxQueryVelocity);
      }

      if (previousFrameIntersection.collision) {
        if (!previousFrameGravityIntersection.collision) {
          moveable.origin.y += moveable.velocity.y;
        }
        if (!intersectionSlope.collision) {
          moveable.origin = intersectionSlope.origin;
        }
        moveable.velocity = {};
      } else {
        moveable.origin = moveable.CalculateProjectedOrigin();
        moveable.velocity = {};
      }
    }

    // ground player
    if (!moveable.jumping && previousFrameGravityIntersection.collision) {
      moveable.origin.y = previousFrameGravityIntersection.origin.y - 1.0f;
    }

    // -- process inputs / events

    auto & current = scene.playerController.current;
    moveable.velocity.x = static_cast<float>(current.movementHorizontal)*0.15f;
    moveable.velocity.y = static_cast<float>(current.movementVertical)*0.15f;
    if (current.dash) { moveable.velocity *= 16.0f; }

    moveable.jumping = current.jump;
    if (moveable.jumping) {
      moveable.velocity.y -= 1.5f;
    }

    // gravity if gravity check failed
    if (!previousFrameGravityIntersection.collision) {
      moveable.velocity.y += 1.0f;
    }

    // check for next frame physics intersection
    moveable.physxQueryVelocity = -1ul;
    if (!moveable.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(moveable.origin);
      ray.endOrigin = glm::round(moveable.CalculateProjectedOrigin());
      moveable.physxQueryVelocity = scene.physicsQueries.AddQuery(ray);
    }

    // check for slopes
    moveable.physxQuerySlope = -1ul;
    if (!moveable.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(moveable.origin);
      auto origin = moveable.CalculateProjectedOrigin();
      ray.endOrigin =
        glm::round(
          origin
        + glm::vec2(0, -5) * glm::max(1.0f, glm::length(moveable.velocity))
        );
      moveable.physxQuerySlope = scene.physicsQueries.AddQuery(ray);
    }

    // apply gravity intersection test
    moveable.physxQueryGravity = -1ul;
    if (!moveable.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(moveable.origin);
      ray.endOrigin = glm::round(moveable.origin + glm::vec2(0, 1));
      moveable.physxQueryGravity =
        scene.physicsQueries.AddQuery(ray);
    }

    // center camera on this
    scene.cameraOrigin = glm::i32vec2(moveable.origin);
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
      ImGui::Checkbox("sleeping", &self.sleeping);
      pul::imgui::Text(
        "physics projection query ID {}"
      , self.physxQueryVelocity & 0xFFFF
      );
      if (self.physxQueryVelocity) {
        auto intersection =
          scene.physicsQueries.RetrieveQuery(self.physxQueryVelocity)
        ;
        pul::imgui::Text(
          " -- collision {}", intersection.collision
        );
      }

      pul::imgui::Text(
        "physics gravity query ID {}"
      , self.physxQueryGravity & 0xFFFF
      );
      if (self.physxQueryGravity) {
        auto intersection =
          scene.physicsQueries.RetrieveQuery(self.physxQueryGravity);
        pul::imgui::Text(" -- collision {}", intersection.collision);
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
