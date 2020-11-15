#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace pul::physics {

  enum class IntersectorType : size_t {
    Point  = 0x10000000
  , Ray    = 0x20000000
  , Circle = 0x40000000
  };

  struct IntersectorPoint {
    static IntersectorType constexpr type = IntersectorType::Point;

    // inputs
    glm::i32vec2 origin;
  };

  // this only works with bounding boxes
  struct IntersectorCircle {
    static IntersectorType constexpr type = IntersectorType::Circle;

    // inputs
    glm::i32vec2 origin;
    float radius;
  };

  struct IntersectorRay {
    static IntersectorType constexpr type = IntersectorType::Ray;

    // inputs
    glm::i32vec2 beginOrigin;
    glm::i32vec2 endOrigin;

    static IntersectorRay Construct(
      glm::vec2 const beginOrigin, glm::vec2 const endOrigin
    );
  };

  struct IntersectionResults {
    bool collision = false;
    glm::i32vec2 origin = glm::i32vec2(0);

    size_t imageTileIdx = -1ul, tilesetIdx = -1ul;
  };

  struct EntityIntersectionResults {
    bool collision = false;

    std::vector<std::pair<glm::i32vec2 /*origin*/, entt::entity>> entities;
  };

  // queries for debug purposes
  struct DebugQueries {
    void Add(
      IntersectorPoint const & intersector
    , pul::physics::IntersectionResults results
    );

    void Add(
      IntersectorRay const & intersector
    , pul::physics::IntersectionResults results
    );

    std::vector<
      std::pair<
        pul::physics::IntersectorPoint
      , pul::physics::IntersectionResults
      >
     > intersectorPoints;

    std::vector<
      std::pair<
        pul::physics::IntersectorRay
      , pul::physics::IntersectionResults
      >
    > intersectorRays;
  };

}
