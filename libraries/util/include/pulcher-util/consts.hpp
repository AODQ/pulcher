#pragma once

#include <glm/fwd.hpp>

#include <cstddef>
#include <array>

namespace pulcher::util {
  float & MsPerFrame();

  std::array<glm::vec2, 6> TriangleVertexArray();
}

namespace pul {
  float constexpr Pi = 3.14159265358979323846f;
  float constexpr Tau = 2.0f * 3.14159265358979323846f;
}
