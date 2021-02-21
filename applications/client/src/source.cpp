/* pulcher | aodq.net */

#include <pulcher-audio/system.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/config.hpp>
#include <pulcher-core/player.hpp>
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
    .add_argument("-m")
    .help(("map path"))
    .default_value(std::string{"assets/base/map/calamity/map-calamity.json"})
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
  -> pul::core::Config
{
  pul::core::Config config;

  std::string windowResolution;
  std::string framebufferResolution;

  try {
    windowResolution = userResults.get<std::string>("-w");
    framebufferResolution = userResults.get<std::string>("-r");
    config.mapPath = std::filesystem::path{userResults.get<std::string>("-m")};
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
    config.framebufferDim.x =
      static_cast<uint16_t>(std::stoi(framebufferResolution.substr(0ul, xLen)));
    config.framebufferDim.y =
      static_cast<uint16_t>(
        std::stoi(
          framebufferResolution.substr(xLen+1ul, framebufferResolution.size())
        )
      );
    config.framebufferDimFloat = glm::vec2(config.framebufferDim);
  } else {
    spdlog::error("framebuffer resolution incorrect format, must use WxH");
  }

  return config;
}

void PrintUserConfig(pul::core::Config const & config) {
  spdlog::info(
    "window dimensions {}x{}", config.windowWidth, config.windowHeight
  );
  spdlog::info("framebuffer dimensions {}" , config.framebufferDim);
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

// this gets capped at pul::util::MsPerFrame
void ProcessLogic(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
) {

  // clear debug physics queries
  auto & queries = scene.PhysicsDebugQueries();
  queries.intersectorRays.clear();
  queries.intersectorPoints.clear();

  auto & imguiIo = ImGui::GetIO();

  pul::controls::UpdateControls(
    pul::gfx::DisplayWindow()
  , scene.playerCenter.x
  , scene.playerCenter.y
  , scene.PlayerController()
  , false
  , scene.debugFrameBufferHovered ? false : imguiIo.WantCaptureMouse
  );

  plugin.LogicUpdate(scene);
}

// this has no framerate cap, but it most provide a minimal of 90 framerate
void ProcessRendering(
  pul::plugin::Info & plugin
, pul::core::SceneBundle & scene
, pul::core::RenderBundle & renderBundle
, pul::core::RenderBundleInstance const & renderInterp
, float const deltaMs
, size_t const numCpuFrames
) {
  pul::gfx::StartFrame(deltaMs);

  static glm::vec3 screenClearColor = glm::vec3(0.7f, 0.4f, .4f);

  { // -- render scene
    sg_pass_action passAction = {};
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = screenClearColor.r;
    passAction.colors[0].val[1] = screenClearColor.g;
    passAction.colors[0].val[2] = screenClearColor.b;
    passAction.colors[0].val[3] = 0.0f;
    passAction.depth.action = SG_ACTION_CLEAR;
    passAction.depth.val = 1.0f;

    sg_begin_pass(pul::gfx::ScenePass(), &passAction);

    plugin.RenderInterpolated(scene, renderInterp);

    sg_end_pass();
  }

  { // -- render UI
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(
      ImVec2(pul::gfx::DisplayWidth(), pul::gfx::DisplayHeight())
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
      plugin.Shutdown(scene);

      // reload configs
      scene.PlayerMetaInfo() = {};

      renderBundle = {};

      // continue loading plugins
      pul::plugin::UpdatePlugins(plugin);
      plugin.Initialize(scene);

      renderBundle = pul::core::RenderBundle::Construct(plugin, scene);

      pul::controls::LoadControllerConfig(
        pul::gfx::DisplayWindow()
      , scene.PlayerController()
      );
    }
    pul::imgui::ItemTooltip(
      "NOTE: RELOADING plugins will save animations, configs, etc"
    );
    ImGui::SameLine();
    if (ImGui::Button("reset ms/frame")) {
      scene.calculatedMsPerFrame = pul::util::MsPerFrame;
    }
    ImGui::SliderFloat(
      "ms / frame", &scene.calculatedMsPerFrame
    , 1.0f, 1000.0f/0.9f
    , "%.3f", 4.0f
    );
    ImGui::ColorEdit3("screen clear", &screenClearColor.x);
    pul::imgui::Text("CPU frames {}", numCpuFrames);

    ImGui::Checkbox(
      "interpolate rendering {}", &renderBundle.debugUseInterpolation
    );

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

    ImGui::Begin("Update Check");
    if (applyGitUpdate && updateReady) {

      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.5, 0.5, 1));
      pul::imgui::Text("There is an update ready to load now !");
      ImGui::PopStyleColor();

      // updating files while they're open only works on linux, not Win64
      #if defined(__unix__) || defined(__APPLE__)
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
    }
    ImGui::End();

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
    , pul::gfx::DisplayWidth(), pul::gfx::DisplayHeight()
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
        imageCenter.x += scene.config.framebufferDim.x*0.5f;
        imageCenter.y += scene.config.framebufferDim.y*0.5f;
        imageCenter.y -= 22.0f; // player center
        scene.playerCenter = glm::u32vec2(imageCenter.x, imageCenter.y);
      }

      ImGui::Image(
        reinterpret_cast<void *>(pul::gfx::SceneImageColor().id)
      , ImVec2(scene.config.framebufferDim.x, scene.config.framebufferDim.y)
      , ImVec2(0, 1)
      , ImVec2(1, 0)
      , ImVec4(1, 1, 1, 1)
      , ImVec4(1, 1, 1, 1)
      );

      scene.debugFrameBufferHovered = ImGui::IsItemHovered();

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
          texW = scene.config.framebufferDim.x
        , texH = scene.config.framebufferDim.y
        ;

        regionX = glm::clamp(regionX, 0.0f, texW - regionSize);
        regionY = glm::clamp(regionY, 0.0f, texH - regionSize);

        ImGui::Image(
          reinterpret_cast<void *>(pul::gfx::SceneImageColor().id)
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

    // -- debug ui
    plugin.DebugUiDispatch(scene);

    simgui_render();

    sg_end_pass();
  }

  sg_commit();

  pul::gfx::EndFrame();
}

pul::plugin::Info InitializePlugins() {
  pul::plugin::Info plugins;

  pul::plugin::LoadPlugin(plugins , "plugins/plugin-base.pulcher-plugin");

  return plugins;
}

} // -- anon namespace

int main(int argc, char const ** argv) {

  spdlog::set_pattern("%^%M:%S |%$ %v");

  pul::core::Config userConfig;

  { // -- collect user options
    auto options = ::StartupOptions();

    options.parse_args(argc, argv);

    userConfig = ::CreateUserConfig(options);
  }

  #ifdef __unix__
    spdlog::info("-- running on Linux platform --");
  #elif defined(__APPLE__)
    spdlog::info("-- running on macOS platform --");
  #elif _WIN64
    spdlog::info("-- running on Windows 64 platform --");
  #elif _WIN32
    spdlog::info("-- running on Windows 32 platform --");
  #endif

  spdlog::info("initializing pulcher");
  // -- initialize relevant components
  pul::gfx::InitializeContext(userConfig);
  ::PrintUserConfig(userConfig);

  pul::plugin::Info plugin = InitializePlugins();

  pul::core::SceneBundle sceneBundle;
  sceneBundle.config = userConfig;
  pul::controls::LoadControllerConfig(
    pul::gfx::DisplayWindow()
  , sceneBundle.PlayerController()
  );

  plugin.Initialize(sceneBundle);

  auto renderBundle = pul::core::RenderBundle::Construct(plugin, sceneBundle);

  ImGuiApplyStyling();

  auto timePreviousFrameBegin = std::chrono::high_resolution_clock::now();

  float msToCalculate = 0.0f;

  while (!glfwWindowShouldClose(pul::gfx::DisplayWindow())) {
    // -- get timing
    auto timeFrameBegin = std::chrono::high_resolution_clock::now();
    float const deltaMs =
      std::chrono::duration_cast<std::chrono::microseconds>(
        timeFrameBegin - timePreviousFrameBegin
      ).count() / 1000.0f;

    // if above a certain range then just dismiss this time interval
    if (deltaMs < 100.0f) {
      // -- update windowing events
      glfwPollEvents();

      // -- logic, ~62 Hz
      msToCalculate += deltaMs;
      size_t calculatedFrames = 0ul;
      while (msToCalculate >= sceneBundle.calculatedMsPerFrame) {
        // -- process logic
        ++ calculatedFrames;
        msToCalculate -= sceneBundle.calculatedMsPerFrame;
        ::ProcessLogic(plugin, sceneBundle);

        // -- update render bundle
        renderBundle.Update(plugin, sceneBundle);
      }

      sceneBundle.numCpuFrames = calculatedFrames;

      // -- rendering interpolation
      auto const msDeltaInterp =
        msToCalculate / sceneBundle.calculatedMsPerFrame
      ;
      auto renderBundleInterp = renderBundle.Interpolate(plugin, msDeltaInterp);

      // -- rendering, unlimited Hz
      ::ProcessRendering(
        plugin, sceneBundle
      , renderBundle, renderBundleInterp
      , deltaMs
      , calculatedFrames
      );

      // -- audio, unlimited Hz
      sceneBundle.AudioSystem().Update(sceneBundle);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(0));

    timePreviousFrameBegin = timeFrameBegin;
  }

  plugin.Shutdown(sceneBundle);

  // has to be last thing to shut down to allow gl deallocation calls
  pul::gfx::Shutdown();

  return 0;
}
