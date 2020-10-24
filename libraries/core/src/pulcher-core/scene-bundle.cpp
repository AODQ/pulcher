#include <pulcher-core/scene-bundle.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-physics/intersections.hpp>

#include <entt/entt.hpp>

struct pul::core::SceneBundle::Impl {
  pul::animation::System animationSystem;
  pul::controls::Controller playerController;
  pul::physics::DebugQueries physicsQueries;

  entt::registry enttRegistry;
};

#define PIMPL_SPECIALIZE pul::core::SceneBundle::Impl
#include <pulcher-util/pimpl.inl>

pul::animation::System & pul::core::SceneBundle::AnimationSystem() {
  return impl->animationSystem;
}

pul::controls::Controller & pul::core::SceneBundle::PlayerController() {
  return impl->playerController;
}

pul::physics::DebugQueries &
pul::core::SceneBundle::PhysicsDebugQueries() {
  return impl->physicsQueries;
}

entt::registry & pul::core::SceneBundle::EnttRegistry() {
  return impl->enttRegistry;
}
