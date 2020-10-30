/* pulcher | aodq.net */

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

#include <chrono>
#include <thread>

namespace pul::plugin { struct Info; }

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

PUL_PLUGIN_DECL void Ui_UiDispatch(
  pul::plugin::Info const & plugin
, pul::core::SceneBundle & sceneBundle
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

  // TODO temporary hack just for me
  #ifdef __unix__
  static int32_t intentionalLatency = 1;
  #else
  static int32_t intentionalLatency = 0;
  #endif
  ImGui::SliderInt("intentional lag", &intentionalLatency, 0, 22);
  if (intentionalLatency > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(intentionalLatency));
  }

  ImGui::Text(
    "%.2f ms/frame (%.0f FPS)"
  , static_cast<double>(1000.0f/io.Framerate)
  , static_cast<double>(io.Framerate)
  );

  ImGui::Text("ui pl test");

  ImGui::End();

  ImGui::Begin("Controls");

    auto & current = sceneBundle.PlayerController().current;
    using Movement = pul::controls::Controller::Movement;

    // render keys in this format:
    /*
          ^    | jump
        <   >  | dash
          v    | crouch

            <0.32, 0.83>
    */

    ImGui::Columns(2);

    ::RenderKey(current.movementVertical == Movement::Up , "  ^", "   ");

    ::RenderKey(current.movementHorizontal == Movement::Left , "<", " ");
    ImGui::SameLine();
    ::RenderKey(current.movementHorizontal == Movement::Right , "  >", "   ");

    ::RenderKey(current.movementVertical == Movement::Down , "  v", "   ");

    ImGui::NextColumn();

    ::RenderKey(current.jump, "jump", "");
    ::RenderKey(current.dash, "dash", "");
    ::RenderKey(current.crouch, "crouch", "");
    ::RenderKey(current.walk, "walk", "");

    ImGui::Columns(1);

  ImGui::End();

  plugin.animation.UiRender(plugin, sceneBundle);
  plugin.audio.UiRender(plugin, sceneBundle);
  plugin.entity.UiRender(sceneBundle);
  plugin.map.UiRender(sceneBundle);
  plugin.physics.UiRender(sceneBundle);
  simgui_render();
}

}
