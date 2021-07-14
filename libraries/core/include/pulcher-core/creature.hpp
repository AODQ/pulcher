#pragma once

#include <pulcher-core/enum.hpp>

#include <array>
#include <variant>

namespace pul::animation { struct ComponentInstance; }
namespace entt { enum class entity : uint32_t; }

namespace pul::core {

  // combined with animation
  struct ComponentCreatureLump {
    struct StateIdle {
      int32_t timer = 1;
    };
    struct StateWalk {
      int32_t timerUntilIdle = 1;
    };
    struct StateRush {
      entt::entity targetEntity;
      bool willChargePlayer = false;
    };
    struct StateFlee {
      int32_t timerUntilIdle = 1;
    };
    struct StateDodge {
      bool hasInitiated = false;
      glm::vec2 airVelocity = {};
    };
    struct StateAttackPoison {
    };
    struct StateAttackKick {
    };
    struct StateBackflip {
      /* enum class ActionState { */
      /*   jk */
      /* }; */

      bool hasInitiated = false;
      glm::vec2 airVelocity = {};
    };

    struct MovementStateFlip {
    };
    struct MovementStateSlide {
    };

    glm::vec2 origin;

    std::variant<
      StateIdle, StateWalk, StateRush, StateDodge, StateAttackPoison,
      StateAttackKick, StateBackflip, StateFlee
    > state
      = StateIdle {}
    ;

    std::variant<MovementStateFlip, MovementStateSlide> movementState
      = MovementStateFlip {}
    ;

    bool movementStateActive = false;
    bool checkForGrounded = true;
    bool hasRecentlyAttacked = false;
  };

  struct ComponentCreatureMoldWing {
    glm::vec2 origin;

    struct StateIdle {
      bool hanging = false;
    };
    struct StateRoam {
      glm::vec2 targetOrigin;
    };
    struct StateRush {
      float targetDirection;
      float timer;
    };

    std::variant<
      StateIdle, StateRoam, StateRush
    > state
      = StateIdle {}
    ;
  };

  struct ComponentCreatureVapivara {
    glm::vec2 origin;

    struct StateCalm {
      struct StateIdle {
        int32_t timer = 0;
      };
      struct StateRoam {
        bool mustTurn = false;
        int32_t timer = 0;
        int32_t direction = 0;
      };

      std::variant<
        StateIdle, StateRoam
      > state = StateIdle { .timer = 50 };
    };

    struct StateIntimidate { // aggressive + ranged
      struct StateStandUp {
        int32_t timer = 0;
      };
      struct StateAttackClose {
      };
      struct StateStandDown {
      };
      std::variant<
        StateStandUp,
        StateAttackClose,
        StateStandDown
      > state = StateStandUp { };
    };

    std::variant<
      StateCalm, StateIntimidate
    > state
      = StateCalm {}
    ;
  };
}
