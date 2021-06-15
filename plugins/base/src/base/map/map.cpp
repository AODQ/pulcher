#include <plugin-base/map/map.hpp>

#include <plugin-base/animation/animation.hpp>
#include <plugin-base/bot/bot.hpp>
#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-core/enum.hpp>
#include <pulcher-core/map.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/pickup.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-physics/tileset.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>
#include <pulcher-util/math.hpp>

#include <cjson/cJSON.h>
#include <entt/entt.hpp>
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
  std::vector<pul::core::TileOrientation> tileOrientations;

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
  pul::gfx::Spritesheet spritesheet;
  pul::physics::Tileset physicsTileset;
  cJSON * jsonTiles;
  size_t spritesheetStartingGid;
};

std::vector<MapTileset> mapTilesets;
sg_pipeline pipeline;
sg_shader shader;

bool GetMapTileset(
  size_t const tileId, size_t & outSpritesheetIdx, size_t & outLocalTileId
) {
  outSpritesheetIdx = -1ul;
  outLocalTileId = 0ul;

  for (size_t idx = 0ul; idx < mapTilesets.size(); ++ idx) {
    if (mapTilesets[idx].spritesheetStartingGid <= tileId) {
      outSpritesheetIdx = idx;
      outLocalTileId = tileId - mapTilesets[idx].spritesheetStartingGid;
    }
  }

  if (outSpritesheetIdx == -1ul) {
    spdlog::error("could not find spritesheet index for tileId {}", tileId);
    return false;
  }

  return true;
}

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

  // TODO this should not parse numbers but just assume depth based on layer

  if (layer.compare(0, 13, "nonsolid-back") == 0) {
    size_t numberIdx = layer.find_last_not_of("0123456789");
    depth = +std::stoi(layer.substr(numberIdx+1));
  } else if (layer.compare(0, 14, "nonsolid-front") == 0) {
    size_t numberIdx = layer.find_last_not_of("0123456789");
    depth = -std::stoi(layer.substr(numberIdx+1));
  } else if (layer.compare(0, 9, "solid-all") == 0) {
    depth = 0;
  } else if (layer.compare(0, 9, "solid-player") == 0) {
    depth = 0;
  }

  // locate spritesheet used and the local tile ID
  size_t spritesheetIdx = -1ul;
  size_t localTileId = 0ul;
  if (!GetMapTileset(tileId, spritesheetIdx, localTileId)) { return; }

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
  renderable->tileOrientations.emplace_back(
    static_cast<pul::core::TileOrientation>(
      (flipDiagonal   ? Idx(pul::core::TileOrientation::FlipDiagonal)   : 0ul)
    | (flipVertical   ? Idx(pul::core::TileOrientation::FlipVertical)   : 0ul)
    | (flipHorizontal ? Idx(pul::core::TileOrientation::FlipHorizontal) : 0ul)
    )
  );

  for (auto const & v
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
      layout(location = 0) in vec2 inOrigin;
      layout(location = 1) in vec2 inUvCoord;

      out vec2 uvCoord;

      uniform vec2 originOffset;
      uniform float tileDepth;
      uniform vec2 framebufferResolution;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin, tileDepth, 1.0f);
        uvCoord = inUvCoord;
      }
    );

    desc.fs.source = PUL_SHADER(
      uniform sampler2D baseSampler;
      in vec2 uvCoord;
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

void ParseLayerTile(
  pul::core::SceneBundle &
, cJSON * layer
, char const * layerLabel
) {
  cJSON * chunk;
  cJSON_ArrayForEach(
    chunk, cJSON_GetObjectItemCaseSensitive(layer, "chunks")
  ) {
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

void ParseLayerCreature(pul::core::SceneBundle & scene , cJSON * layer) {
  auto & registry = scene.EnttRegistry();
  cJSON * creatureobj;

  cJSON_ArrayForEach(
    creatureobj, cJSON_GetObjectItemCaseSensitive(layer, "objects")
  ) {
    std::string creatureLabel = {};

    cJSON * creatureProperties;
    cJSON_ArrayForEach(
      creatureProperties
    , cJSON_GetObjectItemCaseSensitive(creatureobj, "properties")
    ) {
      if (std::string{creatureProperties->string} == "creature-name") {
        creatureLabel = std::string{creatureProperties->valuestring};
      }
    }

    auto const origin =
      glm::vec2 {
          cJSON_GetObjectItemCaseSensitive(creatureobj, "x")->valueint
        , cJSON_GetObjectItemCaseSensitive(creatureobj, "y")->valueint
      }
    ;

    spdlog::info("adding of label: '{}' @ {}", creatureLabel, origin);
    if (std::string{creatureLabel} == std::string{"lump"}) {
      spdlog::info("adding for real");
      auto lumpEntity = registry.create();
      registry.emplace<pul::core::ComponentCreatureLump>(
        lumpEntity,
        pul::core::ComponentCreatureLump { .origin = origin, }
      );

      pul::animation::Instance lumpAnimationInstance;
      plugin::animation::ConstructInstance(
        scene, lumpAnimationInstance, scene.AnimationSystem()
      , "creature-lump"
      );

      lumpAnimationInstance.origin = origin;
      lumpAnimationInstance.pieceToState["body"].Apply("idle", true);
      registry.emplace<pul::animation::ComponentInstance>(
        lumpEntity, std::move(lumpAnimationInstance)
      );
    }
  }
}

void ParseLayerObject(pul::core::SceneBundle & scene , cJSON * layer) {
  cJSON * object;

  cJSON_ArrayForEach(
    object, cJSON_GetObjectItemCaseSensitive(layer, "objects")
  ) {
    size_t objectId = cJSON_GetObjectItemCaseSensitive(object, "id")->valueint;
    size_t tileId = -1ul;
    if (cJSON_GetObjectItemCaseSensitive(object, "gid"))
      { tileId = cJSON_GetObjectItemCaseSensitive(object, "gid")->valueint; }

    std::string objectTypeStr = "";
    auto objectType = cJSON_GetObjectItemCaseSensitive(object, "type");
    cJSON * jsonTile = nullptr;

    // if the object type is empty then it has been cached & we need to locate
    // it in the tileset
    if (
        !objectType
     || (objectTypeStr = std::string{objectType->valuestring}) == ""
    ) {
      // if type is empty then check in the tileset for type

      // get tile/spritesheet info
      size_t spritesheetIdx;
      size_t localTileId;
      if (!GetMapTileset(tileId, spritesheetIdx, localTileId)) { return; }

      // iterate thru tiles to find ID
      cJSON * tile;
      cJSON * jsonTiles = ::mapTilesets[spritesheetIdx].jsonTiles;
      bool foundId = false;
      cJSON_ArrayForEach(tile, jsonTiles) {
        size_t id = cJSON_GetObjectItemCaseSensitive(tile, "id")->valueint;
        if (id == localTileId) {

          if (auto jsonType = cJSON_GetObjectItemCaseSensitive(tile, "type")) {
            objectTypeStr = jsonType->valuestring;
            jsonTile = tile;
            foundId = true;
          }
          break;
        }
      }

      if (!foundId) {
        spdlog::error(
          "could not find object ID {} for tile {} in tileset {}"
        , objectId, localTileId
        , ::mapTilesets[spritesheetIdx].spritesheet.filename
        );
      }
    }

    if (objectTypeStr == "player-spawner") {
      glm::i32vec2 const origin =
        glm::vec2(
          cJSON_GetObjectItemCaseSensitive(object, "x")->valueint
        , cJSON_GetObjectItemCaseSensitive(object, "y")->valueint
        );

      scene.PlayerMetaInfo().playerSpawnPoints.emplace_back(origin);
    }

    if (objectTypeStr == "item-pickup") {
      auto & registry = scene.EnttRegistry();
      auto pickupEntity = registry.create();

      glm::vec2 const origin =
        glm::vec2(
          cJSON_GetObjectItemCaseSensitive(object, "x")->valueint
        , cJSON_GetObjectItemCaseSensitive(object, "y")->valueint
        );

      pul::core::PickupType pickupType;
      pul::core::WeaponType weaponPickupType = pul::core::WeaponType::Size;

      // locate pickup type & weapon type
      bool applyPickupBg = false;
      std::string animationPickupStr = "";
      std::string animationStatePickupStr = "";
      cJSON * property;
      cJSON_ArrayForEach(
        property, cJSON_GetObjectItemCaseSensitive(jsonTile, "properties")
      ) {
        std::string
          name = cJSON_GetObjectItemCaseSensitive(property, "name")->valuestring
        , val = cJSON_GetObjectItemCaseSensitive(property, "value")->valuestring
        ;

        if (name == "type") {
          animationPickupStr = "pickups";
          if (animationStatePickupStr == "")
            { animationStatePickupStr = val; }

          if (val == "armor-large")
            { pickupType = pul::core::PickupType::ArmorLarge; }
          if (val == "armor-medium")
            { pickupType = pul::core::PickupType::ArmorMedium; }
          if (val == "armor-small")
            { pickupType = pul::core::PickupType::ArmorSmall; }
          if (val == "health-large")
            { pickupType = pul::core::PickupType::HealthLarge; }
          if (val == "health-medium")
            { pickupType = pul::core::PickupType::HealthMedium; }
          if (val == "health-small")
            { pickupType = pul::core::PickupType::HealthSmall; }
          if (val == "weapon") {
            animationPickupStr = "pickup-weapon";
            applyPickupBg = true;
            pickupType = pul::core::PickupType::Weapon; 
          }
        } else if (name == "weapon") {

          if (val != "")
            { animationStatePickupStr = val; }

          if (val == "bad-fetus")
            { weaponPickupType = pul::core::WeaponType::BadFetus; }
          if (val == "doppler-beam")
            { weaponPickupType = pul::core::WeaponType::DopplerBeam; }
          if (val == "grannibal")
            { weaponPickupType = pul::core::WeaponType::Grannibal; }
          if (val == "manshredder")
            { weaponPickupType = pul::core::WeaponType::Manshredder; }
          if (val == "pericaliya")
            { weaponPickupType = pul::core::WeaponType::Pericaliya; }
          if (val == "pmf")
            { weaponPickupType = pul::core::WeaponType::PMF; }
          if (val == "volnias")
            { weaponPickupType = pul::core::WeaponType::Volnias; }
          if (val == "wallbanger")
            { weaponPickupType = pul::core::WeaponType::Wallbanger; }
          if (val == "zeus-stinger")
            { weaponPickupType = pul::core::WeaponType::ZeusStinger; }
          if (val == "all")
            { pickupType = pul::core::PickupType::WeaponAll; }
        }
      }

      registry.emplace<pul::core::ComponentPickup>(
        pickupEntity, pickupType, weaponPickupType, origin, true, 0ul
      );

      pul::animation::Instance pickupAnimationInstance;
      plugin::animation::ConstructInstance(
        scene, pickupAnimationInstance, scene.AnimationSystem()
      , animationPickupStr.c_str()
      );

      pickupAnimationInstance.origin = origin;
      pickupAnimationInstance
        .pieceToState["pickups"].Apply(animationStatePickupStr, true);
      if (applyPickupBg) {
        pickupAnimationInstance
          .pieceToState["pickup-bg"].Apply(animationStatePickupStr, true);
      }

      registry.emplace<pul::animation::ComponentInstance>(
        pickupEntity, std::move(pickupAnimationInstance)
      );
    }
  }
}

} // -- namespace

void plugin::map::LoadMap(
  pul::core::SceneBundle & scene
, char const * filename
) {
  scene.mapFilename = filename;
  spdlog::info("Loading map '{}'", filename);

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
  cJSON_ArrayForEach(
    tileset, cJSON_GetObjectItemCaseSensitive(map, "tilesets")
  ) {

    cJSON * tilesetJson;

    // either the file is embedded or it is externally loaded
    if (cJSON_GetObjectItemCaseSensitive(tileset, "source")) {
      auto tilesetFilename =
        cJSON_GetObjectItemCaseSensitive(tileset, "source")->valuestring;

      spdlog::debug("loading tileset '{}'", tilesetFilename);

      // load file
      std::filesystem::path tilesetJsonPath =
          std::filesystem::path(filename).remove_filename() / tilesetFilename
      ;

      auto file = std::ifstream{tilesetJsonPath.string()};
      if (file.eof() || !file.good()) {
        spdlog::error("could not load tileset '{}'", tilesetJsonPath.string());
        return;
      }

      auto str =
        std::string {
          std::istreambuf_iterator<char>(file)
        , std::istreambuf_iterator<char>()
        };

      tilesetJson = cJSON_Parse(str.c_str());
    } else {
      tilesetJson = tileset;
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
      auto image = pul::gfx::Image::Construct(tilesetPath.string().c_str());

      // get plugin to load tileset
      pul::physics::Tileset physxTileset;
      plugin::physics::ProcessTileset(physxTileset, image);

      auto tilesJson = cJSON_GetObjectItemCaseSensitive(tilesetJson, "tiles");
      // copy tilesJson if not null
      if (tilesJson)
        { tilesJson = cJSON_Duplicate(tilesJson, true); }

      // emplace tileset w/ spritesheet and related tilemap info
      ::mapTilesets
        .emplace_back(MapTileset {
            pul::gfx::Spritesheet::Construct(image)
          , std::move(physxTileset)
          , tilesJson
          , static_cast<size_t>(
              cJSON_GetObjectItemCaseSensitive(tileset, "firstgid")->valueint
            )
        });
    }
  }

  cJSON * layer;
  cJSON_ArrayForEach(layer, cJSON_GetObjectItemCaseSensitive(map, "layers")) {
    auto layerLabel =
      cJSON_GetObjectItemCaseSensitive(layer, "name")->valuestring;

    auto const layerType =
      std::string{cJSON_GetObjectItemCaseSensitive(layer, "type")->valuestring};

    if (layerType == "tilelayer") {
      ParseLayerTile(scene, layer, layerLabel);
    } else if (layerType == "objectgroup") {

      if (std::string{layerLabel} == std::string{"creatures"})
        ParseLayerCreature(scene, layer);
      else
        ParseLayerObject(scene, layer);
    } else {
      spdlog::error("unable to parse layer of type '{}'", layerType);
    }

  }

  ::MapSokolEnd();

  { // create physics geometry for map

    std::vector<pul::physics::Tileset const *> tilesets;
    std::vector<std::span<size_t>> mapTileIndices;
    std::vector<std::span<glm::u32vec2>> mapTileOrigins;
    std::vector<std::span<pul::core::TileOrientation>> mapTileOrientations;

    for (auto & renderable : ::renderables) {
      // only depth 0 layers can have collision
      if (renderable.depth != 0) { continue; }

      tilesets
        .emplace_back(
          &::mapTilesets[renderable.spritesheetPrimaryIdx].physicsTileset
        );

      mapTileIndices.emplace_back(std::span(renderable.tileIds));
      mapTileOrigins.emplace_back(std::span(renderable.tileOrigins));
      mapTileOrientations.emplace_back(std::span(renderable.tileOrientations));
    }

    plugin::physics::LoadMapGeometry(
      tilesets, mapTileIndices, mapTileOrigins, mapTileOrientations
    );

    // create navigation map
    plugin::bot::BuildNavigationMap(filename);
  }

  cJSON_Delete(map);
}

void plugin::map::Render(
  pul::core::SceneBundle const & scene
, pul::core::RenderBundleInstance const & renderBundle
) {
  sg_apply_pipeline(pipeline);

  glm::vec2 cameraOrigin = renderBundle.cameraOrigin;

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 0
  , &cameraOrigin.x
  , sizeof(float) * 2ul
  );

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 2
  , &scene.config.framebufferDimFloat.x
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

void plugin::map::DebugUiDispatch(pul::core::SceneBundle & scene) {
  ImGui::Begin("Map Info");

  ImGui::DragInt2("map origin", &scene.cameraOrigin.x);

  static size_t tileInfoTilesetIdx = -1ul;
  static glm::vec2 tileInfoPixelClicked;

  ImGui::Separator();
  pul::imgui::Text("total tilemap sets: {}", ::mapTilesets.size());

  for (size_t i = 0ul; i < ::mapTilesets.size(); ++ i) {
    auto const & tileset = ::mapTilesets[i];
    auto & spritesheet = tileset.spritesheet;
    pul::imgui::Text("filename: '{}'", tileset.spritesheet.filename);
    pul::imgui::Text(
      "tile width {} height {} total {}"
    , tileset.spritesheet.width / 32ul
    , tileset.spritesheet.height / 32ul
    , tileset.spritesheet.height / 32ul * tileset.spritesheet.width / 32ul
    );
    pul::imgui::Text("idx: {}", i);

    ImGui::Image(
      reinterpret_cast<void *>(spritesheet.Image().id)
    , ImVec2(spritesheet.width, spritesheet.height), ImVec2(0, 1), ImVec2(1, 0)
    );

    if (pul::imgui::CheckImageClicked(tileInfoPixelClicked)) {
      tileInfoTilesetIdx = i;
    }
  }

  ImGui::End();

  if (tileInfoTilesetIdx != -1ul) {
    ImGui::End();

    bool open = true;
    ImGui::Begin("Tile Info", &open);

    if (!open) {
      tileInfoTilesetIdx = -1ul;
    } else {
      pul::imgui::Text("tileset clicked {}", tileInfoTilesetIdx);
      pul::imgui::Text(
        "pixel clicked {}x{}", tileInfoPixelClicked.x, tileInfoPixelClicked.y
      );

      // calculate tile indices for spritesheet
      size_t tileIdx;
      glm::u32vec2 origin;
      pul::util::CalculateTileIndices(
        tileIdx, origin, glm::u32vec2(tileInfoPixelClicked)
      , ::mapTilesets[tileInfoTilesetIdx].spritesheet.width / 32ul
      , ::mapTilesets[tileInfoTilesetIdx].physicsTileset.tiles.size()
      );

      pul::imgui::Text("tile idx {} origin {}", tileIdx, origin);

      pul::physics::Tile const & physicsTile =
        ::mapTilesets[tileInfoTilesetIdx].physicsTileset.tiles[tileIdx];

      std::string physxStr = "";
      for (size_t i = 0; i < 32*32; ++ i) {
        if (i % 32 == 0 && i > 0) {
          physxStr += "\n";
        }

        physxStr +=
          physicsTile.signedDistanceField[i%32][i/32] > 0.0f ? "#" : "-";
      }

      pul::imgui::Text("{}", physxStr);
    }

    ImGui::End();
  }

  ImGui::Begin("Map Info");

  ImGui::Separator();
  ImGui::Separator();

  pul::imgui::Text("map renderables: {}", ::renderables.size());
  for (auto & renderable : ::renderables) {
    ImGui::PushID(&renderable);
    pul::imgui::Text("draw call: {}", renderable.tileCount);
    pul::imgui::Text("depth: {}", renderable.depth);
    pul::imgui::Text("spritesheet: {}u", renderable.spritesheetPrimaryIdx);
    ImGui::Checkbox("enabled", &renderable.enabled);

    ImGui::Separator();
    ImGui::PopID();
  }

  ImGui::End();
}

void plugin::map::Shutdown() {
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

  for (auto & mapTileset : ::mapTilesets) {
    if (mapTileset.jsonTiles)
      { cJSON_Delete(mapTileset.jsonTiles); }
  }
  ::mapTilesets.clear();
}
