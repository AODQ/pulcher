#include <pulcher-core/scene-bundle.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/hud.hpp>
#include <pulcher-physics/intersections.hpp>

#include <entt/entt.hpp>

struct pul::core::SceneBundle::Impl {
  pul::animation::System animationSystem;
  pul::audio::System audioSystem;
  pul::core::PlayerMetaInfo playerMetaInfo;
  pul::controls::Controller playerController;
  pul::physics::DebugQueries physicsQueries;
  pul::core::ComponentPlayer storedDebugPlayerComponent;
  pul::core::HudInfo hudInfo;

  entt::registry enttRegistry;
};

#define PIMPL_SPECIALIZE pul::core::SceneBundle::Impl
#include <pulcher-util/pimpl.inl>

pul::animation::System & pul::core::SceneBundle::AnimationSystem() {
  return impl->animationSystem;
}

pul::audio::System & pul::core::SceneBundle::AudioSystem() {
  return impl->audioSystem;
}

pul::controls::Controller & pul::core::SceneBundle::PlayerController() {
  return impl->playerController;
}

pul::core::PlayerMetaInfo & pul::core::SceneBundle::PlayerMetaInfo() {
  return impl->playerMetaInfo;
}

pul::physics::DebugQueries &
pul::core::SceneBundle::PhysicsDebugQueries() {
  return impl->physicsQueries;
}

pul::core::ComponentPlayer &
pul::core::SceneBundle::StoredDebugPlayerComponent() {
  return impl->storedDebugPlayerComponent;
}

pul::core::HudInfo & pul::core::SceneBundle::Hud() {
  return impl->hudInfo;
}

entt::registry & pul::core::SceneBundle::EnttRegistry() {
  return impl->enttRegistry;
}
