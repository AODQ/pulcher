#include <pulcher-core/scene-bundle.hpp>

#include <entt/entity/registry.hpp>

entt::registry & pulcher::core::Registry() {
  static entt::registry self;
  return self;
}
