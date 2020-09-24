#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-physics/tileset.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

#include <filesystem>
#include <fstream>

namespace {

int32_t constexpr tileDepthMin = -500, tileDepthMax = +500;

size_t mapWidth, mapHeight;

size_t constexpr
  FlippedHorizontalGidFlag = 0x80000000
, FlippedVerticalGidFlag   = 0x40000000
, FlippedDiagonalGidFlag   = 0x20000000
;

void MapSokolInitialize() {
}

struct LayerRenderable {
  // below gets destroyed when no longer used, since the data is only necessary
  // to create the GPU buffers
  std::vector<std::array<float, 2ul>> origins;
  std::vector<std::array<float, 2ul>> uvCoords;

  // kept in order to do CPU tilemap processing
  std::vector<size_t> tileIds;
  std::vector<glm::u32vec2> tileOrigins;

  sg_buffer bufferVertex;
  sg_buffer bufferUvCoords;
  sg_bindings bindings;

  size_t spritesheetPrimaryIdx;

  size_t tileCount;

  int32_t depth; // tileDepthMin .. tileDepthMax

  bool enabled = true;
};

std::vector<LayerRenderable> renderables;

struct MapTileset {
  pulcher::gfx::Spritesheet spritesheet;
  pulcher::physics::Tileset physicsTileset;
  size_t spritesheetStartingGid;
};

std::vector<MapTileset> mapTilesets;
sg_pipeline pipeline;
sg_shader shader;

void MapSokolPushTile(
  std::string const & layer
, uint32_t const x, uint32_t const y
, bool const flipHorizontal
, bool const flipVertical
, bool const flipDiagonal
, size_t const tileId
) {

  if (tileId == 0ul) { return; }

  int32_t depth = 0ul;

  if (layer == "background") {
    depth = 1;
  } else if (layer == "middleground") {
    depth = 0;
  } else if (layer == "foreground") {
    depth = -1;
  }

  // locate spritesheet used and the local tile ID
  size_t spritesheetIdx = -1ul;
  size_t localTileId = 0ul;
  for (size_t idx = 0ul; idx < mapTilesets.size(); ++ idx) {
    if (mapTilesets[idx].spritesheetStartingGid <= tileId) {
      spritesheetIdx = idx;
      localTileId = tileId - mapTilesets[idx].spritesheetStartingGid;
    }
  }

  if (spritesheetIdx == -1ul) {
    spdlog::error("could not find spritesheet index for tileId {}", tileId);
    return;
  }

  // locate appropiate 'renderable'
  LayerRenderable * renderable = nullptr;
  for (auto & r : renderables) {
    if (r.depth == depth && r.spritesheetPrimaryIdx == spritesheetIdx) {
      renderable = &r;
      break;
    }
  }

  if (renderable == nullptr) {
    renderables.emplace_back();
    renderables.back().depth = depth;
    renderables.back().spritesheetPrimaryIdx = spritesheetIdx;
    renderable = &renderables.back();
  }

  auto & spritesheetPrimary =
    ::mapTilesets[renderable->spritesheetPrimaryIdx].spritesheet;

  size_t const
    uvWidth  = spritesheetPrimary.width
  , uvHeight = spritesheetPrimary.height
  , uvTileWidth  = uvWidth / 32ul
  , uvTileHeight = uvHeight / 32ul
  ;

  renderable->tileOrigins.emplace_back(glm::u32vec2(x, y));
  renderable->tileIds.emplace_back(localTileId);

  for (
      auto const & v
    : std::vector<std::array<float, 2>> {
        {0.0f,  0.0f}
      , {1.0f,  1.0f}
      , {1.0f,  0.0f}

      , {0.0f,  0.0f}
      , {0.0f,  1.0f}
      , {1.0f,  1.0f}
      }
  ) {
    renderable->origins.emplace_back();
    renderable->origins.back()[0] = (static_cast<float>(x) + v[0])*32.0f;
    renderable->origins.back()[1] = (static_cast<float>(y) + v[1])*32.0f;

    // -- compute UV coords
    // compose range into 0 .. tileWidth (modulo / division | X / Y)
    // then bring into range 0 .. 1 by dividing by tile width/height
    float const
      uvOffsetX = (localTileId%uvTileWidth) / static_cast<float>(uvTileWidth)
    , uvOffsetY = (localTileId/uvTileWidth+1) / static_cast<float>(uvTileHeight)
    ;
    renderable->uvCoords.emplace_back();

    auto uvCoord = v;

    if (flipHorizontal) {
      uvCoord[0] = 1.0f - uvCoord[0];
    }
    if (flipVertical) {
      uvCoord[1] = 1.0f - uvCoord[1];
    }
    if (flipDiagonal) {
      std::swap(uvCoord[0], uvCoord[1]);
    }

    renderable->uvCoords.back()[0] =
      uvOffsetX + uvCoord[0]/static_cast<float>(uvWidth) * 32.0f;

    renderable->uvCoords.back()[1] =
      1.0f - uvOffsetY + (1.0f-uvCoord[1])/static_cast<float>(uvHeight) * 32.0f;
  }
}

void MapSokolEnd() {

  for (auto & renderable : renderables) {
    { // -- vertex origin buffer
      sg_buffer_desc desc = {};
      desc.size = renderable.origins.size() * sizeof(float) * 2;
      desc.usage = SG_USAGE_IMMUTABLE;
      desc.content = renderable.origins.data();
      desc.label = "vertex buffer";
      renderable.bufferVertex = sg_make_buffer(&desc);
    }

    { // -- vertex uv coord buffer
      sg_buffer_desc desc = {};
      desc.size = renderable.uvCoords.size() * sizeof(float) * 2;
      desc.usage = SG_USAGE_IMMUTABLE;
      desc.content = renderable.uvCoords.data();
      desc.label = "uv coord buffer";
      renderable.bufferUvCoords = sg_make_buffer(&desc);
    }

    // bindings
    renderable.bindings.vertex_buffers[0] = renderable.bufferVertex;
    renderable.bindings.vertex_buffers[1] = renderable.bufferUvCoords;
    renderable.bindings.fs_images[0] =
      ::mapTilesets[renderable.spritesheetPrimaryIdx].spritesheet.Image();
    renderable.tileCount = renderable.origins.size();

    // dealloc vectors if no longer needed
    renderable.origins = {};
    renderable.uvCoords = {};
  }

  { // -- tilemap shader
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float);
    desc.vs.uniform_blocks[1].uniforms[0].name = "tileDepth";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT;

    desc.vs.uniform_blocks[2].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[2].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[2].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.fs.images[0].name = "baseSampler";
    desc.fs.images[0].type = SG_IMAGETYPE_2D;

    desc.vs.source = PUL_SHADER(
      in layout(location = 0) vec2 inVertexOrigin;
      in layout(location = 1) vec2 inVertexUvCoord;

      out vec2 uvCoord;
      flat out uint tileId;

      uniform vec2 originOffset;
      uniform float tileDepth;
      uniform vec2 framebufferResolution;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inVertexOrigin)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin, tileDepth, 1.0f);
        uvCoord = inVertexUvCoord;
        tileId = uint(gl_VertexID/6);
      }
    );

    desc.fs.source = PUL_SHADER(
      uniform sampler2D baseSampler;
      in vec2 uvCoord;
      flat in uint tileId;
      out vec4 outColor;
      void main() {
        outColor = texture(baseSampler, uvCoord);
        if (outColor.a < 0.01f) { discard; }
      }
    );

    shader = sg_make_shader(&desc);
  }

  { // -- tilemap pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[0].step_rate = 6u;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;

    desc.layout.buffers[1].stride = 0u;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[1].step_rate = 1u;
    desc.layout.attrs[1].buffer_index = 1;
    desc.layout.attrs[1].offset = 0;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.index_type = SG_INDEXTYPE_NONE;

    /* desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; */
    desc.shader = shader;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    desc.rasterizer.cull_mode = SG_CULLMODE_BACK;
    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "map pipeline";

    pipeline = sg_make_pipeline(&desc);
  }
}

} // -- namespace

extern "C" {

void Load(
  pulcher::plugin::Info const & plugins
, char const * filename
) {
  spdlog::info("Loading '{}'", filename);

  cJSON * map;
  {
    // load file
    auto file = std::ifstream{filename};
    if (file.eof() || !file.good()) {
      spdlog::error("could not load map");
      return;
    }

    auto str =
      std::string {
        std::istreambuf_iterator<char>(file)
      , std::istreambuf_iterator<char>()
      };

    map = cJSON_Parse(str.c_str());

    if (map == nullptr) {
      spdlog::critical(
        " -- failed to parse json for map; '{}'", cJSON_GetErrorPtr()
      );
      return;
    }
  }

  ::mapWidth  = cJSON_GetObjectItemCaseSensitive(map, "width")->valueint;
  ::mapHeight = cJSON_GetObjectItemCaseSensitive(map, "height")->valueint;

  spdlog::info(" -- dimensions {}x{}", ::mapWidth, ::mapHeight);

  ::MapSokolInitialize();

  cJSON * tileset;
  cJSON_ArrayForEach(tileset, cJSON_GetObjectItemCaseSensitive(map, "tilesets"))
  {
    auto tilesetFilename =
      cJSON_GetObjectItemCaseSensitive(tileset, "source")->valuestring;

    spdlog::debug("loading tileset '{}'", tilesetFilename);

    cJSON * tilesetJson;
    { // load file
      std::filesystem::path tilesetJsonPath =
          std::filesystem::path(filename).remove_filename() / tilesetFilename
      ;

      // load file
      auto file = std::ifstream{tilesetJsonPath.string()};
      if (file.eof() || !file.good()) {
        spdlog::error("could not load tilest '{}'", tilesetJsonPath.string());
        return;
      }

      auto str =
        std::string {
          std::istreambuf_iterator<char>(file)
        , std::istreambuf_iterator<char>()
        };

      tilesetJson = cJSON_Parse(str.c_str());
    }

    std::filesystem::path tilesetPath =
        std::filesystem::path(filename).remove_filename()
      / std::filesystem::path(
          cJSON_GetObjectItemCaseSensitive(tilesetJson, "image")->valuestring
        );
    ;

    if (!std::filesystem::exists(tilesetPath)) {
      spdlog::error(" -- invalid path for tileset");
      continue;
    }

    { // construct map tileset
      auto image = pulcher::gfx::Image::Construct(tilesetPath.string().c_str());
      ::mapTilesets
        .emplace_back(
          pulcher::gfx::Spritesheet::Construct(image)
        , plugins.physics.ProcessTileset(image)
        , cJSON_GetObjectItemCaseSensitive(tileset, "firstgid")->valueint
        );
    }
  }

  cJSON * layer;
  cJSON_ArrayForEach(layer, cJSON_GetObjectItemCaseSensitive(map, "layers")) {
    auto layerLabel =
      cJSON_GetObjectItemCaseSensitive(layer, "name")->valuestring;

    spdlog::debug("parsing layer '{}'", layerLabel);

    cJSON * chunk;
    cJSON_ArrayForEach(chunk, cJSON_GetObjectItemCaseSensitive(layer, "chunks"))
    {
      auto width =
        static_cast<uint32_t>(
          cJSON_GetObjectItemCaseSensitive(chunk, "width")->valueint
        );

      auto x = cJSON_GetObjectItemCaseSensitive(chunk, "x")->valueint;
      auto y = cJSON_GetObjectItemCaseSensitive(chunk, "y")->valueint;

      size_t localItr = 0;

      cJSON * gidJson;

      cJSON_ArrayForEach(
        gidJson
      , cJSON_GetObjectItemCaseSensitive(chunk, "data")
      ) {
        auto gid = gidJson->valueint;
        auto tileId =
            gid
          & ~(
              FlippedHorizontalGidFlag
            | FlippedVerticalGidFlag
            | FlippedDiagonalGidFlag
            )
        ;

        bool const
          flipHorizontal = gid & ::FlippedHorizontalGidFlag
        , flipVertical   = gid & ::FlippedVerticalGidFlag
        , flipDiagonal   = gid & ::FlippedDiagonalGidFlag
        ;

        size_t localX = localItr % width;
        size_t localY = localItr / width;

        ::MapSokolPushTile(
          layerLabel
        , static_cast<uint32_t>(x + localX)
        , static_cast<uint32_t>(y + localY)
        , flipHorizontal, flipVertical, flipDiagonal
        , tileId
        );

        ++ localItr;
      }
    }
  }

  ::MapSokolEnd();

  { // create physics geometry for map

    std::vector<pulcher::physics::Tileset const *> tilesets;
    std::vector<std::span<size_t>> mapTileIndices;
    std::vector<std::span<glm::u32vec2>> mapTileOrigins;

    spdlog::debug("renderable count {}", ::renderables.size());

    for (auto & renderable : ::renderables) {
      // only depth 0 layers can have collision
      if (renderable.depth != 0) { continue; }

      tilesets
        .emplace_back(
          &::mapTilesets[renderable.spritesheetPrimaryIdx].physicsTileset
        );

      mapTileIndices.emplace_back(std::span(renderable.tileIds));
      mapTileOrigins.emplace_back(std::span(renderable.tileOrigins));
    }

    plugins.physics.LoadMapGeometry(tilesets, mapTileIndices, mapTileOrigins);
  }

  cJSON_Delete(map);
}

void Render(pulcher::core::SceneBundle & scene) {
  sg_apply_pipeline(pipeline);

  glm::vec2 cameraOrigin = scene.cameraOrigin;

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 0
  , &cameraOrigin.x
  , sizeof(float) * 2ul
  );

  std::array<float, 2> windowResolution {{
    static_cast<float>(pulcher::gfx::DisplayWidth())
  , static_cast<float>(pulcher::gfx::DisplayHeight())
  }};

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 2
  , windowResolution.data()
  , sizeof(float) * 2ul
  );

  for (auto & renderable : ::renderables) {
    if (!renderable.enabled) { continue; }
    sg_apply_bindings(&renderable.bindings);

    float mixedDepth =
        (renderable.depth - tileDepthMin)
      / static_cast<float>(tileDepthMax - tileDepthMin)
    ;
    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , &mixedDepth
    , sizeof(float)
    );

    sg_draw(0, renderable.tileCount, 1);
  }
}

void UiRender(pulcher::core::SceneBundle & scene) {
  ImGui::Begin("Map Info");

  ImGui::DragInt2("map origin", &scene.cameraOrigin.x);

  static size_t tileInfoTilesetIdx = -1ul;

  ImGui::Separator();
  ImGui::Text("total tilemap sets: '%lu'", ::mapTilesets.size());

  for (size_t i = 0ul; i < ::mapTilesets.size(); ++ i) {
    auto const & tileset = ::mapTilesets[i];
    auto & spritesheet = tileset.spritesheet;
    ImGui::Text("filename: '%s'", tileset.spritesheet.filename.c_str());
    ImGui::Text(
      "tile width %lu height %lu total %lu"
    , tileset.spritesheet.width / 32ul
    , tileset.spritesheet.height / 32ul
    , tileset.spritesheet.height / 32ul * tileset.spritesheet.width / 32ul
    );
    ImGui::Text("idx: %lu", i);

    auto const imMax = ImGui::GetWindowContentRegionMax();
    auto const imMin = ImGui::GetWindowContentRegionMin();
    auto bounds = ImVec2(imMax.x - imMin.x, imMax.y - imMin.y);
    bounds.x = glm::min(static_cast<size_t>(bounds.x), spritesheet.width);
    bounds.y =
      bounds.x * (spritesheet.height / static_cast<float>(spritesheet.width));

    ImGui::Image(
      reinterpret_cast<void *>(spritesheet.Image().id)
    , bounds, ImVec2(0, 1), ImVec2(1, 0)
    );

    if (ImGui::IsItemClicked()) {
      tileInfoTilesetIdx = i;
      /* point.origin.y / 32ul * ::tilemapLayer.width + point.origin.x / 32ul */
    }
  }

  if (tileInfoTilesetIdx != -1ul) {
    ImGui::End();

    bool open = true;
    ImGui::Begin("Tile Info", &open);

    if (!open) {
      tileInfoTilesetIdx = -1ul;
    }

    ImGui::Text("tileset clicked %lu", tileInfoTilesetIdx);

    ImGui::End();
    ImGui::Begin("Map Info");
  }

  ImGui::Separator();
  ImGui::Separator();

  ImGui::Text("map renderables: %lu", ::renderables.size());
  for (auto & renderable : ::renderables) {
    ImGui::PushID(&renderable);
    ImGui::Text("draw call: %lu", renderable.tileCount);
    ImGui::Text("depth: %d", renderable.depth);
    ImGui::Text("spritesheet: %lu", renderable.spritesheetPrimaryIdx);
    ImGui::Checkbox("enabled", &renderable.enabled);

    ImGui::Separator();
    ImGui::PopID();
  }

  ImGui::End();
}

void Shutdown() {
  spdlog::info("destroying map");

  for (auto & renderable : ::renderables) {
    sg_destroy_buffer(renderable.bufferVertex);
    sg_destroy_buffer(renderable.bufferUvCoords);
  }

  ::renderables = {};

  sg_destroy_pipeline(::pipeline);
  sg_destroy_shader(::shader);

  ::pipeline = {};
  ::shader = {};
  ::mapTilesets.clear();
}

} // extern C
