/* pulcher | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <cxxopts.hpp>
#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>
#include <imgui/imgui_impl_opengl3.hpp>

#include <string>
#include <utility>
#include <vector>

namespace {

auto StartupOptions() -> cxxopts::Options {
  auto options = cxxopts::Options("core-of-pulcher", "2D shooter game");
  options.add_options()
    (
      "i,ip-address", "ip address to connect to"
    , cxxopts::value<std::string>()->default_value("69.243.92.93")
    ) (
      "r,resolution", "window resolution (0x0 means display resolution)"
    , cxxopts::value<std::string>()->default_value("0x0")
    ) (
      "p,port", "port number to connect to"
    , cxxopts::value<uint16_t>()->default_value("6599")
    ) (
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
    config.networkIpAddress   = userResults["ip-address"].as<std::string>();
    config.networkPortAddress = userResults["port"].as<uint16_t>();
    windowResolution = userResults["resolution"].as<std::string>();
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
  if (config.networkIpAddress != "") {
    spdlog::info(
      "ip address '{}:{}'"
    , config.networkIpAddress, config.networkPortAddress
    );
  }

  spdlog::info(
    "window dimensions {}x{}", config.windowWidth, config.windowHeight
  );
}

void InitializeImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO & io = ImGui::GetIO();
  io.ConfigFlags =
    0
  | ImGuiConfigFlags_DockingEnable
  ;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(pulcher::gfx::DisplayWindow(), true);
  ImGui_ImplOpenGL3_Init("#version 330 core");
}

} // -- anon namespace

int main(int argc, char const ** argv) {
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
  ::InitializeImGui();
  pulcher::plugin::Info plugin;

  pulcher::plugin::LoadPlugin(
    plugin, pulcher::plugin::Type::UserInterface
  , "ui-base.pulcher-plugin"
  );

  ::PrintUserConfig(userConfig);

  while (!glfwWindowShouldClose(pulcher::gfx::DisplayWindow())) {

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Test");
    if (ImGui::Button("Reload plugins")) {
      pulcher::plugin::UpdatePlugins(plugin);
    }
    ImGui::End();

    plugin.userInterface.Dispatch();

    ImGui::Render();

    // -- validate display size in case of resize
    glfwGetFramebufferSize(
      pulcher::gfx::DisplayWindow()
    , &pulcher::gfx::DisplayWidth()
    , &pulcher::gfx::DisplayHeight()
    );
    glViewport(
      0, 0, pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight()
    );

    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(pulcher::gfx::DisplayWindow());
    }

    glfwSwapBuffers(pulcher::gfx::DisplayWindow());
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(pulcher::gfx::DisplayWindow());
  glfwTerminate();

  return 0;
}
