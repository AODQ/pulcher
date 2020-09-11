#include <pulcher-core/log.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-physics/tileset.hpp>

#include <imgui/imgui.hpp>

#include <span>
#include <vector>

namespace {

// basically, when doings physics, we want tile lookups to be cached / quick,
// and we only want to do one tile intersection test per tile-grid. In other
// words, while there may be multiple tilesets contributing to the
// collision layer, there is still only one collision layer

struct TilemapLayer {
  std::vector<pulcher::physics::Tileset const *> tilesets;

  struct TileInfo {
    size_t imageTileIdx = -1ul;
    size_t tilesetIdx = -1ul;
    glm::vec2 origin;
  };

  std::vector<TileInfo> tileInfo;
  uint32_t width;
};

TilemapLayer tilemapLayer;

} // -- namespace

extern "C" {

pulcher::physics::Tileset ProcessTileset(
  pulcher::gfx::Image const & image
) {
  pulcher::physics::Tileset tileset;
  tileset.tiles.reserve(image.width * image.height / 32ul);

  // iterate thru every tile
  for (size_t tileX = 0ul; tileX < image.width  / 32ul; ++ tileX)
  for (size_t tileY = 0ul; tileY < image.height / 32ul; ++ tileY) {
    pulcher::physics::Tile tile;

    auto constexpr gridSize = pulcher::physics::Tile::gridSize;

    // iterate thru every texel of the tile, stratified by physics tile GridSize
    for (size_t texelX = 0ul; texelX < 32ul; texelX += 32ul/gridSize)
    for (size_t texelY = 0ul; texelY < 32ul; texelY += 32ul/gridSize) {

      size_t const
        localTexelX = tileX*32ul + texelX
      , localTexelY = tileY*32ul + texelY
      , gridTexelX = static_cast<size_t>(texelX*(gridSize/32.0f))
      , gridTexelY = static_cast<size_t>(texelY*(gridSize/32.0f))
      ;

      float const alpha = image.data[image.Idx(localTexelX, localTexelY)].a;

      tile.signedDistanceField[gridTexelX][gridTexelY] = alpha;
    }

    tileset.tiles.emplace_back(tile);
  }

  return tileset;
}

void ClearMapGeometry() {
  tilemapLayer = {};
}

void LoadMapGeometry(
  std::vector<pulcher::physics::Tileset const *> const & tilesets
, std::vector<std::span<size_t>>                 const & mapTileIndices
, std::vector<std::span<glm::u32vec2>>           const & mapTileOrigins
) {
  ClearMapGeometry();

  // -- assert tilesets.size == mapTileIndices.size == mapTileOrigins.size
  if (tilesets.size() != mapTileOrigins.size()) {
    spdlog::critical("mismatching size on tilesets & map tile origins");
    return;
  }

  if (mapTileIndices.size() != mapTileOrigins.size()) {
    spdlog::critical("mismatching size on map tile indices/origins");
    return;
  }

  // -- compute max width/height of tilemap
  uint32_t width = 0ul, height = 0ul;
  for (auto & tileOrigins : mapTileOrigins)
  for (auto & origin : tileOrigins) {
    width = std::max(width, origin.x);
    height = std::max(height, origin.y);
  }
  ::tilemapLayer.width = width;

  // copy tilesets over
  ::tilemapLayer.tilesets =
    decltype(::tilemapLayer.tilesets){tilesets.begin(), tilesets.end()};

  // resize
  ::tilemapLayer.tileInfo.resize(width * height);

  // cache tileset info for quick tile fetching
  for (size_t tilesetIdx = 0ul; tilesetIdx < tilesets.size(); ++ tilesetIdx) {
    auto const & tileIndices = mapTileIndices[tilesetIdx];
    auto const & tileOrigins = mapTileOrigins[tilesetIdx];

    for (size_t i = 0ul; i < tileIndices.size(); ++ i) {
      auto const & imageTileIdx = tileIndices[i];
      auto const & tileOrigin   = tileOrigins[i];

      size_t const tileIdx = tileOrigin.y / width + tileOrigin.x;
      if (tileIdx >= ::tilemapLayer.tileInfo.size()) {
        spdlog::error("tile is OOB from tilemap layer");
        continue;
      }

      auto & tile = ::tilemapLayer.tileInfo[tileIdx];
      if (tile.imageTileIdx != -1ul) {
        spdlog::error("multiple tiles are intersecting on the collision layer");
        continue;
      }

      tile.tilesetIdx   = tilesetIdx;
      tile.imageTileIdx = imageTileIdx;
      tile.origin       = tileOrigin;
    }
  }
}

void ProcessPhysics(pulcher::physics::BufferedQueries & queries) {

  for (auto & point : queries.GetComputing().intersectorPoints) {
    // -- get physics tile from acceleration structure
    size_t const tileIdx =
      point.origin.y / 32ul / ::tilemapLayer.width + point.origin.x / 32ul
    ;

    auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];
    auto const * tileset = ::tilemapLayer.tilesets[tileInfo.tilesetIdx];

    pulcher::physics::Tile const & physicsTile =
      tileset->tiles[tileInfo.imageTileIdx];

    // -- get absolute texel origin of intersection
    auto texelOrigin = glm::u32vec2(point.origin.x%32u, point.origin.y%32u);

    // -- compute intersection SDF and accel hints
    if (physicsTile.signedDistanceField[texelOrigin.x][texelOrigin.y] > 0.0f) {
      point.outputCollision = true;
    }
  }

  /* for (auto & ray : queries.intersectorRays) { */
  /* } */

  // submit results to queries (will perform copying of output intersections to
  // results and ready it to be written to again)
  queries.GetComputing().Submit();

  // swap as buffer is ready to be used externally again
  queries.Swap();
}

void UiRender(pulcher::physics::BufferedQueries & queries) {
  ImGui::Begin("Physics");

  ImGui::Text("buffer query idx %lu", queries.QueryIdx());
  static bool collisionDetection = false;
  static size_t queryIdx = -1ul;
  static bool collision = false;
  static bool nextFrameCol = false;
  if (ImGui::Button("point-collision detection test")) {
    collisionDetection = true;
  }

  if (collisionDetection) {
    ImGui::Text("OK, click the texel!");
  }

  if (nextFrameCol) {
    collision = queries.Get().RetrieveQuery(queryIdx).collision;
    queryIdx = -1ul;
    nextFrameCol = false;
  }

  if (collisionDetection && pulcher::gfx::LeftMousePressed()) {
    pulcher::physics::IntersectorPoint point;
    point.origin.x = pulcher::gfx::MouseX();
    point.origin.y = pulcher::gfx::MouseY();

    queryIdx = queries.Get().AddQuery(point);
    nextFrameCol = true;
  }

  ImGui::End();
}

} // -- extern C
