#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-physics/tileset.hpp>
#include <pulcher-util/log.hpp>

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

    bool Valid() const { return tilesetIdx != -1ul; }
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
  tileset.tiles.reserve((image.width / 32ul) * (image.height / 32ul));

  // iterate thru every tile
  for (size_t tileY = 0ul; tileY < image.height / 32ul; ++ tileY)
  for (size_t tileX = 0ul; tileX < image.width  / 32ul; ++ tileX) {
    pulcher::physics::Tile tile;

    auto constexpr gridSize = pulcher::physics::Tile::gridSize;

    // iterate thru every texel of the tile, stratified by physics tile GridSize
    for (size_t texelY = 0ul; texelY < 32ul; texelY += 32ul/gridSize)
    for (size_t texelX = 0ul; texelX < 32ul; texelX += 32ul/gridSize) {

      size_t const
        imageTexelX = tileX*32ul + texelX
      , imageTexelY = tileY*32ul + texelY
      , gridTexelX = static_cast<size_t>(texelX*(gridSize/32.0f))
      , gridTexelY = static_cast<size_t>(texelY*(gridSize/32.0f))
      ;

      float const alpha = image.data[(image.height-imageTexelY)*image.width + imageTexelX].a;

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
    width = std::max(width, origin.x+1);
    height = std::max(height, origin.y+1);
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

    PUL_ASSERT(tilesets[tilesetIdx], continue;);

    for (size_t i = 0ul; i < tileIndices.size(); ++ i) {
      auto const & imageTileIdx = tileIndices[i];
      auto const & tileOrigin   = tileOrigins[i];

      PUL_ASSERT_CMP(
        imageTileIdx, <, tilesets[tilesetIdx]->tiles.size(), continue;
      );

      size_t const tileIdx = tileOrigin.y * width + tileOrigin.x;

      PUL_ASSERT_CMP(tileIdx, <, ::tilemapLayer.tileInfo.size(), continue;);

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

void ProcessPhysics(pulcher::core::SceneBundle & scene) {

  auto & queries = scene.physicsQueries;

  auto computingIntersectorPoints = std::move(queries.intersectorPoints);
  auto computingIntersectorRays = std::move(queries.intersectorRays);

  queries.intersectorRays   = {};
  queries.intersectorPoints = {};
  queries.intersectorResultsPoints   = {};
  queries.intersectorResultsRays = {};

  for (auto & point : computingIntersectorPoints) {
    // -- get physics tile from acceleration structure
    size_t const tileIdx =
      point.origin.y / 32ul * ::tilemapLayer.width + point.origin.x / 32ul
    ;

    PUL_ASSERT_CMP(
      tileIdx, <, ::tilemapLayer.tileInfo.size()
    , queries.intersectorResultsPoints.emplace_back(false); continue;
    );

    auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];
    auto const * tileset = ::tilemapLayer.tilesets[tileInfo.tilesetIdx];

    if (!tileInfo.Valid()) {
      queries.intersectorResultsPoints.emplace_back(false);
      continue;
    }

    pulcher::physics::Tile const & physicsTile =
      tileset->tiles[tileInfo.imageTileIdx];

    // -- get absolute texel origin of intersection
    auto texelOrigin = glm::u32vec2(point.origin.x%32u, point.origin.y%32u);

    // -- compute intersection SDF and accel hints
    if (physicsTile.signedDistanceField[texelOrigin.x][texelOrigin.y] > 0.0f) {
      queries
        .intersectorResultsPoints
        .emplace_back(
          true, point.origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
        );
    } else {
      queries.intersectorResultsPoints.emplace_back(false);
    }
  }

  /* for (auto & ray : queries.intersectorRays) { */
  /* } */
}

void UiRender(pulcher::core::SceneBundle &) {
  ImGui::Begin("Physics");

  ImGui::Text("tilemap width %u", ::tilemapLayer.width);
  ImGui::Text("tile info size %lu", ::tilemapLayer.tileInfo.size());

  ImGui::End();
}

} // -- extern C
