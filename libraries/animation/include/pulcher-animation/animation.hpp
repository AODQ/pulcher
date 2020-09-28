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

    Each state has a list of components that are animated thru using the State.

    Pieces are connected together using "parent" and thus they get
    skeletal-esque animation as their offsets are compounded together.

  TODO change std::string to size_t and use std::hash

  */

  struct Animator {
    // -- structs
    struct Component {
      glm::u32vec2 tile;
      glm::i32vec2 originOffset = {};
    };

    struct State {
      std::vector<Component> components;
      float msDeltaTime;

      void (*Update)(State & state) = nullptr;
    };

    struct Piece {
      std::map<std::string, pulcher::animation::Animator::State> states;
      glm::u32vec2 dimensions;
      glm::i32vec2 origin;
      uint8_t renderOrder; // valid from 0 .. 100
    };

    struct SkeletalPiece {
      std::string label;
      std::vector<SkeletalPiece> children;
    };

    // -- members
    pulcher::gfx::Spritesheet spritesheet;
    std::map<std::string, pulcher::animation::Animator::Piece> pieces;
    std::vector<SkeletalPiece> skeleton;
    std::string label;
  };

  struct System {
    std::map<std::string, std::shared_ptr<Animator>> animators;

    sg_pipeline sgPipeline;
    sg_shader sgProgram;
  };

  struct Instance {
    std::string animatorLabel;
    std::shared_ptr<Animator> animator;

    struct StateInfo {
      std::string label;
      float deltaTime = 0.0f;
      size_t componentIt = 0ul;
    };

    std::map<std::string, StateInfo> pieceToState;

    sg_buffer sgBufferOrigin;
    sg_buffer sgBufferUvCoord;
    sg_bindings sgBindings;
    size_t drawCallCount;

    // keep origin/uv coord buffer data around for streaming updates
    std::vector<glm::vec2> uvCoordBufferData;
    std::vector<glm::vec3> originBufferData;
  };

  struct ComponentInstance {
    pulcher::animation::Instance instance;
  };

}
