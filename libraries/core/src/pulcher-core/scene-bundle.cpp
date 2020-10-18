#include <pulcher-core/scene-bundle.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-physics/intersections.hpp>

#include <entt/entt.hpp>

struct pulcher::core::SceneBundle::Impl {
  pulcher::animation::System animationSystem;
  pulcher::controls::Controller playerController;
  pulcher::physics::DebugQueries physicsQueries;

  entt::registry enttRegistry;
};

#define PIMPL_SPECIALIZE pulcher::core::SceneBundle::Impl
#include <pulcher-util/pimpl.inl>

pulcher::animation::System & pulcher::core::SceneBundle::AnimationSystem() {
  return impl->animationSystem;
}

pulcher::controls::Controller & pulcher::core::SceneBundle::PlayerController() {
  return impl->playerController;
}

pulcher::physics::DebugQueries &
pulcher::core::SceneBundle::PhysicsDebugQueries() {
  return impl->physicsQueries;
}

entt::registry & pulcher::core::SceneBundle::EnttRegistry() {
  return impl->enttRegistry;
}
