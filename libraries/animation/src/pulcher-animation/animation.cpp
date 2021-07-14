#include <pulcher-animation/animation.hpp>

#include <pulcher-util/log.hpp>
#include <pulcher-util/random.hpp>

size_t pul::animation::Animator::State::VariationIdxLookup(
  VariationRuntimeInfo const & variationRti
) {
  switch (variationType) {
    default: return 0ul;
    case pul::animation::VariationType::Range:
      for (size_t compIdx = 0ul; compIdx < variations.size(); ++ compIdx) {
        auto & variation = variations[compIdx];
        if (variationRti.range.angle <= variation.range.rangeMax) {
          return compIdx;
          break;
        }
      }

      return 0ul;

    case pul::animation::VariationType::Random:
      return variationRti.random.idx;

    case pul::animation::VariationType::Normal:
      return 0ul;
  }
}

std::vector<pul::animation::Component> *
pul::animation::Animator::State::ComponentLookup(
  VariationRuntimeInfo const & variationRti
) {
  if (variations.size() == 0ul) { return nullptr; }

  auto & variation = variations[this->VariationIdxLookup(variationRti)];

  switch (variationType) {
    default: return nullptr;
    case pul::animation::VariationType::Range:
      // apply flipping if requested & possible
      if (variationRti.range.flip && variation.range.data[1].size() > 0ul) {
        return &variation.range.data[1];
      }

      // default / normal state
      return &variation.range.data[0];

    case pul::animation::VariationType::Random:
      return &variation.random.data;

    case pul::animation::VariationType::Normal:
      return &variation.normal.data;
  }
}

void pul::animation::Instance::StateInfo::Apply(
  std::string const & nLabel, bool force
) {
  if (!force && label == nLabel) { return; }
  label = nLabel;
  deltaTime = 0.0f;
  componentIt = 0ul;
  animationFinished = false;

  variationRti = {};

  auto const & state = animator->pieces[pieceLabel].states[label];

  switch (state.variationType) {
    default: spdlog::error("variation type default"); break;
    case pul::animation::VariationType::Normal: break;
    case pul::animation::VariationType::Range: break;
    case pul::animation::VariationType::Random:
      variationRti.random.idx =
        pul::util::RandomInt32(0, state.variations.size()-1);
    break;
  }
}

char const * ToStr(pul::animation::VariationType type) {
  switch (type) {
    default: return "n/a";
    case pul::animation::VariationType::Range:  return "range";
    case pul::animation::VariationType::Random: return "random";
    case pul::animation::VariationType::Normal: return "normal";
  }
}

pul::animation::VariationType pul::animation::ToVariationType(
  char const * label
) {
  auto labelStr = std::string{label};
  if (labelStr == "range") { return pul::animation::VariationType::Range; }
  if (labelStr == "random") { return pul::animation::VariationType::Random; }
  if (labelStr == "normal") { return pul::animation::VariationType::Normal; }
  spdlog::critical("unknown variation type {}", label);
  return pul::animation::VariationType::Normal;
}
