#pragma once

#include <string>

namespace pul::core {
  enum class TileOrientation : size_t {
    FlipHorizontal = 0b001
  , FlipVertical   = 0b010
  , FlipDiagonal   = 0b100
  , None   = 0b000
  };
}

std::string ToStr(pul::core::TileOrientation);
