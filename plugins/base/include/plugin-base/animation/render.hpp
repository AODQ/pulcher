#pragma once

#include <pulcher-animation/animation.hpp>

#include <vector>

namespace pul::animation { struct Instance; }
namespace pul::core { struct RenderBundleInstance; }
namespace pul::core { struct SceneBundle; }

namespace plugin::animation {

  struct Interpolant {
    pul::animation::Instance instance;
  };

  void RenderInterpolated(
    pul::core::SceneBundle const & scene
  , pul::core::RenderBundleInstance const & interpolatedBundle
  , std::vector<plugin::animation::Interpolant> const & instancesCurrent
  );

  void Interpolate(
    const float msDeltaInterp
  , std::vector<plugin::animation::Interpolant> const & instancesPrevious
  , std::vector<plugin::animation::Interpolant> const & instancesCurrent
  , std::vector<plugin::animation::Interpolant> & instancesOutput
  );
}
