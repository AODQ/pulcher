/* pulcher | aodq.net */

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/config.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <argparse/argparse.hpp>
#pragma GCC diagnostic pop

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>
#include <process.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

bool applyGitUpdate = true;

auto StartupOptions() -> argparse::ArgumentParser {
  auto options = argparse::ArgumentParser("pulcher-client", "0.0.1");
  options
    .add_argument("-w")
    .help(("window resolution (0x0 means display resolution)"))
    .default_value(std::string{"0x0"})
  ;

  options
    .add_argument("-r")
    .help(("framebuffer resolution"))
    .default_value(std::string{"960x720"})
  ;

  options
    .add_argument("-d")
    .help("debug mode (console printing)")
    .default_value(false)
    .implicit_value(true)
  ;

  options
    .add_argument("-g")
    .help("do not automatically check for git updates")
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
  std::string framebufferResolution;

  try {
    windowResolution = userResults.get<std::string>("-w");
    framebufferResolution = userResults.get<std::string>("-r");
    if (userResults.get<bool>("-d")) {
      spdlog::set_level(spdlog::level::debug);
    }
    if (userResults.get<bool>("-g")) {
      ::applyGitUpdate = false;
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

  if (
      auto xLen = framebufferResolution.find("x");
      xLen != std::string::npos && xLen < framebufferResolution.size()-1ul
  ) {
    config.framebufferWidth =
      static_cast<uint16_t>(std::stoi(framebufferResolution.substr(0ul, xLen)));
    config.framebufferHeight =
      static_cast<uint16_t>(
        std::stoi(
          framebufferResolution.substr(xLen+1ul, framebufferResolution.size())
        )
      );
  } else {
    spdlog::error("framebuffer resolution incorrect format, must use WxH");
  }

  return config;
}

void PrintUserConfig(pulcher::core::Config const & config) {
  spdlog::info(
    "window dimensions {}x{}", config.windowWidth, config.windowHeight
  );
  spdlog::info(
    "framebuffer dimensions {}x{}"
  , config.framebufferWidth, config.framebufferHeight
  );
}

static void ImGuiApplyStyling()
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
  plugin.map.Load(plugin, "assets/base/map/test.json");
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

// this gets capped at 90Hz
void ProcessLogic(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
) {

  // clear debug physics queries
  auto & queries = scene.PhysicsDebugQueries();
  queries.intersectorRays.clear();
  queries.intersectorPoints.clear();

  pulcher::controls::UpdateControls(
    pulcher::gfx::DisplayWindow()
  , scene.playerCenter.x
  , scene.playerCenter.y
  , scene.PlayerController()
  );
  plugin.entity.EntityUpdate(plugin, scene);
  plugin.animation.UpdateFrame(plugin, scene);
}

// this has no framerate cap, but it most provide a minimal of 90 framerate
void ProcessRendering(
  pulcher::plugin::Info & plugin, pulcher::core::SceneBundle & scene
, float deltaMs
, size_t numCpuFrames
) {
  pulcher::gfx::StartFrame(deltaMs);

  static glm::vec3 screenClearColor = glm::vec3(0.7f, 0.4f, .4f);

  { // -- render scene
    sg_pass_action passAction = {};
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = screenClearColor.r;
    passAction.colors[0].val[1] = screenClearColor.g;
    passAction.colors[0].val[2] = screenClearColor.b;
    passAction.depth.action = SG_ACTION_CLEAR;
    passAction.depth.val = 1.0f;

    sg_begin_pass(pulcher::gfx::ScenePass(), &passAction);

    plugin.map.Render(scene);
    plugin.animation.RenderAnimations(plugin, scene);
    plugin.physics.RenderDebug(scene);

    sg_end_pass();
  }

  { // -- render UI
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(
      ImVec2(pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight())
    );
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(
      "background"
    , nullptr
    ,   ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_NoScrollWithMouse
      | ImGuiWindowFlags_NoBringToFrontOnFocus
      | ImGuiWindowFlags_NoCollapse
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_NoMove
      | ImGuiWindowFlags_NoNavFocus
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoDocking
    );

    static auto const dockId = ImGui::GetID("DockBackground");
    ImGui::DockSpace(
      dockId
    , ImVec2(0.0f, 0.0f)
    , ImGuiDockNodeFlags_PassthruCentralNode
    );
    ImGui::PopStyleVar(3);

    ImGui::Begin("Diagnostics");
    if (ImGui::Button("Reload plugins")) {
      ::ShutdownPluginInfo(plugin, scene);
      pulcher::plugin::UpdatePlugins(plugin);
      ::LoadPluginInfo(plugin, scene);
    }
    pul::imgui::ItemTooltip(
      "WARNING: RELOADING plugins will not save anything!\n"
      "progress (like editing animations) will be lost"
    );
    ImGui::SliderFloat(
      "ms / frame", &scene.calculatedMsPerFrame
    , 1000.0f/90.0f, 1000.0f/0.9f
    , "%.3f", 4.0f
    );
    ImGui::ColorEdit3("screen clear", &screenClearColor.x);
    pul::imgui::Text("CPU frames {}", numCpuFrames);

    ImGui::End();

    // check for update every 10s
    static bool updateReady = false;
    static std::string updateDetails = {};
    static bool hasGitCheckThreadInit = false;
    if (applyGitUpdate && !hasGitCheckThreadInit) {
      hasGitCheckThreadInit = true;
      std::thread gitCheckThread = std::thread([&]() {
        while (!updateReady) {
          auto remoteProc = TinyProcessLib::Process("git remote update");
          remoteProc.get_exit_status();

          std::string output = {};

          auto checkProc =
            TinyProcessLib::Process(
              "git status -uno", "",
              [&](char const * bytes, size_t n) {
                for (size_t i = 0ul; i < n; ++ i) { output += bytes[i]; }
              },
              [&](char const * bytes, size_t n) {
                for (size_t i = 0ul; i < n; ++ i) { output += bytes[i]; }
              },
              true
            );

          checkProc.get_exit_status();

          // on fatal error just stop trying to check for updates
          if (output.find("fatal:") != std::string::npos) {
            break;
          }

          if (
              output.find("Your branch is behind") != std::string::npos
           || output.find("HEAD detached at") != std::string::npos
          ) {
            output += "\n\n -- revision log --\n\n";

            auto logProc =
              TinyProcessLib::Process(
                "git log ..@{u}", "",
                [&](char const * bytes, size_t n) {
                  for (size_t i = 0ul; i < n; ++ i) { output += bytes[i]; }
                },
                [&](char const * bytes, size_t n) {
                  for (size_t i = 0ul; i < n; ++ i) { output += bytes[i]; }
                },
                true
              );
            logProc.get_exit_status();

            updateReady = true;
            updateDetails = output;
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
      });
      gitCheckThread.detach();
    }

    if (applyGitUpdate && updateReady) {
      ImGui::Begin("UPDATE");

      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.5, 0.5, 1));
      pul::imgui::Text("There is an update ready to load now !");
      ImGui::PopStyleColor();

      // updating files while they're open only works on linux, not Win64
      #ifdef __unix__
        if (ImGui::Button("Click to update")) {
          {
            auto proc = TinyProcessLib::Process("git stash");
            proc.get_exit_status();
          }
          {
            auto proc = TinyProcessLib::Process("git pull --ff-only");
            proc.get_exit_status();
          }

          spdlog::info("Closing pulcher");
          // TODO exit normally

          exit(0);
        }

        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          pul::imgui::Text(
            "THIS WILL CLOSE PULCHER, SAVE BEFORE CLICKING!"
          );
          ImGui::EndTooltip();
        }
      #endif

      pul::imgui::Text("Details: {}", updateDetails);
      ImGui::End();
    }

    sg_pass_action passAction = {};
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = screenClearColor.r;
    passAction.colors[0].val[1] = screenClearColor.g;
    passAction.colors[0].val[2] = screenClearColor.b;
    passAction.colors[0].val[3] = 1.0f;
    passAction.depth.action = SG_ACTION_CLEAR;
    passAction.depth.val = 1.0f;

    sg_begin_default_pass(
      &passAction
    , pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight()
    );


    ImGui::PushStyleColor(
      ImGuiCol_ChildBg
    , ImVec4(screenClearColor.r, screenClearColor.g, screenClearColor.b, 1.0f)
    );
    ImGui::Begin("scene");
      static bool zoomImage = false;
      static bool zoomOriginSet = false;
      static bool zoomOriginSetting = false;
      static glm::vec2 zoomOrigin = {};

      { // set screen center
        ImVec2 imageCenter = ImGui::GetCursorScreenPos();
        imageCenter.x += scene.config.framebufferWidth*0.5f;
        imageCenter.y += scene.config.framebufferHeight*0.5f;
        imageCenter.y -= 32.0f; // player center
        scene.playerCenter = glm::u32vec2(imageCenter.x, imageCenter.y);
      }

      ImGui::Image(
        reinterpret_cast<void *>(pulcher::gfx::SceneImageColor().id)
      , ImVec2(scene.config.framebufferWidth, scene.config.framebufferHeight)
      , ImVec2(0, 1)
      , ImVec2(1, 0)
      , ImVec4(1, 1, 1, 1)
      , ImVec4(1, 1, 1, 1)
      );

      if (zoomImage) {
        ImGuiIO & io = ImGui::GetIO();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 mousePos = io.MousePos;

        if (zoomOriginSetting && ImGui::IsItemClicked()) {
          zoomOriginSet = true;
          zoomOriginSetting = false;
          zoomOrigin = glm::vec2(mousePos.x, mousePos.y);
        }

        if (zoomOriginSet) { mousePos = ImVec2(zoomOrigin.x, zoomOrigin.y); }

        ImGui::SameLine();
        float
          regionSize = 64.0f
        , regionX = glm::abs(mousePos.x - pos.x - regionSize*0.5f)
        , regionY = glm::abs(mousePos.y - pos.y + regionSize*0.5f)
        , zoom = 4.0f
        ;

        float const
          texW = scene.config.framebufferWidth
        , texH = scene.config.framebufferHeight
        ;

        regionX = glm::clamp(regionX, 0.0f, texW - regionSize);
        regionY = glm::clamp(regionY, 0.0f, texH - regionSize);

        ImGui::Image(
          reinterpret_cast<void *>(pulcher::gfx::SceneImageColor().id)
        , ImVec2(regionSize * zoom, regionSize * zoom)
        , ImVec2(regionX / texW, (regionY + regionSize) / texH)
        , ImVec2((regionX + regionSize) / texW, regionY / texH)
        , ImVec4(1, 1, 1, 1)
        , ImVec4(1, 1, 1, 0.5f)
        );

        pul::imgui::Text("Min: ({:.2f}, {:.2f})", regionX, regionY);
        pul::imgui::Text(
          "Max: ({:.2f}, {:.2f})", regionX + regionSize, regionY + regionSize
        );
      }

      ImGui::Checkbox("zoom image", &zoomImage);

      if (zoomImage) {
        if (!zoomOriginSetting && ImGui::Button("set image zoom origin"))
          { zoomOriginSetting = true; }

        if (zoomOriginSetting)
          { ImGui::Text("click image to set zoom origin"); }

        ImGui::Checkbox("zoom origin set", &zoomOriginSet);
      }

    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::End();

    plugin.userInterface.UiDispatch(plugin, scene);
    simgui_render();

    sg_end_pass();
  }

  sg_commit();

  pulcher::gfx::EndFrame();
}

pulcher::plugin::Info InitializePlugins() {
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

  return plugins;
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
  ::PrintUserConfig(userConfig);

  pulcher::plugin::Info plugin = InitializePlugins();

  pulcher::core::SceneBundle sceneBundle;
  sceneBundle.config = userConfig;
  ::LoadPluginInfo(plugin, sceneBundle);

  ImGuiApplyStyling();

  auto timePreviousFrameBegin = std::chrono::high_resolution_clock::now();

  float msToCalculate = 0.0f;

  while (!glfwWindowShouldClose(pulcher::gfx::DisplayWindow())) {
    // -- get timing
    auto timeFrameBegin = std::chrono::high_resolution_clock::now();
    float const deltaMs =
      std::chrono::duration_cast<std::chrono::microseconds>(
        timeFrameBegin - timePreviousFrameBegin
      ).count() / 1000.0f;

    // -- update windowing events
    glfwPollEvents();

    // -- logic, 90 Hz
    msToCalculate += deltaMs;
    size_t calculatedFrames = 0ul;
    while (msToCalculate >= sceneBundle.calculatedMsPerFrame) {
      ++ calculatedFrames;
      msToCalculate -= sceneBundle.calculatedMsPerFrame;
      ::ProcessLogic(plugin, sceneBundle);
    }

    sceneBundle.numCpuFrames = calculatedFrames;

    // -- rendering, unlimited Hz
    ::ProcessRendering(plugin, sceneBundle, deltaMs, calculatedFrames);

    std::this_thread::sleep_for(std::chrono::milliseconds(0));

    timePreviousFrameBegin = timeFrameBegin;
  }

  ::ShutdownPluginInfo(plugin, sceneBundle);

  // has to be last thing to shut down to allow gl deallocation calls
  pulcher::gfx::Shutdown();

  return 0;
}
