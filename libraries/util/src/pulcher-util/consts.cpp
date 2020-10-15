#include <pulcher-util/consts.hpp>

float & pulcher::util::MsPerFrame() {
  static float msPerFrame = 1000.0f / 90.0f;
  return msPerFrame;
}

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
