#pragma once

#include <vector>

#include <pulcher-animation/animation.hpp>

namespace pul::core { struct SceneBundle; }

namespace plugin::animation {
  void ComputeCache(
    pul::animation::Instance & instance
  , std::vector<pul::animation::Animator::SkeletalPiece> const & skeletals
  , glm::mat3 const skeletalMatrix
  , bool const skeletalFlip
  , float const skeletalRotation
  );

  void ComputeVertices(
    pul::animation::Instance & instance
  , bool forceUpdate = false
  );

  void LoadAnimations(
    pul::core::SceneBundle & scene
  );

  void ConstructInstance(
    pul::core::SceneBundle &
  , pul::animation::Instance & animationInstance
  , pul::animation::System & animationSystem
  , char const * label
  );

  void UpdateCache(pul::animation::Instance & instance);

  void UpdateCacheWithPrecalculatedMatrix(
    pul::animation::Instance & instance
  , glm::mat3 const & skeletalMatrix
  );

  void UpdateFrame(pul::core::SceneBundle & scene);

  void Shutdown(pul::core::SceneBundle & scene);

  void DebugUiDispatch(pul::core::SceneBundle & scene);
}
