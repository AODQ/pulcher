#pragma once

#include <array>
#include <vector>

// SDF tilesets

namespace pul::physics {

  enum struct TileIntersectAccelerationHint : uint8_t {
    Default = 0b0 // no info, must use SDF
  , Empty = 0b0001 // empty tile, intersection can be skipped as no collision
  , Full = 0b0010 // tile has no alpha, intersection gaurunteed to collide
  };

  struct Tile {
    static size_t constexpr gridSize = 32ul;
    std::array<std::array<float, gridSize>, gridSize> signedDistanceField;
    std::array<std::array<uint8_t, gridSize>, gridSize> accelerationHints;
  };

  struct Tileset {
    std::vector<pul::physics::Tile> tiles;
  };
}
