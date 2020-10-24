#pragma once

#include <sokol/gfx.hpp>
#include <sokol/imgui.hpp>

struct GLFWwindow;

namespace pul::core { struct Config; }
struct sg_pass;

namespace pul::gfx {
  bool InitializeContext(pul::core::Config & config);
  int & DisplayWidth();
  int & DisplayHeight();
  GLFWwindow * DisplayWindow();

  void StartFrame(float deltaMs);
  void EndFrame();

  void Shutdown();

  sg_pass & ScenePass();
  sg_image & SceneImageColor();
}

#define PUL_SHADER(...) \
  "#version 330 core\n" \
  #__VA_ARGS__
