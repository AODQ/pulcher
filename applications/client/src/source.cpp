/* pulcher | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <argparse/argparse.hpp>
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
    .default_value(true)
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

  ::PrintUserConfig(userConfig);

  pulcher::core::SceneBundle sceneBundle;

  plugins.map.Load(plugins, "base/map/test.json");
  plugins.entity.StartScene(plugins, sceneBundle);

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
    , sceneBundle.playerController
    );
    plugins.physics.ProcessPhysics(sceneBundle);
    plugins.entity.EntityUpdate(plugins, sceneBundle);

    // -- rendering
    pulcher::gfx::StartFrame(deltaMs);

    ImGui::Begin("Diagnostics");
    if (ImGui::Button("Reload plugins")) {
      plugins.map.Shutdown();
      plugins.physics.ClearMapGeometry();
      plugins.entity.Shutdown();
      pulcher::plugin::UpdatePlugins(plugins);
      plugins.map.Load(plugins, "base/map/test.json");
      plugins.entity.StartScene(plugins, sceneBundle);
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
    plugins.map.UiRender(sceneBundle);
    plugins.entity.UiRender(sceneBundle);
    plugins.physics.UiRender(sceneBundle);

    plugins.map.Render(sceneBundle);

    simgui_render();
    sg_end_pass();
    sg_commit();

    pulcher::gfx::EndFrame();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    timePreviousFrameBegin = timeFrameBegin;
  }

  plugins.map.Shutdown();

  pulcher::gfx::Shutdown();

  return 0;
}
