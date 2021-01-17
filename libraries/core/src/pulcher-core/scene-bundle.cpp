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
  pul::core::ComponentOrigin storedDebugPlayerOriginComponent;
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

pul::core::ComponentOrigin &
pul::core::SceneBundle::StoredDebugPlayerOriginComponent() {
  return impl->storedDebugPlayerOriginComponent;
}

pul::core::HudInfo & pul::core::SceneBundle::Hud() {
  return impl->hudInfo;
}

entt::registry & pul::core::SceneBundle::EnttRegistry() {
  return impl->enttRegistry;
}

//------------------------------------------------------------------------------

pul::core::RenderBundle pul::core::RenderBundle::Construct(
  SceneBundle const & scene
) {
  pul::core::RenderBundle self;

  // update current & copy to previous to make both instances valid
  self.Update(scene);
  self.previous = self.current;

  return self;
}

void pul::core::RenderBundle::Update(pul::core::SceneBundle const & scene) {

  previous = std::move(current);

  current.playerOrigin = scene.playerOrigin;
  current.cameraOrigin = scene.cameraOrigin;
  current.playerCenter = scene.playerCenter;
}

pul::core::RenderBundleInstance
pul::core::RenderBundle::Interpolate(
  float const msDeltaInterp
) {
  pul::core::RenderBundleInstance instance;

  instance.playerOrigin =
    glm::mix(previous.playerOrigin, current.playerOrigin, msDeltaInterp);

  instance.cameraOrigin =
    glm::mix(
      glm::vec2(previous.cameraOrigin)
    , glm::vec2(current.cameraOrigin)
    , msDeltaInterp
    );

  instance.playerCenter =
    glm::mix(
      glm::vec2(previous.cameraOrigin)
    , glm::vec2(current.cameraOrigin)
    , msDeltaInterp
    );

  instance.msDeltaInterp = msDeltaInterp;

  return instance;
}
