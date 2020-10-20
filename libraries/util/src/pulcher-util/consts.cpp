#include <pulcher-util/consts.hpp>

std::array<glm::vec2, 6> pulcher::util::TriangleVertexArray() {
  return
    std::array<glm::vec2, 6> {{
      {0.0f,  0.0f}
    , {1.0f,  1.0f}
    , {1.0f,  0.0f}

    , {0.0f,  0.0f}
    , {0.0f,  1.0f}
    , {1.0f,  1.0f}
    }};
}

pul::Direction pul::ToDirection(float x, float y) {
  if (x < 0.0f) {
    if (y < 0.0f) { return pul::Direction::UpperLeft; }
    else if (y > 0.0f) { return pul::Direction::LowerLeft; }
    return pul::Direction::Left;
  } else if (x > 0.0f) {
    if (y < 0.0f) { return pul::Direction::UpperRight; }
    else if (y > 0.0f) { return pul::Direction::LowerRight; }
    return pul::Direction::Right;
  } else if (y < 0.0f) { return pul::Direction::Up; }
  else if (y > 0.0f) { return pul::Direction::Down; }

  return pul::Direction::None;
}
