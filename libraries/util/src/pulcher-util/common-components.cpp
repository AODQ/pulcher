#include <pulcher-util/common-components.hpp>
#include <pulcher-util/enum.hpp>

pul::util::HitboxTag operator|(
  pul::util::HitboxTag lhs, pul::util::HitboxTag rhs
) {
  return static_cast<pul::util::HitboxTag>(Idx(lhs) | Idx(rhs));
}

pul::util::HitboxTag operator&(
  pul::util::HitboxTag lhs, pul::util::HitboxTag rhs
) {
  return static_cast<pul::util::HitboxTag>(Idx(lhs) & Idx(rhs));
}

pul::util::HitboxTag operator&(
  pul::util::HitboxTag lhs, uint32_t rhs
) {
  return static_cast<pul::util::HitboxTag>(Idx(lhs) & rhs);
}
