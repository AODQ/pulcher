#pragma once

#include <sokol/gfx.hpp>
#include <sokol/imgui.hpp>

struct GLFWwindow;

namespace pulcher::core { struct Config; }

namespace pulcher::gfx {
  bool InitializeContext(pulcher::core::Config & config);
  int & DisplayWidth();
  int & DisplayHeight();
  GLFWwindow * DisplayWindow();

  void StartFrame(float deltaMs);
  void EndFrame();

  void Shutdown();

  // TODO this needs to be moved elsewhere
  bool LeftMousePressed();
  uint32_t MouseX();
  uint32_t MouseY();
}
