#include <plugin-base/physics/physics.hpp>

#include <plugin-base/debug/renderer.hpp>

#include <pulcher-core/map.hpp>
#include <pulcher-core/player.hpp> // hitbox
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-physics/tileset.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>
#include <pulcher-util/math.hpp>

#include <entt/entt.hpp>
#include <glad/glad.hpp>
#include <imgui/imgui.hpp>

#include <span>
#include <vector>

namespace {

bool showPhysicsQueries = true;

// basically, when doings physics, we want tile lookups to be cached / quick,
// and we only want to do one tile intersection test per tile-grid. In other
// words, while there may be multiple tilesets contributing to the
// collision layer, there is still only one collision layer

pul::physics::TilemapLayer tilemapLayer;

float CalculateSdfDistance(
  pul::physics::TilemapLayer::TileInfo const & tileInfo
, glm::u32vec2 texel
) {
  if (tileInfo.tilesetIdx == -1ul) { return 0.0f; }

  auto const * tileset = tilemapLayer.tilesets[tileInfo.tilesetIdx];

  if (!tileInfo.Valid()) { return 0.0f; }

  pul::physics::Tile const & physicsTile =
    tileset->tiles[tileInfo.imageTileIdx];

  // apply tile orientation
  auto const tileOrientation = Idx(tileInfo.orientation);

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipHorizontal))
    { texel.x = 31 - texel.x; }

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipVertical))
    { texel.y = 31 - texel.y; }

  if (tileOrientation & Idx(pul::core::TileOrientation::FlipDiagonal)) {
    std::swap(texel.x, texel.y);
  }

  // -- compute intersection SDF and accel hints
  return physicsTile.signedDistanceField[texel.x][texel.y];
}

glm::vec2 GetAabbMin(glm::vec2 const & aabbOrigin, glm::vec2 const & aabbDim) {
  glm::vec2 const p0 = aabbOrigin - aabbDim/2.0f;
  glm::vec2 const p1 = aabbOrigin + aabbDim/2.0f;
  return glm::min(p0, p1);
}

glm::vec2 GetAabbMax(glm::vec2 const & aabbOrigin, glm::vec2 const & aabbDim) {
  glm::vec2 const p0 = aabbOrigin - aabbDim/2.0f;
  glm::vec2 const p1 = aabbOrigin + aabbDim/2.0f;
  return glm::max(p0, p1);
}

bool IntersectionRayAabb(
  glm::vec2 const & rayBegin, glm::vec2 const & rayEnd
, glm::vec2 const & aabbOrigin, glm::vec2 const & aabbDim
, float & intersectionLength
) {
  glm::vec2 normal = glm::normalize(rayEnd - rayBegin);
  normal.x = normal.x == 0.0f ? 1.0f : 1.0f / normal.x;
  normal.y = normal.y == 0.0f ? 1.0f : 1.0f / normal.y;

  glm::vec2 const min = (GetAabbMin(aabbOrigin, aabbDim) - rayBegin) * normal;
  glm::vec2 const max = (GetAabbMax(aabbOrigin, aabbDim) - rayBegin) * normal;

  float tmin = glm::max(glm::min(min.x, max.x), glm::min(min.y, max.y));
  float tmax = glm::min(glm::max(min.x, max.x), glm::max(min.y, max.y));

  if (tmax < 0.0f || tmin > tmax)
    { return false; }

  float t = tmin < 0.0f ? tmax : tmin;
  intersectionLength = t;
  return t > 0.0f && t < glm::length(rayEnd - rayBegin);
}

bool IntersectionCircleAabb(
  glm::vec2 const & circleOrigin, float const circleRadius
, glm::vec2 const & aabbOrigin, glm::vec2 const & aabbDim
, glm::vec2 & closestOrigin
) {
  closestOrigin =
    glm::clamp(
      circleOrigin
    , GetAabbMin(aabbOrigin, aabbDim)
    , GetAabbMax(aabbOrigin, aabbDim)
    );

  return glm::length(circleOrigin - closestOrigin) <= circleRadius;
}

} // -- namespace

// -- plugin functions
void plugin::physics::EntityIntersectionRaycast(
  pul::core::SceneBundle & scene
, pul::physics::IntersectorRay const & ray
, pul::physics::EntityIntersectionResults & intersectionResults
) {
  auto & registry = scene.EnttRegistry();

  auto view =
    registry.view<
      pul::core::ComponentHitboxAABB
    , pul::core::ComponentOrigin
    >();

  intersectionResults.entities.clear();

  glm::vec2 const
    rayOriginBegin = glm::vec2(ray.beginOrigin)
  , rayOriginEnd = glm::vec2(ray.endOrigin)
  ;

  for (auto & entity : view) {
    auto const & hitbox = view.get<pul::core::ComponentHitboxAABB>(entity);
    auto const & origin = view.get<pul::core::ComponentOrigin>(entity);

    float intersectionLength;
    bool const intersection =
      ::IntersectionRayAabb(
        rayOriginBegin, rayOriginEnd
      , origin.origin, glm::vec2(hitbox.dimensions)
      , intersectionLength
      );

    if (intersection) {
      glm::vec2 const intersectionOrigin =
          rayOriginBegin
        + intersectionLength * glm::normalize(rayOriginEnd - rayOriginBegin)
      ;

      intersectionResults.collision = true;
      std::pair<glm::i32vec2, entt::entity> results;
      std::get<0>(results) = glm::i32vec2(glm::round(intersectionOrigin));
      std::get<1>(results) = entity;
      intersectionResults.entities.emplace_back(results);
    }
  }
}

void plugin::physics::EntityIntersectionCircle(
  pul::core::SceneBundle & scene
, pul::physics::IntersectorCircle const & circle
, pul::physics::EntityIntersectionResults & intersectionResults
) {
  auto & registry = scene.EnttRegistry();

  auto view =
    registry.view<
      pul::core::ComponentHitboxAABB
    , pul::core::ComponentOrigin
    >();

  intersectionResults.entities.clear();

  for (auto & entity : view) {
    auto const & hitbox = view.get<pul::core::ComponentHitboxAABB>(entity);
    auto const & origin = view.get<pul::core::ComponentOrigin>(entity);

    glm::vec2 closestOrigin;
    bool intersection =
      ::IntersectionCircleAabb(
        glm::vec2(circle.origin), circle.radius
      , origin.origin + glm::vec2(hitbox.offset), glm::vec2(hitbox.dimensions)
      , closestOrigin
      );

    if (intersection) {
      intersectionResults.collision = true;
      std::pair<glm::i32vec2, entt::entity> results;
      std::get<0>(results) = glm::i32vec2(glm::round(closestOrigin));
      std::get<1>(results) = entity;
      intersectionResults.entities.emplace_back(results);
    }
  }
}

void plugin::physics::ProcessTileset(
  pul::physics::Tileset & tileset
, pul::gfx::Image const & image
) {
  tileset = {};
  tileset.tiles.reserve((image.width / 32ul) * (image.height / 32ul));

  // iterate thru every tile
  for (size_t tileY = 0ul; tileY < image.height / 32ul; ++ tileY)
  for (size_t tileX = 0ul; tileX < image.width  / 32ul; ++ tileX) {
    pul::physics::Tile tile;

    auto constexpr gridSize = pul::physics::Tile::gridSize;

    // iterate thru every texel of the tile, stratified by physics tile GridSize
    for (size_t texelY = 0ul; texelY < 32ul; texelY += 32ul/gridSize)
    for (size_t texelX = 0ul; texelX < 32ul; texelX += 32ul/gridSize) {

      size_t const
        imageTexelX = tileX*32ul + texelX
      , imageTexelY = tileY*32ul + texelY
      , gridTexelX = static_cast<size_t>(texelX*(gridSize/32.0f))
      , gridTexelY = static_cast<size_t>(texelY*(gridSize/32.0f))
      ;

      float const alpha =
        image.data[(image.height-imageTexelY-1)*image.width + imageTexelX].a;

      tile.signedDistanceField[gridTexelX][gridTexelY] = alpha;
    }

    tileset.tiles.emplace_back(tile);
  }
}

void plugin::physics::ClearMapGeometry() {
  tilemapLayer = {};
}

void plugin::physics::LoadMapGeometry(
  std::vector<pul::physics::Tileset const *> const & tilesets
, std::vector<std::span<size_t>>             const & mapTileIndices
, std::vector<std::span<glm::u32vec2>>       const & mapTileOrigins
, std::vector<std::span<pul::core::TileOrientation>> const & mapTileOrientations
) {
  plugin::physics::ClearMapGeometry();

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
    auto const & tileOrientations = mapTileOrientations[tilesetIdx];

    PUL_ASSERT(tilesets[tilesetIdx], continue;);

    for (size_t i = 0ul; i < tileIndices.size(); ++ i) {
      auto const & imageTileIdx    = tileIndices[i];
      auto const & tileOrigin      = tileOrigins[i];
      auto const & tileOrientation = tileOrientations[i];

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
      tile.orientation  = tileOrientation;
    }
  }
}

bool plugin::physics::InverseSceneIntersectionRaycast(
  pul::core::SceneBundle &
, pul::physics::IntersectorRay const & ray
, pul::physics::IntersectionResults & intersectionResults
) {
  intersectionResults = {};
  // TODO this is slow and can be optimized by using SDFs
  pul::physics::BresenhamLine(
    ray.beginOrigin, ray.endOrigin
  , [&](int32_t x, int32_t y) {
      if (intersectionResults.collision) { return; }
      auto origin = glm::i32vec2(x, y);
      // -- get physics tile from acceleration structure

      // calculate tile indices, not for the spritesheet but for the tile in
      // the physx map
      size_t tileIdx;
      glm::u32vec2 texelOrigin;
      if (
        !pul::util::CalculateTileIndices(
          tileIdx, texelOrigin, origin
        , ::tilemapLayer.width, ::tilemapLayer.tileInfo.size()
        )
      ) {
        return;
      }

      PUL_ASSERT_CMP(::tilemapLayer.tileInfo.size(), >, tileIdx, return;);
      auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];

      if (::CalculateSdfDistance(tileInfo, texelOrigin) == 0.0f) {
        intersectionResults =
          pul::physics::IntersectionResults {
            true, origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
          };
      }
    }
  );

  if (::showPhysicsQueries) {
    plugin::debug::RenderLine(
      ray.beginOrigin, ray.endOrigin,
      intersectionResults.collision
    ? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.2f, 1.0f, 0.2f, 1.0f)
    );
  }

  return intersectionResults.collision;
}

bool plugin::physics::IntersectionRaycast(
  pul::core::SceneBundle &
, pul::physics::IntersectorRay const & ray
, pul::physics::IntersectionResults & intersectionResults
) {
  intersectionResults = {};
  // TODO this is slow and can be optimized by using SDFs
  pul::physics::BresenhamLine(
    ray.beginOrigin, ray.endOrigin
  , [&](int32_t x, int32_t y) {
      if (intersectionResults.collision) { return; }
      auto origin = glm::i32vec2(x, y);
      // -- get physics tile from acceleration structure

      // calculate tile indices, not for the spritesheet but for the tile in
      // the physx map
      size_t tileIdx;
      glm::u32vec2 texelOrigin;
      if (
        !pul::util::CalculateTileIndices(
          tileIdx, texelOrigin, origin
        , ::tilemapLayer.width, ::tilemapLayer.tileInfo.size()
        )
      ) {
        return;
      }

      PUL_ASSERT_CMP(::tilemapLayer.tileInfo.size(), >, tileIdx, return;);
      auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];

      if (::CalculateSdfDistance(tileInfo, texelOrigin) > 0.0f) {
        intersectionResults =
          pul::physics::IntersectionResults {
            true, origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
          };
      }
    }
  );

  if (::showPhysicsQueries) {
    plugin::debug::RenderLine(
      ray.beginOrigin, ray.endOrigin,
      intersectionResults.collision
    ? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.2f, 1.0f, 0.2f, 1.0f)
    );
  }

  return intersectionResults.collision;
}

pul::physics::TilemapLayer * plugin::physics::TilemapLayer() {
  return &tilemapLayer;
}

bool plugin::physics::IntersectionAabb(
  pul::core::SceneBundle &
, pul::physics::IntersectorAabb const &
, pul::physics::IntersectionResults &
) {
  return false;
}

bool plugin::physics::IntersectionPoint(
  pul::core::SceneBundle &
, pul::physics::IntersectorPoint const & point
, pul::physics::IntersectionResults & intersectionResults
) {
  intersectionResults = {};


  // -- get physics tile from acceleration structure
  size_t tileIdx;
  glm::u32vec2 texelOrigin;
  if (
    !pul::util::CalculateTileIndices(
      tileIdx, texelOrigin, point.origin
    , ::tilemapLayer.width, ::tilemapLayer.tileInfo.size()
    )
  ) {
    // TODO point
    return false;
  }

  PUL_ASSERT_CMP(::tilemapLayer.tileInfo.size(), >, tileIdx, return false;);
  auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];

  if (::CalculateSdfDistance(tileInfo, texelOrigin) > 0.0f) {
    intersectionResults =
      pul::physics::IntersectionResults {
        true, point.origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
      };

    // TODO point
    return true;
  }

  // TODO point
  return false;
}

void plugin::physics::DebugUiDispatch(pul::core::SceneBundle &) {
  ImGui::Begin("Physics");

  pul::imgui::Text("tilemap width {}", ::tilemapLayer.width);
  pul::imgui::Text("tile info size {}", ::tilemapLayer.tileInfo.size());

  ImGui::Checkbox("show physics queries", &::showPhysicsQueries);

  ImGui::End();
}
