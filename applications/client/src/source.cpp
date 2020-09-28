/* pulcher | aodq.net */

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/config.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <argparse/argparse.hpp>
#pragma GCC diagnostic pop

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

auto StartupOptions() -> argparse::ArgumentParser {
  auto options = argparse::ArgumentParser("pulcher-client", "0.0.1");
  options
    .add_argument("-r")
    .help(("window resolution (0x0 means display resolution)"))
    .default_value(std::string{"0x0"})
  ;

  options
    .add_argument("-d")
    .help("debug mode (console printing)")
    .default_value(false)
    .implicit_value(true)
  ;

  return options;
}

auto CreateUserConfig(argparse::ArgumentParser const & userResults)
  -> pulcher::core::Config
{
  pulcher::core::Config config;

  std::string windowResolution;

  try {
    windowResolution = userResults.get<std::string>("-r");
    if (userResults.get<bool>("-d")) {
      spdlog::set_level(spdlog::level::debug);
    }
  } catch (const std::runtime_error & err) {
    spdlog::critical("{}", err.what());
  }

  // -- window resolution
  if (
      auto xLen = windowResolution.find("x");
      xLen != std::string::npos && xLen < windowResolution.size()-1ul
  ) {
    config.windowWidth =
      static_cast<uint16_t>(std::stoi(windowResolution.substr(0ul, xLen)));
    config.windowHeight =
      static_cast<uint16_t>(
        std::stoi(windowResolution.substr(xLen+1ul, windowResolution.size()))
      );
  } else {
    spdlog::error("window resolution incorrect format, must use WxH");
  }

  return config;
}

void PrintUserConfig(pulcher::core::Config const & config) {
  spdlog::info(
    "window dimensions {}x{}", config.windowWidth, config.windowHeight
  );
}

static void ImguiApplyColor()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.TabRounding = 0.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarRounding = 0.0f;
    style.ScrollbarSize = 10.0f;
    style.WindowMenuButtonPosition = ImGuiDir_Right;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.12f, 0.13f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.04f, 0.05f, 0.05f, 0.50f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.12f, 0.13f, 0.13f, 0.94f);
    colors[ImGuiCol_Border]                = ImVec4(0.25f, 0.26f, 0.28f, 0.50f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.01f, 0.01f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.21f, 0.23f, 0.50f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.26f, 0.28f, 0.75f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.30f, 0.31f, 0.34f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.04f, 0.05f, 0.05f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.18f, 0.19f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.24f, 0.25f, 0.27f, 0.75f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.41f, 0.42f, 0.42f, 0.75f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.62f, 0.63f, 0.63f, 0.75f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.94f, 0.93f, 0.95f, 0.75f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.60f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.41f, 0.42f, 0.42f, 0.75f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.62f, 0.63f, 0.63f, 0.75f);
    colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.21f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.41f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.19f, 0.11f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.41f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.25f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.41f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.62f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.31f, 0.34f, 0.75f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.41f, 0.42f, 0.42f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.62f, 0.63f, 0.63f, 0.75f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.21f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.37f, 0.38f, 0.31f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.30f, 0.31f, 0.34f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.12f, 0.13f, 0.13f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.19f, 0.11f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.51f, 0.99f, 0.50f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.20f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.62f, 0.62f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.44f, 0.36f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.71f, 0.01f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.61f, 0.01f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.51f, 0.99f, 0.50f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.01f, 0.01f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.51f, 0.99f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.01f, 1.01f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.81f, 0.81f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.81f, 0.81f, 0.35f);
}

void LoadPluginInfo(
  pulcher::plugin::Info & plugin, pulcher::core::SceneBundle & scene
) {
  plugin.map.Load(plugin, "base/map/test.json");
  plugin.animation.LoadAnimations(plugin, scene);

  // last thing so that all the previous information (maps, animation, etc)
  // can be loaded up. They can still modify the registry if they need tho.
  plugin.entity.StartScene(plugin, scene);
}

void ShutdownPluginInfo(
  pulcher::plugin::Info & plugin, pulcher::core::SceneBundle & scene
) {
  plugin.animation.Shutdown(scene);
  plugin.map.Shutdown();
  plugin.physics.ClearMapGeometry();

  // entity has to shut down last from plugins to allow entities in the
  // registry to be deallocated
  plugin.entity.Shutdown(scene);
}

} // -- anon namespace

int main(int argc, char const ** argv) {

  spdlog::set_pattern("%^%M:%S |%$ %v");

  pulcher::core::Config userConfig;

  { // -- collect user options
    auto options = ::StartupOptions();

    options.parse_args(argc, argv);

    userConfig = ::CreateUserConfig(options);
  }

  #ifdef __unix__
    spdlog::info("-- running on Linux platform --");
  #elif _WIN64
    spdlog::info("-- running on Windows 64 platform --");
  #elif _WIN32
    spdlog::info("-- running on Windows 32 platform --");
  #endif

  spdlog::info("initializing pulcher");
  // -- initialize relevant components
  pulcher::gfx::InitializeContext(userConfig);
  pulcher::plugin::Info plugins;

  pulcher::plugin::LoadPlugin(
    plugins, pulcher::plugin::Type::UserInterface
  , "plugins/ui-base.pulcher-plugin"
  );

  pulcher::plugin::LoadPlugin(
    plugins, pulcher::plugin::Type::Map
  , "plugins/plugin-map.pulcher-plugin"
  );

  pulcher::plugin::LoadPlugin(
    plugins, pulcher::plugin::Type::Physics
  , "plugins/plugin-physics.pulcher-plugin"
  );

  pulcher::plugin::LoadPlugin(
    plugins, pulcher::plugin::Type::Entity
  , "plugins/plugin-entity.pulcher-plugin"
  );

  pulcher::plugin::LoadPlugin(
    plugins, pulcher::plugin::Type::Animation
  , "plugins/plugin-animation.pulcher-plugin"
  );

  ::PrintUserConfig(userConfig);

  pulcher::core::SceneBundle sceneBundle;
  ::LoadPluginInfo(plugins, sceneBundle);

  ImguiApplyColor();

  auto timePreviousFrameBegin = std::chrono::high_resolution_clock::now();

  while (!glfwWindowShouldClose(pulcher::gfx::DisplayWindow())) {

    auto timeFrameBegin = std::chrono::high_resolution_clock::now();
    float const deltaMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        timeFrameBegin - timePreviousFrameBegin
      ).count();

    glfwPollEvents();

    // -- logic
    pulcher::controls::UpdateControls(
      pulcher::gfx::DisplayWindow()
    , pulcher::gfx::DisplayWidth()
    , pulcher::gfx::DisplayHeight()
    , sceneBundle.PlayerController()
    );
    plugins.physics.ProcessPhysics(sceneBundle);
    plugins.entity.EntityUpdate(plugins, sceneBundle);

    // -- rendering
    pulcher::gfx::StartFrame(deltaMs);

    ImGui::Begin("Diagnostics");
    if (ImGui::Button("Reload plugins")) {

      ::ShutdownPluginInfo(plugins, sceneBundle);
      pulcher::plugin::UpdatePlugins(plugins);
      ::LoadPluginInfo(plugins, sceneBundle);
    }
    ImGui::End();

    sg_pass_action passAction = {};
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = 0.2f;
    sg_begin_default_pass(
      &passAction
    , pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight()
    );

    plugins.userInterface.UiDispatch(plugins, sceneBundle);

    plugins.map.Render(sceneBundle);
    plugins.animation.RenderAnimations(plugins, sceneBundle);

    simgui_render();
    sg_end_pass();
    sg_commit();

    pulcher::gfx::EndFrame();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    timePreviousFrameBegin = timeFrameBegin;
  }

  ::ShutdownPluginInfo(plugins, sceneBundle);

  // has to be last thing to shut down to allow gl deallocation calls
  pulcher::gfx::Shutdown();

  return 0;
}
