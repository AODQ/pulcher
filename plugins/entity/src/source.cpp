#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>

namespace {

// probably should be renamed to Player or similar
struct ComponentPlayer {
  glm::vec2 origin = {};
  glm::vec2 velocity = {};

  size_t physxQueryGravity = -1ul;
  size_t physxQueryVelocity = -1ul;
  size_t physxQuerySlope = -1ul;
  size_t physxQueryCeiling = -1ul;
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

PUL_PLUGIN_DECL void StartScene(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto playerEntity = registry.create();
  registry.emplace<ComponentPlayer>(playerEntity);
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
}

PUL_PLUGIN_DECL void Shutdown(pulcher::core::SceneBundle & scene) {

  // delete registry
  scene.EnttRegistry() = {};
}

PUL_PLUGIN_DECL void EntityUpdate(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();

  auto view =
    registry.view<ComponentControllable, ComponentPlayer, ComponentCamera>();

  auto & physicsQueries = scene.PhysicsQueries();

  for (auto entity : view) {

    auto & player = view.get<ComponentPlayer>(entity);

    pulcher::physics::IntersectionResults previousFrameGravityIntersection;
    if (player.physxQueryGravity != -1ul) {
      previousFrameGravityIntersection =
        physicsQueries.RetrieveQuery(
          player.physxQueryGravity
        );
    }

    pulcher::physics::IntersectionResults intersectionSlope;
    if (player.physxQuerySlope != -1ul) {
      intersectionSlope = physicsQueries.RetrieveQuery(player.physxQuerySlope);
    }

    pulcher::physics::IntersectionResults intersectionCeiling;
    player.physxQueryCeiling = -1ul;
    if (player.physxQueryCeiling != -1ul) {
      intersectionCeiling =
        physicsQueries.RetrieveQuery(player.physxQueryCeiling);
    }

    { // -- apply projected movement physics
      pulcher::physics::IntersectionResults previousFrameIntersection;
      if (player.physxQueryVelocity != -1ul) {
        previousFrameIntersection =
          physicsQueries.RetrieveQuery(player.physxQueryVelocity);
      }

      if (previousFrameIntersection.collision) {
        if (!previousFrameGravityIntersection.collision) {
          player.origin.y += player.velocity.y;
        }
        if (!intersectionSlope.collision) {
          player.origin = intersectionSlope.origin;
        }
        player.velocity = {};
      } else {
        player.origin = player.CalculateProjectedOrigin();
        player.velocity = {};
      }
    }

    // ground player
    if (
         !player.jumping
      && !intersectionCeiling.collision
      && previousFrameGravityIntersection.collision
    ) {
      player.origin.y = previousFrameGravityIntersection.origin.y - 1.0f;
    }

    // -- process inputs / events

    auto & current = scene.PlayerController().current;
    player.velocity.x = static_cast<float>(current.movementHorizontal);
    player.velocity.y = static_cast<float>(current.movementVertical);
    if (current.dash) { player.velocity *= 16.0f; }

    player.jumping = current.jump;
    if (player.jumping && !intersectionCeiling.collision) {
      player.velocity.y -= 3.5f;
    }

    // gravity if gravity check failed
    if (!previousFrameGravityIntersection.collision) {
      player.velocity.y += 1.0f;
    }

    // check for next frame physics intersection
    player.physxQueryVelocity = -1ul;
    if (!player.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(player.origin);
      ray.endOrigin = glm::round(player.CalculateProjectedOrigin());
      player.physxQueryVelocity = physicsQueries.AddQuery(ray);
    }

    // check for ceiling
    player.physxQueryCeiling = -1ul;
    if (!player.sleeping && !current.crouch) {
      pulcher::physics::IntersectorPoint point;
      point.origin = glm::round(player.origin + glm::vec2(0.0f, -32.0f));
      player.physxQueryCeiling = physicsQueries.AddQuery(point);
    }

    // check for slopes
    player.physxQuerySlope = -1ul;
    if (!player.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(player.origin);
      auto origin = player.CalculateProjectedOrigin();
      ray.endOrigin =
        glm::round(
          origin
        + glm::vec2(0, -5) * glm::max(1.0f, glm::length(player.velocity))
        );
      player.physxQuerySlope = physicsQueries.AddQuery(ray);
    }

    // apply gravity intersection test
    player.physxQueryGravity = -1ul;
    if (!player.sleeping && !current.crouch) {
      pulcher::physics::IntersectorRay ray;
      ray.beginOrigin = glm::round(player.origin);
      ray.endOrigin = glm::round(player.origin + glm::vec2(0, 1));
      player.physxQueryGravity = physicsQueries.AddQuery(ray);
    }

    // center camera on this
    scene.cameraOrigin = glm::i32vec2(player.origin);
  }
}

PUL_PLUGIN_DECL void UiRender(pulcher::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();
  auto & physicsQueries = scene.PhysicsQueries();

  ImGui::Begin("Entity");
  registry.each([&](auto entity) {
    pul::imgui::Text("entity ID {}", static_cast<size_t>(entity));

    ImGui::Separator();

    if (registry.has<ComponentLabel>(entity)) {
      auto & label = registry.get<ComponentLabel>(entity);
      ImGui::Text("'%s'", label.label.c_str());
    }

    if (registry.has<ComponentPlayer>(entity)) {
      ImGui::Text("--- player ---"); ImGui::SameLine(); ImGui::Separator();
      auto & self = registry.get<ComponentPlayer>(entity);
      pul::imgui::Text("origin <{:2f}, {:2f}>", self.origin.x, self.origin.y);
      pul::imgui::Text(
        "velocity <{:2f}, {:2f}>", self.velocity.x, self.velocity.y
      );
      ImGui::Checkbox("sleeping", &self.sleeping);
      pul::imgui::Text(
        "physics projection query ID {}"
      , self.physxQueryVelocity & 0xFFFF
      );
      if (self.physxQueryVelocity != -1ul) {
        auto intersection =
          physicsQueries.RetrieveQuery(self.physxQueryVelocity)
        ;
        pul::imgui::Text(
          " -- collision {}", intersection.collision
        );
      }

      pul::imgui::Text(
        "physics gravity query ID {}"
      , self.physxQueryGravity & 0xFFFF
      );
      if (self.physxQueryGravity != -1ul) {
        auto intersection = physicsQueries.RetrieveQuery(self.physxQueryGravity);
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
