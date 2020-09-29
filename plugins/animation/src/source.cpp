#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>
#include <sokol/gfx.hpp>

#include <fstream>

namespace {

static size_t animMsTimer = 0ul;
static bool animLoop = true;
static float animShowPreviousSprite = 0.0f;
static bool animShowZoom = false;
static bool animPlaying = true;
static bool animEmptyOnLoopEnd = false;
static size_t animMaxTime = 100'000ul;

void JsonParseRecursiveSkeleton(
  cJSON * skeletalParentJson
, std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
) {
  if (!skeletalParentJson) { return; }
  cJSON * skeletalChildJson;
  cJSON_ArrayForEach(
    skeletalChildJson
  , cJSON_GetObjectItemCaseSensitive(skeletalParentJson, "skeleton")
  ) {
    pulcher::animation::Animator::SkeletalPiece skeletal;
    skeletal.label =
      cJSON_GetObjectItemCaseSensitive(skeletalChildJson, "label")->valuestring;
    JsonParseRecursiveSkeleton(skeletalChildJson, skeletal.children);
    skeletals.emplace_back(std::move(skeletal));
  }
}

cJSON * JsonWriteRecursiveSkeleton(
  std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
) {
  if (skeletals.size() == 0ul) { return nullptr; }

  cJSON * skeletalsJson = cJSON_CreateArray();
  for (auto & skeletal : skeletals) {
    cJSON * skeletalJson = cJSON_CreateObject();
    cJSON_AddItemToArray(skeletalsJson, skeletalJson);

    cJSON_AddItemToObject(
      skeletalJson, "label", cJSON_CreateString(skeletal.label.c_str())
    );

    if (auto childrenJson = JsonWriteRecursiveSkeleton(skeletal.children)) {
      cJSON_AddItemToObject(skeletalJson, "skeleton", childrenJson);
    }
  }

  return skeletalsJson;
}

size_t ComputeVertexBufferSize(
  std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
) {
  size_t vertices = 0ul;
  for (const auto & skeletal : skeletals) {
    vertices += 6 + ComputeVertexBufferSize(skeletal.children);
  }
  return vertices;
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, pulcher::animation::Animator::SkeletalPiece const & skeletal
, size_t & indexOffset
, glm::i32vec2 & originOffset
, bool & forceUpdate
) {
  // okay the below is crazy with the map lookups, I need to cache the
  // state somehow
  auto & piece = instance.animator->pieces[skeletal.label];
  auto & stateInfo = instance.pieceToState[skeletal.label];
  auto & state = piece.states[stateInfo.label];

  // if there are no components to render, output a degenerate tile
  if (state.components.size() == 0ul) {
    for (size_t it = 0ul; it < 6ul; ++ it) {
      instance.uvCoordBufferData[indexOffset + it] = glm::vec2(-1);
      instance.originBufferData[indexOffset + it] = glm::vec3(-1);
    }

    indexOffset += 6ul;
    return;
  }

  auto & component = state.components[stateInfo.componentIt];

  // update delta time and if animation update is necessary, apply uv coord
  // updates
  bool hasUpdate = forceUpdate;
  if (state.msDeltaTime > 0.0f) {
    stateInfo.deltaTime += pulcher::util::msPerFrame;
    if (stateInfo.deltaTime > state.msDeltaTime) {
      stateInfo.deltaTime = stateInfo.deltaTime - state.msDeltaTime;
      stateInfo.componentIt =
        (stateInfo.componentIt + 1) % state.components.size();
      hasUpdate = true;
    }
  }

  // update originOffset
  originOffset += piece.origin + component.originOffset;

  if (!hasUpdate) {
    indexOffset += 6ul;
    return;
  }

  forceUpdate = true;

  auto pieceDimensions = glm::vec2(piece.dimensions);
  auto pieceOrigin =
    glm::vec3(piece.origin, static_cast<float>(piece.renderOrder));

  // update origins & UV coords
  for (size_t it = 0ul; it < 6; ++ it, ++ indexOffset) {
    auto v = pulcher::util::TriangleVertexArray()[it];

    instance.uvCoordBufferData[indexOffset] =
        (v*pieceDimensions + glm::vec2(component.tile)*pieceDimensions)
      * instance.animator->spritesheet.InvResolution()
    ;

    instance.originBufferData[indexOffset] =
        glm::vec3(
          v*pieceDimensions + glm::vec2(originOffset)
        , static_cast<float>(piece.renderOrder)
        )
    ;
  }
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
, size_t & indexOffset
, const glm::i32vec2 originOffset
, bool const forceUpdate
) {
  for (const auto & skeletal : skeletals) {
    // compute origin / uv coords
    glm::i32vec2 newOriginOffset = originOffset;
    bool newForceUpdate = forceUpdate;
    ComputeVertices(
      instance, skeletal, indexOffset, newOriginOffset, newForceUpdate
    );

    // continue to children
    ComputeVertices(
      instance, skeletal.children, indexOffset, newOriginOffset, newForceUpdate
    );
  }
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, bool forceUpdate = false
) {
  size_t indexOffset = 0ul;
  ComputeVertices(
    instance, instance.animator->skeleton, indexOffset, glm::vec2()
  , forceUpdate
  );
}

} // -- namespace

extern "C" {
PUL_PLUGIN_DECL void DestroyInstance(
  pulcher::animation::Instance & instance
) {
  if (instance.sgBufferOrigin.id) {
    sg_destroy_buffer(instance.sgBufferOrigin);
    instance.sgBufferOrigin = {};
  }

  if (instance.sgBufferUvCoord.id) {
    sg_destroy_buffer(instance.sgBufferUvCoord);
    instance.sgBufferUvCoord = {};
  }

  instance.uvCoordBufferData = {};
  instance.originBufferData = {};
  instance.pieceToState = {};
  instance.animator = {};
}

PUL_PLUGIN_DECL void ConstructInstance(
  pulcher::animation::Instance & animationInstance
, pulcher::animation::System & animationSystem
, char const * label
) {
  if (
    auto instance = animationSystem.animators.find(label);
    instance != animationSystem.animators.end()
  ) {
    animationInstance.animator = instance->second;
  } else {
    spdlog::error("Could not find animation of type '{}'", label);
    return;
  }

  // set default values for pieces
  for (auto & piecePair : animationInstance.animator->pieces) {
    if (piecePair.second.states.begin() == piecePair.second.states.end()) {
      spdlog::error(
        "need at least one state for piece '{}' of '{}'"
      , piecePair.first, animationInstance.animator->label
      );
      continue;
    }
    animationInstance.pieceToState[piecePair.first] =
      {piecePair.second.states.begin()->first};
  }

  { // -- compute initial sokol buffers

    // precompute size
    size_t const vertexBufferSize =
      ::ComputeVertexBufferSize(animationInstance.animator->skeleton);
    spdlog::debug("vertex buffer size {}", vertexBufferSize);

    animationInstance.uvCoordBufferData.resize(vertexBufferSize);
    animationInstance.originBufferData.resize(vertexBufferSize);

    ::ComputeVertices(animationInstance, true);

    { // -- origin
      sg_buffer_desc desc = {};
      desc.size = vertexBufferSize * sizeof(glm::vec3);
      desc.usage = SG_USAGE_STREAM;
      desc.content = animationInstance.originBufferData.data();
      desc.label = "origin buffer";
      animationInstance.sgBufferOrigin = sg_make_buffer(&desc);
    }

    { // -- uv coord
      sg_buffer_desc desc = {};
      desc.size = vertexBufferSize * sizeof(glm::vec2);
      desc.usage = SG_USAGE_STREAM;
      desc.content = animationInstance.uvCoordBufferData.data();
      desc.label = "uv coord buffer";
      animationInstance.sgBufferUvCoord = sg_make_buffer(&desc);
    }

    // bindings
    animationInstance.sgBindings.vertex_buffers[0] =
      animationInstance.sgBufferOrigin;
    animationInstance.sgBindings.vertex_buffers[1] =
      animationInstance.sgBufferUvCoord;
    animationInstance.sgBindings.fs_images[0] =
      animationInstance.animator->spritesheet.Image();

    // get draw call count
    animationInstance.drawCallCount = vertexBufferSize;
  }
}

} // -- extern C

namespace {

void ReconstructInstance(
  pulcher::animation::Instance & instance
, pulcher::animation::System & system
) {
  auto label = instance.animator->label;
  DestroyInstance(instance);
  ConstructInstance(instance, system, label.c_str());
}

void DisplayImGuiSkeleton(
  pulcher::animation::Instance & instance
, pulcher::animation::System & system
, std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
) {

  ImGui::SameLine();
  if (ImGui::Button("add"))
    { ImGui::OpenPopup("add skeletal"); }

  if (ImGui::BeginPopup("add skeletal")) {
    for (auto & piecePair : instance.animator->pieces) {
      auto & label = std::get<0>(piecePair);
      if (ImGui::Selectable(label.c_str())) {
        skeletals.emplace_back(
          pulcher::animation::Animator::SkeletalPiece{label, {}}
        );
        ReconstructInstance(instance, system);
      }
    }

    ImGui::EndPopup();
  }

  size_t skeletalIdx = 0ul;
  for (auto & skeletal : skeletals) {
    if (
      !ImGui::TreeNode(fmt::format("skeletal '{}'", skeletal.label).c_str())
    ) {
      ++ skeletalIdx;
      continue;
    }

    auto & stateInfo = instance.pieceToState[skeletal.label];
    pul::imgui::Text("State '{}'", stateInfo.label);
    pul::imgui::Text("current delta time {}", stateInfo.deltaTime);
    pul::imgui::Text("componentIt {}", stateInfo.componentIt);

    if (ImGui::Button("remove")) {
      skeletals.erase(skeletals.begin() + skeletalIdx);
      ReconstructInstance(instance, system);

      ImGui::TreePop();
      continue;
    }

    DisplayImGuiSkeleton(instance, system, skeletal.children);

    ImGui::TreePop();

    ++ skeletalIdx;
  }
}

void ImGuiRenderSpritesheetTile(
  pulcher::animation::Instance & instance
, pulcher::animation::Animator::Piece & piece
, pulcher::animation::Animator::Component & component
, float alpha = 1.0f
) {
  auto & spritesheet = instance.animator->spritesheet;

  auto pieceDimensions = glm::vec2(piece.dimensions);
  auto const imgUl =
    glm::vec2(component.tile)*pieceDimensions
  * spritesheet.InvResolution()
  ;

  auto const imgLr =
    imgUl + pieceDimensions * spritesheet.InvResolution()
  ;

  ImVec2 dimensions = ImVec2(piece.dimensions.x, piece.dimensions.y);

  if (animShowZoom) {
    dimensions.x *= 5.0f;
    dimensions.y *= 5.0f;
  }

  ImGui::Image(
    reinterpret_cast<void *>(spritesheet.Image().id)
  , dimensions
  , ImVec2(imgUl.x, 1.0f-imgUl.y)
  , ImVec2(imgLr.x, 1.0f-imgLr.y)
  , (
      animMsTimer == animMaxTime && animEmptyOnLoopEnd
    ? ImVec4(0, 0, 0, 0) : ImVec4(1, 1, 1, alpha)
    )
  , ImVec4(1, 1, 1, 1)
  );
}

void SaveAnimations(pulcher::animation::System & system) {
  cJSON * playerDataJson;

  playerDataJson = cJSON_CreateObject();

  cJSON * spritesheetsJson = cJSON_CreateArray();
  cJSON_AddItemToObject(playerDataJson, "spritesheets", spritesheetsJson);

  for (auto & animatorPair : system.animators) {
    auto & animator = *animatorPair.second;

    cJSON * spritesheetJson = cJSON_CreateObject();
    cJSON_AddItemToArray(spritesheetsJson, spritesheetJson);

    cJSON_AddItemToObject(
      spritesheetJson, "label", cJSON_CreateString(animator.label.c_str())
    );

    cJSON_AddItemToObject(
      spritesheetJson
    , "filename", cJSON_CreateString(animator.spritesheet.filename.c_str())
    );

    // TODO bug if skeleton is empty i think
    cJSON_AddItemToObject(
      spritesheetJson
    , "skeleton"
    , JsonWriteRecursiveSkeleton(animator.skeleton)
    );

    cJSON * animationPieceJson = cJSON_CreateArray();
    cJSON_AddItemToObject(
      spritesheetJson, "animation-piece", animationPieceJson
    );

    for (auto const & piecePair : animator.pieces) {
      auto const & pieceLabel = std::get<0>(piecePair);
      auto const & piece = std::get<1>(piecePair);

      cJSON * pieceJson = cJSON_CreateObject();
      cJSON_AddItemToArray(animationPieceJson, pieceJson);

      cJSON_AddItemToObject(
        pieceJson, "label", cJSON_CreateString(pieceLabel.c_str())
      );
      cJSON_AddItemToObject(
        pieceJson, "dimension-x", cJSON_CreateInt(piece.dimensions.x)
      );
      cJSON_AddItemToObject(
        pieceJson, "dimension-y", cJSON_CreateInt(piece.dimensions.y)
      );
      cJSON_AddItemToObject(
        pieceJson, "origin-x", cJSON_CreateInt(piece.origin.x)
      );
      cJSON_AddItemToObject(
        pieceJson, "origin-y", cJSON_CreateInt(piece.origin.y)
      );
      cJSON_AddItemToObject(
        pieceJson, "render-order", cJSON_CreateInt(piece.renderOrder)
      );

      cJSON * statesJson = cJSON_CreateArray();
      cJSON_AddItemToObject(pieceJson, "states", statesJson);

      for (auto const & statePair : piece.states) {
        auto const & stateLabel = std::get<0>(statePair);
        auto const & state = std::get<1>(statePair);

        cJSON * stateJson = cJSON_CreateObject();
        cJSON_AddItemToArray(statesJson, stateJson);

        cJSON_AddItemToObject(
          stateJson, "label", cJSON_CreateString(stateLabel.c_str())
        );
        cJSON_AddItemToObject(
          stateJson, "ms-delta-time", cJSON_CreateInt(state.msDeltaTime)
        );

        cJSON * componentsJson = cJSON_CreateArray();
        cJSON_AddItemToObject(stateJson, "components", componentsJson);

        for (auto const & component : state.components) {
          cJSON * componentJson = cJSON_CreateObject();
          cJSON_AddItemToArray(componentsJson, componentJson);

          cJSON_AddItemToObject(
            componentJson, "x", cJSON_CreateInt(component.tile.x)
          );
          cJSON_AddItemToObject(
            componentJson, "y", cJSON_CreateInt(component.tile.y)
          );
          cJSON_AddItemToObject(
            componentJson, "origin-offset-x"
          , cJSON_CreateInt(component.originOffset.x)
          );
          cJSON_AddItemToObject(
            componentJson, "origin-offset-y"
          , cJSON_CreateInt(component.originOffset.y)
          );
        }
      }
    }
  }

  auto jsonStr = cJSON_Print(playerDataJson);

  spdlog::info("saving: \n{}", jsonStr);

  cJSON_Delete(playerDataJson);
}

}

extern "C" {
PUL_PLUGIN_DECL void LoadAnimations(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {

  // note that this will probably need to be expanded to handle generic amount
  // of spritesheet types (player, particles, enemies, etc)

  cJSON * playerDataJson;
  {
    // load file
    auto file = std::ifstream{"base/spritesheets/player/data.json"};
    if (file.eof() || !file.good()) {
      spdlog::error("could not load player spritesheet");
      return;
    }

    auto str =
      std::string {
        std::istreambuf_iterator<char>(file)
      , std::istreambuf_iterator<char>()
      };

    playerDataJson = cJSON_Parse(str.c_str());

    if (playerDataJson == nullptr) {
      spdlog::critical(
        " -- failed to parse json for player spritesheet; '{}'"
      , cJSON_GetErrorPtr()
      );
      return;
    }
  }

  auto & animationSystem = scene.AnimationSystem();

  animationSystem.filename = "base/spritesheets/player/data.json";

  cJSON * sheetJson;
  cJSON_ArrayForEach(
    sheetJson, cJSON_GetObjectItemCaseSensitive(playerDataJson, "spritesheets")
  ) {
    auto animator = std::make_shared<pulcher::animation::Animator>();

    animator->label =
      std::string{
        cJSON_GetObjectItemCaseSensitive(sheetJson, "label")->valuestring
      };
    spdlog::debug("loading animation spritesheet '{}'", animator->label);

    animator->spritesheet =
      pulcher::gfx::Spritesheet::Construct(
        pulcher::gfx::Image::Construct(
          cJSON_GetObjectItemCaseSensitive(sheetJson, "filename")->valuestring
        )
      );

    cJSON * pieceJson;
    cJSON_ArrayForEach(
      pieceJson, cJSON_GetObjectItemCaseSensitive(sheetJson, "animation-piece")
    ) {
      std::string pieceLabel =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "label")->valuestring;

      pulcher::animation::Animator::Piece piece;

      piece.dimensions.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-x")->valueint;
      piece.dimensions.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-y")->valueint;

      piece.origin.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-x")->valueint;
      piece.origin.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-y")->valueint;
      {
        auto order =
          cJSON_GetObjectItemCaseSensitive(pieceJson, "render-order")->valueint;
        if (order < 0u || order >= 100u) {
          spdlog::error(
            "render-order for '{}' of '{}' is OOB ({}); "
            "range must be from 0 to 100"
          , pieceLabel, animator->label, order
          );
          order = 99u;
        }
        piece.renderOrder = order;
      }

      cJSON * stateJson;
      cJSON_ArrayForEach(
        stateJson, cJSON_GetObjectItemCaseSensitive(pieceJson, "states")
      ) {
        std::string stateLabel =
          cJSON_GetObjectItemCaseSensitive(stateJson, "label")->valuestring;

        pulcher::animation::Animator::State state;

        state.msDeltaTime =
          cJSON_GetObjectItemCaseSensitive(
            stateJson, "ms-delta-time"
          )->valueint;

        cJSON * componentJson;
        cJSON_ArrayForEach(
            componentJson
          , cJSON_GetObjectItemCaseSensitive(stateJson, "components")
        ) {
          pulcher::animation::Animator::Component component;
          component.tile.x =
            cJSON_GetObjectItemCaseSensitive(componentJson, "x")->valueint;

          component.tile.y =
            cJSON_GetObjectItemCaseSensitive(componentJson, "y")->valueint;

          if (
            auto offX =
              cJSON_GetObjectItemCaseSensitive(componentJson, "origin-offset-x")
          ) {
            component.originOffset.x = offX->valueint;
          }

          if (
            auto offY =
              cJSON_GetObjectItemCaseSensitive(componentJson, "origin-offset-y")
          ) {
            component.originOffset.y = offY->valueint;
          }

          state.components.emplace_back(component);
        }

        piece.states[stateLabel] = std::move(state);
      }

      animator->pieces[pieceLabel] = std::move(piece);
    }

    // load skeleton
    ::JsonParseRecursiveSkeleton(sheetJson, animator->skeleton);

    // store animator
    spdlog::debug("storing animator of {}", animator->label);
    animationSystem.animators[animator->label] = animator;
  }

  cJSON_Delete(playerDataJson);

  { // -- sokol animation program
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.fs.images[0].name = "baseSampler";
    desc.fs.images[0].type = SG_IMAGETYPE_2D;

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec3 inOrigin;
      layout(location = 1) in vec2 inUvCoord;

      out vec2 uvCoord;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin.xy)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin, 0.5001f + inOrigin.z/100000.0f, 1.0f);
        uvCoord = vec2(inUvCoord.x, 1.0f-inUvCoord.y);
      }
    );

    desc.fs.source = PUL_SHADER(
      uniform sampler2D baseSampler;
      in vec2 uvCoord;
      out vec4 outColor;
      void main() {
        outColor = texture(baseSampler, uvCoord);
        if (outColor.a < 0.1f) { discard; }
      }
    );

    animationSystem.sgProgram = sg_make_shader(&desc);
  }

  { // -- sokol pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[0].step_rate = 6u;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;

    desc.layout.buffers[1].stride = 0u;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[1].step_rate = 1u;
    desc.layout.attrs[1].buffer_index = 1;
    desc.layout.attrs[1].offset = 0;
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = animationSystem.sgProgram;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    desc.rasterizer.cull_mode = SG_CULLMODE_BACK;
    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "animation pipeline";

    animationSystem.sgPipeline = sg_make_pipeline(&desc);
  }
}

PUL_PLUGIN_DECL void Shutdown(pulcher::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  { // -- delete sokol animation information
    auto view = registry.view<pulcher::animation::ComponentInstance>();

    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      DestroyInstance(self.instance);
    }
  }

  sg_destroy_shader(scene.AnimationSystem().sgProgram);
  sg_destroy_pipeline(scene.AnimationSystem().sgPipeline);

  scene.AnimationSystem() = {};
}

PUL_PLUGIN_DECL void UpdateFrame(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  // update each component
  auto view = registry.view<pulcher::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

    ::ComputeVertices(self.instance);

    // must update entire animation buffer
    sg_update_buffer(
      self.instance.sgBufferUvCoord,
      self.instance.uvCoordBufferData.data(),
      self.instance.uvCoordBufferData.size() * sizeof(glm::vec2)
    );
    sg_update_buffer(
      self.instance.sgBufferOrigin,
      self.instance.originBufferData.data(),
      self.instance.originBufferData.size() * sizeof(glm::vec3)
    );
  }
}

PUL_PLUGIN_DECL void RenderAnimations(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  { // -- render sokol animations

    // bind pipeline & global uniforms
    sg_apply_pipeline(scene.AnimationSystem().sgPipeline);

    glm::vec2 animationOrigin = glm::vec2(0);

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 0
    , &animationOrigin.x
    , sizeof(float) * 2ul
    );

    std::array<float, 2> windowResolution {{
      static_cast<float>(pulcher::gfx::DisplayWidth())
    , static_cast<float>(pulcher::gfx::DisplayHeight())
    }};

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , windowResolution.data()
    , sizeof(float) * 2ul
    );

    // render each component
    auto view = registry.view<pulcher::animation::ComponentInstance>();
    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      sg_apply_bindings(self.instance.sgBindings);
      sg_draw(0, self.instance.drawCallCount, 1);
    }
  }
}

PUL_PLUGIN_DECL void UiRender(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  ImGui::Begin("Animation");

  auto & registry = scene.EnttRegistry();

  static pulcher::animation::Instance * editInstance = nullptr;

  { // -- display entity info
    auto view = registry.view<pulcher::animation::ComponentInstance>();

    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      ImGui::Separator();
      pul::imgui::Text(
        "animation instance '{}'", self.instance.animator->label
      );
      if (ImGui::Button("edit")) { editInstance = &self.instance; }
    }
  }
  ImGui::End();

  if (editInstance) {

    ImGui::Begin("Animation Timeline");
      ImGui::Checkbox("loop", &animLoop);
      ImGui::Checkbox("playing", &animPlaying);
      ImGui::Checkbox("empty on loop end", &animEmptyOnLoopEnd);

      ImGui::Separator();

      pul::imgui::InputInt("max loop time", &animMaxTime);
      pul::imgui::SliderInt("timer", &animMsTimer, 0, animMaxTime);

      ImGui::Separator();

      ImGui::Checkbox("sprite zoom", &animShowZoom);

      ImGui::SliderFloat(
        "previous sprite alpha", &animShowPreviousSprite, 0.0f, 1.0f
      );

      if (animPlaying) {
        animMsTimer += pulcher::util::msPerFrame;

        animMsTimer =
            animLoop
          ? animMsTimer % animMaxTime : glm::min(animMsTimer, animMaxTime)
        ;
      }
    ImGui::End();

    auto & instance = *editInstance;

    ImGui::Begin("Animation Skeleton");
      ImGui::Separator();
      ImGui::Text("skeleton");
      ImGui::Separator();
      ImGui::Text("");
      DisplayImGuiSkeleton(
        instance, scene.AnimationSystem(), editInstance->animator->skeleton
      );

      ImGui::Separator();
      ImGui::Separator();

    ImGui::End();

    ImGui::Begin("Animation Editor");

    if (ImGui::Button("reload")) {
      ReconstructInstance(instance, scene.AnimationSystem());
    }

    if (ImGui::Button("save")) {
      ::SaveAnimations(scene.AnimationSystem());
    }

    ImGui::Separator();

    pul::imgui::Text("animator '{}'", instance.animator->label);

    ImGui::Separator();

    ImGui::Text("pieces");
    ImGui::Separator();
    ImGui::Separator();

    size_t pieceIt = 0ul;
    for (auto & piecePair : instance.animator->pieces) {
      ImGui::Separator();

      auto & piece = piecePair.second;

      ImGui::PushID(++ pieceIt);

      if (ImGui::TreeNode(piecePair.first.c_str())) {

        pul::imgui::InputInt2("dimensions", &piece.dimensions.x);
        pul::imgui::InputInt2("origin", &piece.origin.x);
        pul::imgui::InputInt("render order", &piece.renderOrder);

        for (auto & statePair : piece.states) {
          ImGui::Separator();

          if (!ImGui::TreeNode(statePair.first.c_str())) {
            continue;
          }

          auto & components = statePair.second.components;

          if (components.size() > 0ul)
          { // display image
            size_t componentIt =
              static_cast<size_t>(animMsTimer / statePair.second.msDeltaTime);
            componentIt %= components.size();

            // render w/ previous sprite alpha if requested

            ImVec2 pos = ImGui::GetCursorScreenPos();
            if (animShowPreviousSprite > 0.0f) {
              ::ImGuiRenderSpritesheetTile(
                instance, piece
              , components[
                  (components.size() + componentIt - 1) % components.size()
                ]
              , animShowPreviousSprite
              );
            }
            ImGui::SetCursorScreenPos(pos);
            ::ImGuiRenderSpritesheetTile(
              instance, piece, components[componentIt]
            );

            ImGui::Separator();
          }

          ImGui::SliderFloat(
            "delta time", &statePair.second.msDeltaTime, 0.0f, 1000.0f
          );

          if (ImGui::Button("Set animation timer")) {
            animMaxTime =
              static_cast<size_t>(
                0.5f + components.size() * statePair.second.msDeltaTime
              );
          }

          // insert at 0, important for when component is size 0 also
          if (ImGui::Button("+")) {
            components.emplace(components.begin());
          }

          for (
            size_t componentIt = 0ul;
            componentIt < components.size();
            ++ componentIt
          ) {
            auto & component = components[componentIt];

            ImGui::PushID(componentIt);

            // do not show columns or editor properties on sprite zoom
            if (!::animShowZoom) {
              ImGui::Columns(2, nullptr, false);

              ImGui::SetColumnWidth(0, piece.dimensions.x + 16);
            }

            // display current image on left side of column, use transparency
            // w/ previous frame if requested
            ImVec2 pos = ImGui::GetCursorScreenPos();
            if (animShowPreviousSprite > 0.0f) {
              ::ImGuiRenderSpritesheetTile(
                instance, piece
              , components[
                  (components.size() + componentIt - 1) % components.size()
                ]
              , animShowPreviousSprite
              );
            }
            ImGui::SetCursorScreenPos(pos);
            ::ImGuiRenderSpritesheetTile(
              instance, piece, component
            );

            if (!::animShowZoom) {
              ImGui::NextColumn();
              pul::imgui::InputInt2("tile", &component.tile.x);

              pul::imgui::InputInt2("origin-offset", &component.originOffset.x);

              if (ImGui::Button("-")) {
                components.erase(components.begin() + componentIt);
                -- componentIt;
              }

              ImGui::SameLine();
              ImGui::SameLine();
              if (ImGui::Button("+")) {
                components
                  .emplace(components.begin() + componentIt + 1, component);
              }

              ImGui::SameLine();
              if (componentIt > 0ul && ImGui::Button("^")) {
                std::swap(components[componentIt], components[componentIt-1]);
              }

              ImGui::SameLine();
              if (componentIt < components.size()-1 && ImGui::Button("V")) {
                std::swap(components[componentIt], components[componentIt+1]);
              }

              ImGui::NextColumn();
            }

            ImGui::PopID(); // componentIt
            ImGui::Separator();
          }

          ImGui::Columns(1);

          ImGui::TreePop(); // state
        }

        ImGui::TreePop(); // piece
      }

      ImGui::PopID(); // pieceIt
    }

    ImGui::End();
  }
}

} // -- extern C
