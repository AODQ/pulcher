#pragma once

#include <pulcher-animation/animation.hpp>

#include <vector>
#include <unordered_map>

namespace pul::animation { struct Instance; }
namespace pul::core { struct RenderBundleInstance; }
namespace pul::core { struct SceneBundle; }

namespace plugin::animation {

  struct Interpolant {
    pul::animation::Instance instance;
  };
}

template <typename... T>
using InterpolantMap = std::unordered_map<size_t, T...>;

namespace plugin::animation {

  void RenderInterpolated(
    pul::core::SceneBundle const & scene
  , pul::core::RenderBundleInstance const & interpolatedBundle
  , std::vector<plugin::animation::Interpolant> const & instancesCurrent
  );

  void Interpolate(
    const float msDeltaInterp
  , InterpolantMap<plugin::animation::Interpolant> const & instancesPrevious
  , InterpolantMap<plugin::animation::Interpolant> const & instancesCurrent
  , std::vector<plugin::animation::Interpolant> & instancesOutput
  );
}
