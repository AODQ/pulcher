#pragma once

#include <pulcher-core/enum.hpp>

#include <array>
#include <variant>

namespace pul::animation { struct ComponentInstance; }
namespace entt { enum class entity : uint32_t; }

namespace pul::core {

  // combined with animation
  struct ComponentCreatureLump {
    glm::vec2 origin;

    int32_t idleTimer;

    bool checkForGrounded = true;

    struct StateIdle {
      int32_t timer = 1;
    };
    struct StateWalk {
      int32_t timerUntilIdle = 1;
    };
    struct StateRush {
      entt::entity targetEntity;
    };
    struct StateDodge {
      bool hasInitiated = false;
      glm::vec2 airVelocity = {};
    };

    std::variant<StateIdle, StateWalk, StateRush, StateDodge> state =
      StateIdle {}
    ;

    struct StateFlip {
      bool active = false;
    };
    StateFlip stateFlip;
  };
}
