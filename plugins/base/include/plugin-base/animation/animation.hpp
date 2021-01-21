#pragma once

#include <vector>

#include <pulcher-animation/animation.hpp>

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
}
