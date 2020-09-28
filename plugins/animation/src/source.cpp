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

void DisplayImGuiSkeleton(
  pulcher::animation::Instance & instance,
  std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
) {
  for (auto const & skeletal : skeletals) {
    if (
      ImGui::TreeNode(fmt::format("display '{}'", skeletal.label).c_str())
    ) {
      auto & stateInfo = instance.pieceToState[skeletal.label];
      pul::imgui::Text("State '{}'", stateInfo.label);
      pul::imgui::Text("current delta time {}", stateInfo.deltaTime);
      pul::imgui::Text("componentIt {}", stateInfo.componentIt);

      ImGui::Separator();
      DisplayImGuiSkeleton(instance, skeletal.children);

      ImGui::TreePop();
    }
  }
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

  auto & component = state.components[stateInfo.componentIt];

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
        if (order < 0 || order >= 100) {
          spdlog::error(
            "render-order for '{}' of '{}' is OOB ({}); "
            "range must be from 0 to 100"
          , pieceLabel, animator->label, order
          );
          order = 99;
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

      if (self.instance.sgBufferOrigin.id) {
        sg_destroy_buffer(self.instance.sgBufferOrigin);
        self.instance.sgBufferOrigin = {};
      }

      if (self.instance.sgBufferUvCoord.id) {
        sg_destroy_buffer(self.instance.sgBufferUvCoord);
        self.instance.sgBufferUvCoord = {};
      }
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

PUL_PLUGIN_DECL void UiRender(pulcher::core::SceneBundle & scene) {

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
    ImGui::Begin("Animation Editor");

    auto & instance = *editInstance;

    DisplayImGuiSkeleton(instance, editInstance->animator->skeleton);

    ImGui::Separator();

    pul::imgui::Text("animator '{}'", instance.animator->label);

    ImGui::Separator();

    size_t pieceIt = 0ul;
    for (auto & piecePair : instance.animator->pieces) {
      ImGui::Separator();

      ImGui::PushID(++ pieceIt);

      pul::imgui::Text("piece '{}'", piecePair.first);

      size_t statePairIt = 0ul;
      for (auto & statePair : piecePair.second.states) {
        pul::imgui::Text("state '{}'", statePair.first);

        ImGui::PushID(++ statePairIt);

        ImGui::SliderFloat(
          "delta time", &statePair.second.msDeltaTime, 0.0f, 1000.0f
        );

        ImGui::Separator();

        auto & components = statePair.second.components;

        for (
          size_t componentIt = 0ul;
          componentIt < components.size();
          ++ componentIt
        ) {
          auto & component = components[componentIt];

          ImGui::PushID(componentIt);

          {
            std::array<int, 2> inpInt;
            inpInt[0] = component.tile.x;
            inpInt[1] = component.tile.y;
            if (ImGui::InputInt2("tile", inpInt.data())) {
              component.tile = {inpInt[0], inpInt[1]};
            }
          }

          {
            std::array<int, 2> inpInt;
            inpInt[0] = component.originOffset.x;
            inpInt[1] = component.originOffset.y;
            if (ImGui::InputInt2("origin-offset", inpInt.data())) {
              component.originOffset = {inpInt[0], inpInt[1]};
            }
          }

          if (ImGui::Button("-")) {
            components.erase(components.begin() + componentIt);
            -- componentIt;
          }

          ImGui::SameLine();
          if (ImGui::Button("+")) {
            components.insert(components.begin() + componentIt, {});
            -- componentIt;
          }

          ImGui::SameLine();
          if (componentIt > 0ul && ImGui::Button("^"))
            { std::swap(components[componentIt], components[componentIt-1]); }

          ImGui::SameLine();
          if (componentIt < components.size()-1 && ImGui::Button("V"))
            { std::swap(components[componentIt], components[componentIt+1]); }

          ImGui::Separator();

          ImGui::PopID(); // componentIt
        }

        ImGui::PopID(); // statePairIt
      }

      ImGui::PopID(); // pieceIt
    }

    ImGui::End();
  }
}

} // -- extern C