#pragma once

#include <glm/glm.hpp>

namespace pul::util {
  inline bool CalculateTileIndices(
    size_t & tileIdx, glm::u32vec2 & texelOrigin
  , glm::i32vec2 const & origin
  , size_t const tilemapWidth, size_t const tilemapLength
  ) {
    tileIdx = origin.y / 32ul * tilemapWidth + origin.x / 32ul;
    if (tileIdx >= tilemapLength) { return false; }

    texelOrigin = glm::u32vec2(origin.x%32ul, origin.y%32ul);
    return true;
  }
}

namespace pul::physics {
  template <typename Fn>
  void BresenhamLine(glm::ivec2 f0, glm::ivec2 f1, Fn && fn) {
    bool steep = false;
    if (glm::abs(f0.x-f1.x) < glm::abs(f0.y-f1.y)) {
      std::swap(f0.x, f0.y);
      std::swap(f1.x, f1.y);
      steep = true;
    }

    if (f0.x > f1.x) {
      int32_t
        dx = f0.x-f1.x
      , dy = f1.y-f0.y
      , derror = glm::abs(dy)*2
      , error = 0.0f
      , stepy = f1.y > f0.y ? +1.0f : -1.0f
      ;

      for (glm::ivec2 f = f0; f.x >= f1.x; -- f.x) {
        if (steep) { fn(f.y, f.x); } else { fn(f.x, f.y); }
        error += derror;
        if (error > dx) {
          f.y += stepy;
          error -= dx*2;
        }
      }
    } else {
      int32_t
        dx = f1.x-f0.x
      , dy = f1.y-f0.y
      , derror = glm::abs(dy)*2
      , error = 0.0f
      , stepy = f1.y > f0.y ? +1.0f : -1.0f
      ;

      for (glm::ivec2 f = f0; f.x <= f1.x; ++ f.x) {
        if (steep) { fn(f.y, f.x); } else { fn(f.x, f.y); }
        error += derror;
        if (error > dx) {
          f.y += stepy;
          error -= dx*2;
        }
      }
    }

  }
}
