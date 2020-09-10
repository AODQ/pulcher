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

uint32_t width, height;

size_t constexpr
  FlippedHorizontalGidFlag = 0x80000000
, FlippedVerticalGidFlag   = 0x40000000
, FlippedDiagonalGidFlag   = 0x20000000
;

void MapSokolInitialize() {
}

/* void MapSokolPushLayer( */
/*   std::string label */
/* , uint32_t width */
/* , uint32_t height */
/* , int32_t startx */
/* , int32_t starty */
/* , int32_t x */
/* , int32_t y */
/* ) { */

/*   if (label == */
/* } */

std::vector<std::array<float, 2>>
  layerBackgroundTileVertexOrigins
, layerBackgroundTileVertexUvCoords
;

size_t drawCalls;
std::vector<pulcher::gfx::Spritesheet> spritesheets;
sg_buffer bufferVertex;
sg_bindings bindings;
sg_pipeline pipeline;
sg_shader shader;

std::vector<std::array<float, 3>>
  layerBackgroundTileVertexOrientation // <flipX, flipY, flipDiagonal>
;

void MapSokolPushTile(
  std::string const & layer
, uint32_t x, uint32_t y
, bool flipHorizontal, bool flipVertical, bool flipDiagonal
, size_t /*tileId*/
) {
  if (layer == "background") {
    for (
        auto & v
      : std::vector<std::array<float, 2>> {
          {0.0f,  0.0f}
        , {1.0f,  1.0f}
        , {1.0f,  0.0f}

        , {0.0f,  0.0f}
        , {0.0f,  1.0f}
        , {1.0f,  1.0f}
        }
    ) {
      layerBackgroundTileVertexOrigins.emplace_back();
      layerBackgroundTileVertexOrigins.back()[0] =
        static_cast<float>(x) + v[0]*32.0f;
      layerBackgroundTileVertexOrigins.back()[1] =
        static_cast<float>(y) + v[1]*32.0f;

      // TODO need to grab actual UV coords
      /* float uvOffsetX = (tileId % 16) * 32.0f / 640.0f; */
      /* float uvOffsetY = (tileId % 16) * 32.0f / 640.0f; */
      layerBackgroundTileVertexUvCoords.emplace_back();
      layerBackgroundTileVertexUvCoords.back()[0] = v[0];
      layerBackgroundTileVertexUvCoords.back()[1] = v[1];
    }

    layerBackgroundTileVertexOrientation.emplace_back();
    layerBackgroundTileVertexOrientation.back()[0] = flipHorizontal ? 1.f : 0.f;
    layerBackgroundTileVertexOrientation.back()[1] = flipVertical ? 1.f : 0.f;
    layerBackgroundTileVertexOrientation.back()[2] = flipDiagonal ? 1.f : 0.f;
  }
}

void MapSokolEnd() {

  {
    sg_buffer_desc desc = {};
    desc.size = layerBackgroundTileVertexOrigins.size() * sizeof(float) * 2;
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.content = layerBackgroundTileVertexOrigins[0].data();
    desc.label = "map-background-vertex-buffer";
    bufferVertex = sg_make_buffer(&desc);
  }

  drawCalls = layerBackgroundTileVertexOrigins.size();

  layerBackgroundTileVertexOrigins = {};

  bindings.vertex_buffers[0] = bufferVertex;

  {
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
    /* desc.fs.images[0].name = "baseSampler"; */
    /* desc.fs.images[0].type = SG_IMAGETYPE_2D; */

    desc.vs.source =
      "#version 330\n"
      "uniform vec2 originOffset;\n"
      "in layout(location = 0) vec2 inVertexOrigin;\n"
      "in layout(location = 1) vec2 inVertexUvCoord;\n"
      "out vec2 uvCoord;\n"
      "void main() {\n"
      "  gl_Position = vec4(inVertexOrigin, 0.5f, 1.0f);\n"
      "  uvCoord = inVertexUvCoord;\n"
      "}\n"
    ;

    desc.fs.source =
      "#version 330\n"
      /* "uniform sampler2D baseSampler;\n" */
      "in vec2 uvCoord;\n"
      "out vec4 outColor;\n"
      "void main() {\n"
      /* "  outColor = texture(baseSampler, uvCoord);\n" */
      "  outColor = vec4(uvCoord, 0.5f, 1.0f);\n"
      "}\n"
    ;

    shader = sg_make_shader(&desc);
  }

  {
    sg_pipeline_desc desc = {};
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    desc.shader = shader;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;
    desc.rasterizer.cull_mode = SG_CULLMODE_BACK;

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

  ::width  = cJSON_GetObjectItemCaseSensitive(map, "width")->valueint;
  ::height = cJSON_GetObjectItemCaseSensitive(map, "height")->valueint;

  spdlog::info(" -- dimensions {}x{}", ::width, ::height);

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

    ::spritesheets.emplace_back(
      std::move(
        pulcher::gfx::Spritesheet::Construct(tilesetPath.string().c_str())
      )
    );
  }

  cJSON * layer;
  cJSON_ArrayForEach(layer, cJSON_GetObjectItemCaseSensitive(map, "layers")) {
    auto layerLabel =
      cJSON_GetObjectItemCaseSensitive(layer, "name")->valuestring;

    spdlog::info("parsing layer '{}'", layerLabel);

    /* ::MapSokolPushLayer( */
    /*   layerLabel */
    /* , layer["width"].get<uint32_t>() */
    /* , layer["height"].get<uint32_t>() */
    /* , layer["startx"].get<int32_t>() */
    /* , layer["starty"].get<int32_t>() */
    /* , layer["x"].get<int32_t>() */
    /* , layer["y"].get<int32_t>() */
    /* ); */

    // only parse renderable parts for now
    /* if (layer.find("chunks") == layer.end()) { continue; } */

    cJSON * chunk;
    cJSON_ArrayForEach(chunk, cJSON_GetObjectItemCaseSensitive(layer, "chunks"))
    {
      auto width =
        static_cast<uint32_t>(
          cJSON_GetObjectItemCaseSensitive(chunk, "width")->valueint
        );

      auto height =
        static_cast<uint32_t>(
          cJSON_GetObjectItemCaseSensitive(chunk, "height")->valueint
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
        , static_cast<uint32_t>(x + 32ul*localX)
        , static_cast<uint32_t>(y + 32ul*localY)
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
  sg_apply_bindings(&bindings);
  sg_draw(0, drawCalls, 1);
}

void UiRender() {
  ImGui::Begin("Map Info");

  ImGui::Text("total spritesheet");
  ImGui::Text("total spritesheets: '%lu'", spritesheets.size());
  for (auto const & spritesheet : spritesheets) {
    ImGui::Text("filename foo: '%s'", spritesheet.filename.c_str());
  }

  ImGui::End();
}

void Shutdown() {
  spdlog::info("destroying map");
  sg_destroy_buffer(bufferVertex);
  sg_destroy_pipeline(pipeline);
  sg_destroy_shader(shader);

  bindings = {};
  bufferVertex = {};
  pipeline = {};
  shader = {};
  spritesheets.clear();
}

} // extern C
