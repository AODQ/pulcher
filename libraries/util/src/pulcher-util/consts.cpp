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
