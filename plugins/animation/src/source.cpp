#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
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

  scene.animationSystem = {};

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
    scene.animationSystem.animators[animator->label] = animator;
  }

  cJSON_Delete(playerDataJson);
}

PUL_PLUGIN_DECL void Shutdown(pulcher::core::SceneBundle & scene) {
  scene.animationSystem = {};
}

PUL_PLUGIN_DECL void RenderAnimations(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle &
) {
}

PUL_PLUGIN_DECL void UiRender(pulcher::core::SceneBundle & scene) {

  ImGui::Begin("Animation");

  for (const auto & animatorPair : scene.animationSystem.animators) {
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
