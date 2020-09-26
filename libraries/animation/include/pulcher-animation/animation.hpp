#pragma once

#include <pulcher-gfx/spritesheet.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace pulcher::animation {

  /*

    Animator
      |
      |           - State - components...
      "---- Piece - State - components...
      |           - State - components...
      ...
      |
      "---- Piece


  For example, player has multiple pieces (head, legs, body, hands, etc)
    Each piece has multiple states dictated by some form of logic (running,
    walking, etc)

    Each state has a list of components that are animated thru using the State

  */

  struct Animator {
    // -- structs
    struct Component {
      size_t tileX, tileY;
    };

    struct State {
      std::vector<Component> components;
      float msDeltaTime;

      void (*Update)(State & state) = nullptr;
    };

    struct Piece {
      std::map<std::string, pulcher::animation::Animator::State> states;
    };

    // -- members
    pulcher::gfx::Spritesheet spritesheet;
    std::map<std::string, pulcher::animation::Animator::Piece> pieces;
    std::string label;
  };

  struct System {
    std::map<std::string, std::shared_ptr<Animator>> animators;
  };

  struct Instance {
    std::shared_ptr<Animator> animator;
  };
}
