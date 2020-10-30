#pragma once

#include <pulcher-gfx/spritesheet.hpp>

#include <sokol/gfx.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace pul::animation {

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
      glm::u32vec2 tile = {};
      glm::i32vec2 originOffset = {};
      uint32_t msDeltaTimeOverride = -1u;
    };

    struct ComponentPart {
      float rangeMax = 0.0f;
      std::array<std::vector<Component>, 2> data = {};
    };

    struct State {
      std::vector<ComponentPart> components;
      uint32_t msDeltaTime;
      bool rotationMirrored = false;
      bool originInterpolates = false;
      bool rotatePixels = false;
      bool flipXAxis = false;
      bool loops = false;

      size_t ComponentPartIdxLookup(float const angle);

      float MsDeltaTime(Component & component) {
        return
          static_cast<float>(
              component.msDeltaTimeOverride == -1u
            ? msDeltaTime : component.msDeltaTimeOverride
          )
        ;
      }

      std::vector<Component> * ComponentLookup(
        bool const flip, float const angle
      );

      void (*Update)(State & state) = nullptr;
    };

    struct Piece {
      std::map<std::string, pul::animation::Animator::State> states = {};
      glm::u32vec2 dimensions = {};
      glm::i32vec2 origin = {};
      uint8_t renderOrder = 0; // valid from 0 .. 100
    };

    struct SkeletalPiece {
      std::string label;
      glm::i32vec2 origin = {};
      std::vector<SkeletalPiece> children = {};
    };

    // -- members
    pul::gfx::Spritesheet spritesheet;
    std::map<std::string, pul::animation::Animator::Piece> pieces;
    std::vector<SkeletalPiece> skeleton;
    std::string label;
    std::string filename;
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
      bool flip = false;
      float angle = 0.0f;
      bool visible = true;
      bool animationFinished = false;

      glm::mat3 cachedLocalSkeletalMatrix = glm::mat3(1.0f);

      void Apply(std::string const & nLabel, bool force = false) {
        if (force || label != nLabel) {
          label = nLabel;
          deltaTime = 0.0f;
          componentIt = 0ul;
          animationFinished = false;
        }
      }
    };

    std::map<std::string, StateInfo> pieceToState;

    // if set to false, the matrix will no longer be recalculated every render
    // frame. This allows an animation matrix to not have to be set every frame
    bool automaticCachedMatrixCalculation = true;

    bool hasCalculatedCachedInfo = false;

    glm::vec2 origin = glm::vec2(0.0);

    sg_buffer sgBufferOrigin = {};
    sg_buffer sgBufferUvCoord = {};
    sg_bindings sgBindings = {};
    size_t drawCallCount = 0ul;

    bool visible = true;

    // keep origin/uv coord buffer data around for streaming updates
    std::vector<glm::vec2> uvCoordBufferData;
    std::vector<glm::vec3> originBufferData;
  };

  struct ComponentInstance {
    pul::animation::Instance instance;
  };

}
