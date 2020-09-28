#pragma once

#include <glm/fwd.hpp>

#include <cstddef>
#include <array>

namespace pulcher::util {
  float constexpr msPerFrame = 1000.0f / 90.0f;

  std::array<glm::vec2, 6> TriangleVertexArray();
}
