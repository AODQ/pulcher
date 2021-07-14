#include <pulcher-core/scene-bundle.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/hud.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/common-components.hpp>

#include <entt/entt.hpp>

struct pul::core::SceneBundle::Impl {
  pul::animation::System animationSystem;
  pul::audio::System audioSystem;
  pul::core::PlayerMetaInfo playerMetaInfo;
  pul::controls::Controller playerController;
  pul::physics::DebugQueries physicsQueries;
  pul::core::ComponentPlayer storedDebugPlayerComponent;
  pul::util::ComponentOrigin storedDebugPlayerOriginComponent;
  pul::core::HudInfo hudInfo;

  entt::registry enttRegistry;
};

#define PIMPL_SPECIALIZE pul::core::SceneBundle::Impl
#include <pulcher-util/pimpl.inl>

pul::animation::System & pul::core::SceneBundle::AnimationSystem() {
  return impl->animationSystem;
}

pul::animation::System const & pul::core::SceneBundle::AnimationSystem() const {
  return impl->animationSystem;
}

pul::audio::System & pul::core::SceneBundle::AudioSystem() {
  return impl->audioSystem;
}

pul::controls::Controller & pul::core::SceneBundle::PlayerController() {
  return impl->playerController;
}

pul::controls::Controller const & pul::core::SceneBundle::PlayerController(
) const {
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

pul::util::ComponentOrigin &
pul::core::SceneBundle::StoredDebugPlayerOriginComponent() {
  return impl->storedDebugPlayerOriginComponent;
}

pul::core::HudInfo & pul::core::SceneBundle::Hud() {
  return impl->hudInfo;
}

pul::core::HudInfo const & pul::core::SceneBundle::Hud() const {
  return impl->hudInfo;
}

entt::registry & pul::core::SceneBundle::EnttRegistry() {
  return impl->enttRegistry;
}

entt::registry const & pul::core::SceneBundle::EnttRegistry() const {
  return impl->enttRegistry;
}

//------------------------------------------------------------------------------

pul::core::RenderBundle pul::core::RenderBundle::Construct(
  pul::plugin::Info const & plugin
, SceneBundle & scene
) {
  pul::core::RenderBundle self;

  // update current & copy to previous to make both instances valid
  self.Update(plugin, scene);
  self.previous = self.current;

  return self;
}

void pul::core::RenderBundle::Update(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {
  previous = std::move(current);

  current.playerOrigin = scene.playerOrigin;
  current.cameraOrigin = scene.cameraOrigin;
  current.playerCenter = scene.playerCenter;

  plugin.UpdateRenderBundleInstance(scene, current);
}

pul::core::RenderBundleInstance
pul::core::RenderBundle::Interpolate(
  pul::plugin::Info const & plugin
, float const msDeltaInterp
) {
  pul::core::RenderBundleInstance instance;

  float interp = msDeltaInterp;

  if (!debugUseInterpolation) {
    interp = 1.0f;
  }

  instance.playerOrigin =
    glm::mix(previous.playerOrigin, current.playerOrigin, interp);

  instance.cameraOrigin =
    glm::mix(
      glm::vec2(previous.cameraOrigin)
    , glm::vec2(current.cameraOrigin)
    , interp
    );

  instance.playerCenter =
    glm::mix(
      glm::vec2(previous.cameraOrigin)
    , glm::vec2(current.cameraOrigin)
    , interp
    );

  instance.msDeltaInterp = interp;

  plugin.Interpolate(msDeltaInterp, previous, current, instance);

  return instance;
}
