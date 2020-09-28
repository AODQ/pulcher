#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>
#include <sokol/gfx.hpp>

#include <fstream>

namespace {
} // -- namespace

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

    cJSON * animationJson;
    cJSON_ArrayForEach(
      animationJson, cJSON_GetObjectItemCaseSensitive(sheetJson, "animation")
    ) {
      std::string pieceLabel =
        cJSON_GetObjectItemCaseSensitive(animationJson, "label")->valuestring;

      pulcher::animation::Animator::Piece piece;

      cJSON * stateJson;
      cJSON_ArrayForEach(
        stateJson, cJSON_GetObjectItemCaseSensitive(animationJson, "states")
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
          component.tileX =
            cJSON_GetObjectItemCaseSensitive(componentJson, "x")->valueint;

          component.tileY =
            cJSON_GetObjectItemCaseSensitive(componentJson, "y")->valueint;

          state.components.emplace_back(component);
        }

        piece.states[stateLabel] = std::move(state);
      }

      animator->pieces[pieceLabel] = std::move(piece);
    }

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
        gl_Position = vec4(vertexOrigin, 0.5f + inOrigin.z/100.0f, 1.0f);
        uvCoord = vec2(inUvCoord.x, 1.0f-inUvCoord.y);
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

PUL_PLUGIN_DECL void RenderAnimations(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  { // -- render sokol animations

    // bind pipeline & global uniforms
    sg_apply_pipeline(scene.AnimationSystem().sgPipeline);

    glm::vec2 cameraOrigin = scene.cameraOrigin;

    spdlog::debug("camera origin {}", cameraOrigin);

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

  for (const auto & animatorPair : scene.AnimationSystem().animators) {
    ImGui::Separator();
    ImGui::Separator();
    pul::imgui::Text("animator '{}'", animatorPair.first);

    auto const & spritesheet = animatorPair.second->spritesheet;

    ImGui::Image(
      reinterpret_cast<void *>(spritesheet.Image().id)
    , ImVec2(spritesheet.width, spritesheet.height), ImVec2(0, 1), ImVec2(1, 0)
    );

    for (auto const & piecePair : animatorPair.second->pieces) {
      ImGui::Separator();
      pul::imgui::Text("piece '{}'", piecePair.first);

      for (auto const & statePair : piecePair.second.states) {
        pul::imgui::Text("\tstate '{}'", statePair.first);
        pul::imgui::Text("\tdelta time {} ms", statePair.second.msDeltaTime);
        for (auto const & component : statePair.second.components) {
          pul::imgui::Text("\t\t {}x{}", component.tileX, component.tileY);
        }
      }
    }
  }

  ImGui::End();
}

} // -- extern C
