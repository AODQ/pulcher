#pragma once

#include <string>
#include <vector>

namespace pul::gfx {
  struct Image {
    size_t width = 0ul, height = 0ul;
    std::vector<glm::u8vec4> data;
    std::string filename;

    static Image Construct(char const * filename);

    size_t Idx(size_t x, size_t y) const { return y*this->width + x; }
  };
}
