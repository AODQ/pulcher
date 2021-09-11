#pragma once

#include <pulcher-gfx/sokol.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-util/common-components.hpp>
#include <pulcher-util/log.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace pul::animation {

  /*

    Check `docs/animation-system.dot` for an example layout

    AnimationSystem (json files)
    |
    |             - Piece   - State   - Variation   - Component
    Spritesheet --- Piece --- State --- Variation --- Component
                  - Piece   - State   - Variation   - Component


  For example, player has multiple pieces (head, legs, body, hands, etc)
    Each piece has multiple states dictated by some form of logic (running,
    walking, etc)

    Each state has a list of components that are animated thru using the State,
      however they are controlled by variation; thus allowing different
      animation states to be played with generic info, such as angle, random,
      or iteratively.

    Pieces are connected together using "parent" and thus they get
    skeletal-esque animation as their offsets are compounded together.

  TODO change std::string to size_t and use std::hash

  */

  struct Component {
    // FIXME , for some reason if you print tile.x here sometimes, especially
    //         within the editor, it prints out garbage values, although the
    //         vector itself displays correct values. It breaks `==` etc.
    glm::u32vec2 tile = {};
    glm::i32vec2 originOffset = {};
    uint32_t msDeltaTimeOverride = -1u;
    size_t numHitboxes = 0; // max 8
    std::array<pul::util::ComponentHitboxAABB, 8> hitboxes;
  };

  enum class VariationType {
    Range, Random, Normal, Size
  };

  VariationType ToVariationType(char const * label);

  struct VariationRuntimeInfo {
    struct Range {
      float angle; // this gets set by StateIno::angle, so don't set it yourself
      bool flip; // this gets set by StateInfo::flip; so don't set it yourself
    };
    struct Random {
      size_t idx;
    };
    struct Normal {
    };

    Range range;
    Random random;
    Normal normal;
  };

  struct Variation {
    struct Range {
      float rangeMax = 0.0f;
      std::array<std::vector<Component>, 2> data = {};
    };
    struct Random {
      std::vector<Component> data = {};
    };
    struct Normal {
      std::vector<Component> data = {};
    };
    Range range;
    Random random;
    Normal normal;

    VariationType type;
  };

  struct Animator {
    // -- structs
    struct State {
      VariationType variationType;
      std::vector<Variation> variations;
      uint32_t msDeltaTime;
      bool rotationMirrored = false;
      bool originInterpolates = false;
      bool rotatePixels = false;
      bool flipXAxis = false;
      bool loops = false;

      // TODO it should be Variation variation instead of
      //      std::vector<Variation> variations
      size_t VariationIdxLookup(VariationRuntimeInfo const &);

      float MsDeltaTime(Component & component) {
        return
          static_cast<float>(
              component.msDeltaTimeOverride == -1u
            ? msDeltaTime : component.msDeltaTimeOverride
          )
        ;
      }

      std::vector<Component> * ComponentLookup(VariationRuntimeInfo const &);

      void (*Update)(State & state) = nullptr;
    };

    struct Piece {
      std::map<std::string, pul::animation::Animator::State> states = {};
      glm::u32vec2 dimensions = {};
      glm::i32vec2 origin = {};
      int16_t renderDepth = 0; // valid from -127 .. 128
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
    glm::uvec2 uvCoordOffset = glm::uvec2(0);
    std::string label;
    std::string filename;
  };

  struct System {
    std::map<std::string, std::shared_ptr<Animator>> animators;

    sg_pipeline sgPipeline;
    sg_shader sgProgram;

    std::unique_ptr<pul::gfx::SgBuffer> sgBuffer = {};
    sg_bindings sgBindings = {};
  };

  struct Instance {
    std::shared_ptr<Animator> animator = {};

    struct StateInfo {
      std::string label;
      float deltaTime = 0.0f;
      size_t componentIt = 0ul;
      float angle = 0.0f;
      bool flip = false;
      bool visible = true;
      bool animationFinished = false;

      glm::vec2 uvCoordWrap = glm::vec2(1.0f);
      glm::vec2 vertWrap = glm::vec2(1.0f);
      bool flipVertWrap = false;

      std::shared_ptr<Animator> animator;
      std::string pieceLabel;

      VariationRuntimeInfo variationRti = {};

      glm::mat3 cachedLocalSkeletalMatrix = glm::mat3(0.0f);

      void Apply(std::string const & nLabel, bool force = false);
    };

    std::map<std::string, StateInfo> pieceToState = {};

    // if set to false, the matrix will no longer be recalculated every render
    // frame. This allows an animation matrix to not have to be set every frame
    bool automaticCachedMatrixCalculation = true;

    bool hasCalculatedCachedInfo = false;

    // TODO this should come from componentOrigin
    glm::vec2 origin = glm::vec2(0.0);

    size_t drawCallCount = 0ul;

    bool visible = true;

    // keep origin/uv coord buffer data around for streaming updates
    std::vector<glm::vec2> uvCoordBufferData = {};
    std::vector<glm::vec3> originBufferData = {};
  };

  struct ComponentInstance {
    // TODO move origin from Instance to here? as well as others?
    pul::animation::Instance instance;
  };

}

char const * ToStr(pul::animation::VariationType type);
