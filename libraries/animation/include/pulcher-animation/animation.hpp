#pragma once

#include <pulcher-gfx/spritesheet.hpp>

#include <sokol/gfx.hpp>

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
      glm::u32vec2 tile;
    };

    struct State {
      std::vector<Component> components;
      float msDeltaTime;

      void (*Update)(State & state) = nullptr;
    };

    struct Piece {
      std::map<std::string, pulcher::animation::Animator::State> states;
      glm::u32vec2 dimensions, origin;
      uint8_t renderOrder; // valid from 0 .. 100
    };

    // -- members
    pulcher::gfx::Spritesheet spritesheet;
    std::map<std::string, pulcher::animation::Animator::Piece> pieces;
    std::string label;
  };

  struct System {
    std::map<std::string, std::shared_ptr<Animator>> animators;

    sg_pipeline sgPipeline;
    sg_shader sgProgram;
  };

  struct Instance {
    std::shared_ptr<Animator> animator;

    std::map<std::string, std::string> pieceToState;

    sg_buffer sgBufferOrigin;
    sg_buffer sgBufferUvCoord;
    sg_bindings sgBindings;
    size_t drawCallCount;
  };

  struct ComponentInstance {
    pulcher::animation::Instance instance;
  };

}
