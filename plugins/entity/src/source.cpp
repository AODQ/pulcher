#include <pulcher-core/log.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>

namespace {

struct ComponentMoveable {
  glm::u32vec2 origin;
  glm::vec2 velocity;
  pulcher::physics::IntersectionResults previousFrameIntersectionResults;
};

struct ComponentControllable {
};

entt::entity playerEntity;

} // -- namespace

extern "C" {

void StartScene(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle &
) {
  auto & registry = pulcher::core::Registry();

  playerEntity = registry.create();
  registry.emplace<ComponentMoveable>(playerEntity);
  registry.emplace<ComponentControllable>(playerEntity);
}

void Shutdown() {
  playerEntity = {};
}

void Update(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle &
) {
  auto & registry = pulcher::core::Registry();

  registry.view<ComponentControllable, ComponentMoveable>()
    .each([](auto & pos, auto & moveable) {
      
    });
}

void UiRender(pulcher::core::SceneBundle &) {
}

} // -- extern C
