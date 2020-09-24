#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace pulcher::physics {

  enum class IntersectorType : size_t {
    Point = 0x10000000
  , Ray   = 0x20000000
  };

  struct IntersectorPoint {
    static IntersectorType constexpr type = IntersectorType::Point;

    // inputs
    glm::i32vec2 origin;
  };

  struct IntersectorRay {
    static IntersectorType constexpr type = IntersectorType::Ray;

    // inputs
    glm::i32vec2 beginOrigin;
    glm::i32vec2 endOrigin;
  };

  struct IntersectionResults {
    bool collision = false;
    glm::i32vec2 origin = glm::i32vec2(0);

    size_t imageTileIdx = -1ul, tilesetIdx = -1ul;

    // TODO below can be removed in release I suppose
    glm::i32vec2 debugBeginOrigin = glm::i32vec2(0);
  };

  struct Queries {
    size_t AddQuery(IntersectorPoint const & intersector);
    size_t AddQuery(IntersectorRay const & intersector);

    pulcher::physics::IntersectionResults RetrieveQuery(size_t);

    std::vector<pulcher::physics::IntersectorPoint> intersectorPoints;
    std::vector<pulcher::physics::IntersectorRay>   intersectorRays;

    std::vector<pulcher::physics::IntersectionResults> intersectorResultsPoints;
    std::vector<pulcher::physics::IntersectionResults> intersectorResultsRays;

    // Swaps buffers around
    void Submit();
  };

}