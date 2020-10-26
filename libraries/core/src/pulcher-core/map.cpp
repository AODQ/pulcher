#include <pulcher-core/map.hpp>

#include <pulcher-util/enum.hpp>

std::string ToStr(pul::core::TileOrientation orientation) {
  std::string str;
  auto const tileOrientation = Idx(orientation);

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipHorizontal)) {
    str += "horizontal";
  }

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipVertical)) {
    if (str.size() != 0ul) { str += " | "; }
    str += "vertical";
  }

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipDiagonal)) {
    if (str.size() != 0ul) { str += " | "; }
    str += "diagonal";
  }

  return "(" + str + ")";
}
