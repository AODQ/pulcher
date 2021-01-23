#pragma once

#include <glm/fwd.hpp>

#include <cstddef>
#include <array>

namespace pul::util {
  float constexpr MsPerFrame = 16.0f;

  std::array<glm::vec2, 6> TriangleVertexArray();
}

namespace pul {
  float constexpr Pi = 3.14159265358979323846f;
  float constexpr Pi_2 = Pi/2.0f;
  float constexpr Pi_4 = Pi/4.0f;
  float constexpr Tau = Pi * 2.0f;

  enum class Direction : int32_t {
    UpperLeft, Up, UpperRight
  , Left,          Right
  , LowerLeft, Down, LowerRight
  , Size
  , CardinalSize
  , None
  };

  Direction ToDirection(float x, float y);
}
