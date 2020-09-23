/* pulcher | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/log.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/enum.hpp>

#include <cxxopts.hpp>
#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

auto StartupOptions() -> cxxopts::Options {
  auto options = cxxopts::Options("core-of-pulcher", "2D shooter game");
  options.add_options()
    (
      "r,resolution", "window resolution (0x0 means display resolution)"
    , cxxopts::value<std::string>()->default_value("0x0")
    ) ("d,debug", "debug mode (mostly console printing)") (
      "h,help", "print usage"
    )
  ;

  return options;
}

auto CreateUserConfig(cxxopts::ParseResult const & userResults)
  -> pulcher::core::Config
{
  pulcher::core::Config config;

  std::string windowResolution;

  try {
    windowResolution = userResults["resolution"].as<std::string>();
    if (userResults["debug"].as<bool>()) {
      spdlog::set_level(spdlog::level::debug);
    }
  } catch (cxxopts::OptionException & parseException) {
    spdlog::critical("{}", parseException.what());
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

    auto userResults = options.parse(argc, argv);

    if (userResults.count("help")) {
      printf("%s\n", options.help().c_str());
      return 0;
    }

    userConfig = ::CreateUserConfig(userResults);
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

  plugins.map.Load(plugins, "base/map/test.json");

  pulcher::core::SceneBundle sceneBundle;

  plugins.entity.StartScene(plugins, sceneBundle);

  pulcher::physics::Queries physicsQueries;

  auto timePreviousFrameBegin = std::chrono::high_resolution_clock::now();

  while (!glfwWindowShouldClose(pulcher::gfx::DisplayWindow())) {

    auto timeFrameBegin = std::chrono::high_resolution_clock::now();
    float const deltaMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        timeFrameBegin - timePreviousFrameBegin
      ).count();

    glfwPollEvents();

    pulcher::controls::UpdateControls(
      pulcher::gfx::DisplayWindow()
    , pulcher::gfx::DisplayWidth()
    , pulcher::gfx::DisplayHeight()
    , sceneBundle.playerController
    );

    plugins.physics.ProcessPhysics(sceneBundle, physicsQueries);

    pulcher::gfx::StartFrame(deltaMs);

    if (ImGui::Button("Reload plugins")) {
      plugins.map.Shutdown();
      plugins.physics.ClearMapGeometry();
      pulcher::plugin::UpdatePlugins(plugins);
      plugins.map.Load(plugins, "base/map/test.json");
    }

    plugins.userInterface.Dispatch();
    plugins.map.UiRender(sceneBundle);
    plugins.physics.UiRender(sceneBundle, physicsQueries);

    sg_pass_action passAction = {};
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = 0.2f;
    sg_begin_default_pass(
      &passAction
    , pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight()
    );

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
