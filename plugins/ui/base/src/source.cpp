#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

extern "C" {

void Dispatch(
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
  ImGui::End();
}

}
