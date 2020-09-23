/* pulcher | aodq.net */

#include <pulcher-core/log.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

namespace pulcher::plugin { struct Info; }

namespace {

void RenderKey(
  bool const current
, char const * const on, char const * const /*off*/
) {
  if (!current) {
    ImGui::TextColored(ImVec4(0.6, 0.4, 0.4, 1.0), "%s", on);
    return;
  }
  ImGui::Text("%s", on);
}

} // -- namespace

extern "C" {

void UiDispatch(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & sceneBundle
) {
  ImGui::Begin("Diagnostics");

  ImGuiIO & io = ImGui::GetIO();
  #ifdef __unix__
    ImGui::Text("Platform: Linux");
  #elif _WIN64
    ImGui::Text("Platform: Windows64");
  #elif _WIN32
    ImGui::Text("Platform: Windows32");
  #endif

  ImGui::Text(
    "%.2f ms/frame (%.0f FPS)"
  , static_cast<double>(1000.0f/io.Framerate)
  , static_cast<double>(io.Framerate)
  );

  ImGui::Text("ui pl test");

  ImGui::End();

  ImGui::Begin("Controls");

    auto & current = sceneBundle.playerController.current;
    using Movement = pulcher::controls::Controller::Movement;

    // render keys in this format:
    /*
          ^    _jump_
        <   >  >dash>
          v    VcrouchV

            <0.32, 0.83>
    */

    // up
    ::RenderKey(
      current.movementVertical == Movement::Up
    , "  ^", "   "
    );

    // jump
    ImGui::SameLine();
    ::RenderKey(current.jump, "   _jump_", "");

    // left
    ::RenderKey(
      current.movementHorizontal == Movement::Left
    , "<", " "
    );

    // right
    ImGui::SameLine();
    ::RenderKey(
      current.movementHorizontal == Movement::Right
    , "  >", "   "
    );

    // dash
    ImGui::SameLine();
    ::RenderKey(current.dash, " >dash>", "");

    // down
    ::RenderKey(
      current.movementVertical == Movement::Down
    , "  v", "   "
    );

    // crouch
    ImGui::SameLine();
    ::RenderKey(current.crouch, "   VcrouchV", "");

  ImGui::End();
}

}
