#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/spritesheet.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>

#include <filesystem>
#include <fstream>

namespace {

int32_t constexpr tileDepthMin = -500, tileDepthMax = +500;

std::array<float, 2ul> mapOrigin = {{ 0.0f, 0.0f }};
size_t mapWidth, mapHeight;

size_t constexpr
  FlippedHorizontalGidFlag = 0x80000000
, FlippedVerticalGidFlag   = 0x40000000
, FlippedDiagonalGidFlag   = 0x20000000
;

void MapSokolInitialize() {
}

struct LayerRenderable {
  std::vector<std::array<float, 2ul>> origins;
  std::vector<std::array<float, 2ul>> uvCoords;

  sg_buffer bufferVertex;
  sg_buffer bufferUvCoords;
  sg_bindings bindings;

  size_t spritesheetPrimaryIdx;

  size_t drawCallCount;

  int32_t depth; // tileDepthMin .. tileDepthMax

  bool enabled = true;
};

std::vector<LayerRenderable> renderables;

std::vector<pulcher::gfx::Spritesheet> spritesheets;
std::vector<size_t> spritesheetsStartingGids;
sg_pipeline pipeline;
sg_shader shader;

void MapSokolPushTile(
  std::string const & layer
, uint32_t x, uint32_t y
, [[maybe_unused]] bool flipHorizontal
, [[maybe_unused]] bool flipVertical
, [[maybe_unused]] bool flipDiagonal
, size_t tileId
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
  for (size_t idx = 0ul; idx < spritesheetsStartingGids.size(); ++ idx) {
    if (spritesheetsStartingGids[idx] <= tileId) {
      spritesheetIdx = idx;
      localTileId = tileId - spritesheetsStartingGids[idx];
      break;
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

  auto & spritesheetPrimary = ::spritesheets[renderable->spritesheetPrimaryIdx];

  size_t const
    uvWidth  = spritesheetPrimary.width
  , uvHeight = spritesheetPrimary.height
  , uvTileWidth  = uvWidth / 32ul
  , uvTileHeight = uvHeight / 32ul
  ;

  /* spdlog::info( */
  /*   "UV {}x{} localTileId {} tileWH {}x{} local TID {}x{} UV COORD {}x{}" */
  /* , uvWidth, uvHeight */
  /* , localTileId */
  /* , uvTileWidth, uvTileHeight */
  /* , (localTileId % uvTileWidth) */
  /* , (localTileId / uvTileWidth) */
  /* , (localTileId % uvTileWidth) / static_cast<float>(uvTileWidth) */
  /* , (localTileId / uvTileWidth) / static_cast<float>(uvTileHeight) */
  /* ); */

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
      uvOffsetX = (localTileId % uvTileWidth) / static_cast<float>(uvTileWidth)
    , uvOffsetY = (localTileId / uvTileWidth) / static_cast<float>(uvTileHeight)
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


    // TODO grab orientations
    /* layerBackgroundTileVertexOrientation.emplace_back(); */
    /* orientation = flipHorizontal ? 1.f : 0.f; */
    /* orientation = flipVertical ? 1.f : 0.f; */
    /* orientation = flipDiagonal ? 1.f : 0.f; */
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
      ::spritesheets[renderable.spritesheetPrimaryIdx].Image();
    renderable.drawCallCount = renderable.origins.size();

    // dealloc vectors if no longer needed
    renderable.origins = {};
    renderable.uvCoords = {};
  }

  { // -- shared shader
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

    desc.vs.source =
      "#version 330 core\n"
      "uniform vec2 originOffset;\n"
      "uniform float tileDepth;\n"
      "uniform vec2 framebufferResolution;\n"
      "in layout(location = 0) vec2 inVertexOrigin;\n"
      "in layout(location = 1) vec2 inVertexUvCoord;\n"
      /* "in layout(location = 2) vec3 inOrientation;\n" */
      "out vec2 uvCoord;\n"
      "flat out uint tileId;\n"
      "void main() {\n"
      "  vec2 framebufferScale = vec2(2.0f) / framebufferResolution;\n"
      "  framebufferScale.y *= 800.0f / 600.0f;"
      "  vec2 vertexOrigin = inVertexOrigin*vec2(1,-1) * framebufferScale;\n"
      "  vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;\n"
      "  gl_Position = vec4(vertexOrigin, tileDepth, 1.0f);\n"
      "  uvCoord = inVertexUvCoord;\n"
      "  tileId = uint(gl_VertexID/6);\n"
      "}\n"
    ;

    desc.fs.source =
      "#version 330 core\n"
      "uniform sampler2D baseSampler;\n"
      "in vec2 uvCoord;\n"
      "flat in uint tileId;\n"
      "out vec4 outColor;\n"
      "void main() {\n"
      "  outColor = texture(baseSampler, uvCoord);\n"
      "  if (outColor.a < 0.1f) { discard; }\n"
      "}\n"
    ;

    shader = sg_make_shader(&desc);
  }

  { // -- shared pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[0].step_rate = 1u;
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

void Load(char const * filename) {
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

    spdlog::info("loading tileset '{}'", tilesetFilename);

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

    // construct spritesheet
    ::spritesheets.emplace_back(
      std::move(
        pulcher::gfx::Spritesheet::Construct(tilesetPath.string().c_str())
      )
    );

    // collect starting GID for spritesheet
    ::spritesheetsStartingGids
      .emplace_back(
        cJSON_GetObjectItemCaseSensitive(tileset, "firstgid")->valueint
      );
  }

  cJSON * layer;
  cJSON_ArrayForEach(layer, cJSON_GetObjectItemCaseSensitive(map, "layers")) {
    auto layerLabel =
      cJSON_GetObjectItemCaseSensitive(layer, "name")->valuestring;

    spdlog::info("parsing layer '{}'", layerLabel);

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

  cJSON_Delete(map);
}

void Render() {
  sg_apply_pipeline(pipeline);

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 0
  , ::mapOrigin.data()
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

    sg_draw(0, renderable.drawCallCount, 1);
  }
}

void UiRender() {
  ImGui::Begin("Map Info");

  ImGui::DragFloat2("map origin", &::mapOrigin[0]);

  ImGui::Separator();
  ImGui::Text("total spritesheets: '%lu'", ::spritesheets.size());
  for (auto const & spritesheet : ::spritesheets) {
    ImGui::Text("filename: '%s'", spritesheet.filename.c_str());
  }

  ImGui::Separator();
  ImGui::Separator();

  ImGui::Text("map renderables: %lu", ::renderables.size());
  for (auto & renderable : ::renderables) {
    ImGui::PushID(renderable.depth);
    ImGui::Text("draw call: %lu", renderable.drawCallCount);
    ImGui::Text("depth: %d", renderable.depth);
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

  sg_destroy_pipeline(pipeline);
  sg_destroy_shader(shader);

  pipeline = {};
  shader = {};
  spritesheets.clear();
}

} // extern C
