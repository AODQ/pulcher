#pragma once

#include <pulcher-audio/fmod-studio-guids.hpp>

#include <pulcher-core/pickup.hpp>
#include <pulcher-util/enum.hpp>

#include <vector>

namespace pul::audio {

  struct InstanceDispatch {
    fmodGuid const * guid = nullptr;

    struct Parameter { std::string key; float value; };
    std::vector<Parameter> parameters;
    glm::vec2 origin;
  };

  struct System {
    std::vector<InstanceDispatch> dispatches;

    bool playerJumped = false;
    bool playerDashed = false;
    bool playerSlided = false;
    bool playerStepped = false;
    bool playerTaunted = false;
    bool volniasHit = false;
    bool volniasChargePrimary = false;
    bool volniasChargeSecondary = false;
    bool volniasEndPrimary = false;
    bool volniasPrefirePrimary = false;
    bool volniasPrefireSecondary = false;
    size_t volniasFire = -1ul;
    bool playerLanded = false;
    size_t envLanded = -1ul;

    std::array<bool, Idx(pul::core::PickupType::Size)> pickup {{ false }};
  };
}
