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
}

#define PUL_SHADER(...) \
  "#version 330 core\n" \
  #__VA_ARGS__
