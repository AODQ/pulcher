#pragma once

#include <pulcher-core/pickup.hpp>
#include <pulcher-util/enum.hpp>

#include <array>

namespace pul::audio {
  struct System {
    bool playerJumped = false;
    bool playerDashed = false;
    bool playerSlided = false;
    bool playerStepped = false;
    bool playerTaunted = false;
    bool playerLanded = false;
    size_t envLanded = -1ul;

    std::array<bool, Idx(pul::core::PickupType::Size)> pickup {{ false }};
  };
}
