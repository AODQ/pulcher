
#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>

#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

extern "C" {

void Dispatch(
) {
  ImGui::Begin("Diagnostics");

  ImGuiIO & io = ImGui::GetIO();

  ImGui::Text(
    "imgui %.2f ms/frame (%.0f FPS)", 1000.0f/io.Framerate, io.Framerate
  );
  ImGui::End();
}

}
