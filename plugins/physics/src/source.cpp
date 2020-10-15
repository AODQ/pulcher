#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-physics/tileset.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>
#include <pulcher-util/math.hpp>

#include <glad/glad.hpp>
#include <imgui/imgui.hpp>

#include <span>
#include <vector>

namespace {

struct DebugRenderInfo {
  sg_buffer bufferOrigin;
  sg_buffer bufferCollision;
  sg_bindings bindings;
  sg_pipeline pipeline;
  sg_shader program;
};


constexpr size_t debugRenderMaxPoints = 1'000;
constexpr size_t debugRenderMaxRays = 1'000;
DebugRenderInfo debugRenderPoint = {};
DebugRenderInfo debugRenderRay = {};

void LoadSokolInfoRay() {
  { // -- origin buffer
    sg_buffer_desc desc = {};
    desc.size = debugRenderMaxRays * sizeof(float) * 4;
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "debug-render-info-ray-origin-buffer";
    debugRenderRay.bufferOrigin = sg_make_buffer(&desc);
  }

  { // -- collision buffer
    sg_buffer_desc desc = {};
    desc.size = debugRenderMaxRays * sizeof(float);
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "debug-render-info-ray-collision-buffer";
    debugRenderRay.bufferCollision = sg_make_buffer(&desc);
  }

  // bindings
  debugRenderRay.bindings.vertex_buffers[0] = debugRenderRay.bufferOrigin;
  debugRenderRay.bindings.vertex_buffers[1] = debugRenderRay.bufferCollision;

  { // -- shader
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec2 inOrigin;
      layout(location = 1) in float inCollision;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;

      flat out int inoutCollision;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin.xy, 0.0f, 1.0f);
        inoutCollision = int(inCollision > 0.0f);
      }
    );

    desc.fs.source = PUL_SHADER(
      flat in int inoutCollision;

      out vec4 outColor;

      void main() {
        outColor =
            inoutCollision > 0
          ? vec4(1.0f, 0.7f, 0.7f, 1.0f) : vec4(0.7f, 1.0f, 0.7f, 1.0f)
        ;
      }
    );

    debugRenderRay.program = sg_make_shader(&desc);
  }

  { // -- pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;

    desc.layout.buffers[1].stride = 0u;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[1].buffer_index = 1;
    desc.layout.attrs[1].offset = 0;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT;

    desc.primitive_type = SG_PRIMITIVETYPE_LINES;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = debugRenderRay.program;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "debug-render-ray-pipeline";

    debugRenderRay.pipeline = sg_make_pipeline(&desc);
  }
}

void LoadSokolInfoPoint() {
  { // -- origin buffer
    sg_buffer_desc desc = {};
    desc.size = debugRenderMaxPoints * sizeof(float) * 2;
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "debug-render-info-point-origin-buffer";
    debugRenderPoint.bufferOrigin = sg_make_buffer(&desc);
  }

  { // -- collision buffer
    sg_buffer_desc desc = {};
    desc.size = debugRenderMaxPoints * sizeof(float);
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "debug-render-info-point-collision-buffer";
    debugRenderPoint.bufferCollision = sg_make_buffer(&desc);
  }

  // bindings
  debugRenderPoint.bindings.vertex_buffers[0] = debugRenderPoint.bufferOrigin;
  debugRenderPoint.bindings.vertex_buffers[1] =
    debugRenderPoint.bufferCollision;

  { // -- shader
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec2 inOrigin;
      layout(location = 1) in float inCollision;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;

      flat out int inoutCollision;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin.xy, 0.0f, 1.0f);
        inoutCollision = int(inCollision > 0.0f);
      }
    );

    desc.fs.source = PUL_SHADER(
      flat in int inoutCollision;

      out vec4 outColor;

      void main() {
        outColor =
            inoutCollision > 0
          ? vec4(1.0f, 0.7f, 0.7f, 1.0f) : vec4(0.7f, 1.0f, 0.7f, 1.0f)
        ;
      }
    );

    debugRenderPoint.program = sg_make_shader(&desc);
  }

  { // -- pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;

    desc.layout.buffers[1].stride = 0u;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.attrs[1].buffer_index = 1;
    desc.layout.attrs[1].offset = 0;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT;

    desc.primitive_type = SG_PRIMITIVETYPE_POINTS;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = debugRenderPoint.program;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    desc.rasterizer.cull_mode = SG_CULLMODE_BACK;
    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "debug-render-point-pipeline";

    debugRenderPoint.pipeline = sg_make_pipeline(&desc);
  }
}

void LoadSokolInfo() {
  ::LoadSokolInfoPoint();
  ::LoadSokolInfoRay();
}

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

// -- plugin functions
extern "C" {

PUL_PLUGIN_DECL void Physics_ProcessTileset(
  pulcher::physics::Tileset & tileset
, pulcher::gfx::Image const & image
) {
  tileset = {};
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

      float const alpha =
        image.data[(image.height-imageTexelY-1)*image.width + imageTexelX].a;

      tile.signedDistanceField[gridTexelX][gridTexelY] = alpha;
    }

    tileset.tiles.emplace_back(tile);
  }
}

PUL_PLUGIN_DECL void Physics_ClearMapGeometry() {
  sg_destroy_buffer(::debugRenderPoint.bufferOrigin);
  sg_destroy_buffer(::debugRenderRay  .bufferOrigin);

  sg_destroy_buffer(::debugRenderPoint.bufferCollision);
  sg_destroy_buffer(::debugRenderRay  .bufferCollision);

  sg_destroy_pipeline(::debugRenderPoint.pipeline);
  sg_destroy_pipeline(::debugRenderRay  .pipeline);

  ::debugRenderPoint.bindings = {};
  ::debugRenderRay  .bindings = {};

  sg_destroy_shader(::debugRenderPoint.program);
  sg_destroy_shader(::debugRenderRay  .program);

  tilemapLayer = {};
}

PUL_PLUGIN_DECL void Physics_LoadMapGeometry(
  std::vector<pulcher::physics::Tileset const *> const & tilesets
, std::vector<std::span<size_t>>                 const & mapTileIndices
, std::vector<std::span<glm::u32vec2>>           const & mapTileOrigins
) {
  Physics_ClearMapGeometry();

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

  ::LoadSokolInfo();
}

PUL_PLUGIN_DECL bool Physics_IntersectionRaycast(
  pulcher::core::SceneBundle const &
, pulcher::physics::IntersectorRay const & ray
, pulcher::physics::IntersectionResults & intersectionResults
) {
  intersectionResults = {};
  // TODO this is slow and can be optimized by using SDFs
  pulcher::physics::BresenhamLine(
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
        !pulcher::util::CalculateTileIndices(
          tileIdx, texelOrigin, origin
        , ::tilemapLayer.width, ::tilemapLayer.tileInfo.size()
        )
      ) {
        return;
      }

      auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];
      auto const * tileset = ::tilemapLayer.tilesets[tileInfo.tilesetIdx];

      if (!tileInfo.Valid()) { return; }

      pulcher::physics::Tile const & physicsTile =
        tileset->tiles[tileInfo.imageTileIdx];

      // -- compute intersection SDF and accel hints
      if (
        physicsTile.signedDistanceField[texelOrigin.x][texelOrigin.y] > 0.0f
      ) {
        intersectionResults =
          pulcher::physics::IntersectionResults {
            true, origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
          };
      }
    }
  );

  return intersectionResults.collision;
}

PUL_PLUGIN_DECL bool Physics_IntersectionPoint(
  pulcher::core::SceneBundle const &
, pulcher::physics::IntersectorPoint const & point
, pulcher::physics::IntersectionResults & intersectionResults
) {
  intersectionResults = {};

  // -- get physics tile from acceleration structure
  size_t tileIdx;
  glm::u32vec2 texelOrigin;
  if (
    !pulcher::util::CalculateTileIndices(
      tileIdx, texelOrigin, point.origin
    , ::tilemapLayer.width, ::tilemapLayer.tileInfo.size()
    )
  ) {
    return false;
  }

  auto const & tileInfo = ::tilemapLayer.tileInfo[tileIdx];
  auto const * tileset = ::tilemapLayer.tilesets[tileInfo.tilesetIdx];

  if (!tileInfo.Valid()) {
    return false;
  }

  pulcher::physics::Tile const & physicsTile =
    tileset->tiles[tileInfo.imageTileIdx];

  // -- compute intersection SDF and accel hints
  if (physicsTile.signedDistanceField[texelOrigin.x][texelOrigin.y] > 0.0f) {
    intersectionResults =
      pulcher::physics::IntersectionResults {
        true, point.origin, tileInfo.imageTileIdx, tileInfo.tilesetIdx
      };

    return true;
  }

  return false;
}

PUL_PLUGIN_DECL void Physics_UiRender(pulcher::core::SceneBundle & scene) {
  ImGui::Begin("Physics");

  pul::imgui::Text("tilemap width {}", ::tilemapLayer.width);
  pul::imgui::Text("tile info size {}", ::tilemapLayer.tileInfo.size());

  auto & queries = scene.PhysicsQueries();

  static bool showPhysicsQueries = false;
  ImGui::Checkbox("show physics queries", &showPhysicsQueries);
  if (showPhysicsQueries && queries.intersectorResultsPoints.size() > 0ul) {
    { // -- update buffers
      std::vector<glm::vec2> points;
      std::vector<float> collisions;
      points.reserve(queries.intersectorResultsPoints.size());
      // update queries
      for (auto & query : queries.intersectorResultsPoints) {
        points.emplace_back(query.origin);
        collisions.emplace_back(static_cast<float>(query.collision));
      }

      sg_update_buffer(
        debugRenderPoint.bufferOrigin
      , points.data(), points.size() * sizeof(glm::vec2)
      );

      sg_update_buffer(
        debugRenderPoint.bufferCollision
      , collisions.data(), collisions.size() * sizeof(float)
      );
    }

    // apply pipeline and render
    sg_apply_pipeline(debugRenderPoint.pipeline);
    sg_apply_bindings(&debugRenderPoint.bindings);
    glm::vec2 cameraOrigin = scene.cameraOrigin;

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 0
    , &cameraOrigin.x
    , sizeof(float) * 2ul
    );

    std::array<float, 2> windowResolution {{
      static_cast<float>(scene.config.framebufferWidth)
    , static_cast<float>(scene.config.framebufferHeight)
    }};

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , windowResolution.data()
    , sizeof(float) * 2ul
    );

    glPointSize(2);
    sg_draw(0, queries.intersectorResultsPoints.size(), 1);
  }

  if (showPhysicsQueries && queries.intersectorResultsRays.size() > 0ul) {
    { // -- update buffers
      std::vector<glm::vec2> lines;
      std::vector<float> collisions;
      lines.reserve(queries.intersectorResultsRays.size()*2);
      // update queries
      for (auto & query : queries.intersectorResultsRays) {
        lines.emplace_back(query.origin);
        collisions.emplace_back(static_cast<float>(query.collision));
        collisions.emplace_back(static_cast<float>(query.collision));
      }

      sg_update_buffer(
        debugRenderRay.bufferOrigin
      , lines.data(), lines.size() * sizeof(glm::vec2)
      );

      sg_update_buffer(
        debugRenderRay.bufferCollision
      , collisions.data(), collisions.size() * sizeof(float)
      );
    }

    // apply pipeline and render
    sg_apply_pipeline(debugRenderRay.pipeline);
    sg_apply_bindings(&debugRenderRay.bindings);
    glm::vec2 cameraOrigin = scene.cameraOrigin;

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 0
    , &cameraOrigin.x
    , sizeof(float) * 2ul
    );

    std::array<float, 2> windowResolution {{
      static_cast<float>(scene.config.framebufferWidth)
    , static_cast<float>(scene.config.framebufferHeight)
    }};

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , windowResolution.data()
    , sizeof(float) * 2ul
    );

    glLineWidth(1.0f);

    sg_draw(0, queries.intersectorResultsRays.size()*2, 1);
  }

  ImGui::End();
}

}
