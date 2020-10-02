#include <pulcher-animation/animation.hpp>

size_t pulcher::animation::Animator::State::ComponentPartIdxLookup(
  float const angle
) {
  for (size_t compIdx = 0ul; compIdx < components.size(); ++ compIdx) {
    auto & component = components[compIdx];
    if (angle <= component.rangeMax) {
      return compIdx;
      break;
    }
  }

  return 0ul;
}

std::vector<pulcher::animation::Animator::Component> *
pulcher::animation::Animator::State::ComponentLookup(
  bool const flip, float const angle
) {
  if (components.size() == 0ul) { return nullptr; }

  auto & component = components[this->ComponentPartIdxLookup(angle)];

  // apply flipping if requested & possible
  if (flip && component.data[1].size() > 0ul) {
    return &component.data[1];
  }

  // default / normal state
  return &component.data[0];
}
