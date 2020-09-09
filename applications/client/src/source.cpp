/* pulcher | aodq.net */

#include <pulcher-core/config.hpp>
#include <pulcher-core/log.hpp>
#include <pulcher-gfx/context.hpp>
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
  spdlog::info(
    "window dimensions {}x{}", config.windowWidth, config.windowHeight
  );
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
  pulcher::plugin::Info plugin;

  pulcher::plugin::LoadPlugin(
    plugin, pulcher::plugin::Type::UserInterface
  , "plugins/ui-base.pulcher-plugin"
  );

  sg_buffer bufferVertex;
  {
    std::array<float, 12> vertices = {{
      -1.0f, -1.0f
    , +1.0f, +1.0f
    , +1.0f, -1.0f

    , -1.0f, -1.0f
    , -1.0f, +1.0f
    , +1.0f, +1.0f
    }};

    sg_buffer_desc desc = {};
    desc.size = sizeof(float) * vertices.size();
    desc.content = vertices.data();
    bufferVertex = sg_make_buffer(&desc);
  }

  sg_bindings bindings = {};
  bindings.vertex_buffers[0] = bufferVertex;

  sg_shader shader;
  {
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
    desc.fs.images[0].name = "baseSampler";
    desc.fs.images[0].type = SG_IMAGETYPE_2D;

    desc.vs.source =
      "#version 330\n"
      "uniform vec2 originOffset;\n"
      "in layout(location = 0) vec2 inVertexOrigin;\n"
      "out vec2 uvCoord;\n"
      "void main() {\n"
      "  gl_Position = vec4(inVertexOrigin, 0.5f, 1.0f);\n"
      "  uvCoord = (inVertexOrigin + vec2(1.0f)) * 0.5f;\n"
      "}\n"
    ;

    desc.fs.source =
      "#version 330\n"
      "uniform sampler2D baseSampler;\n"
      "in vec2 uvCoord;\n"
      "out vec4 outColor;\n"
      "void main() {\n"
      "  outColor = texture(baseSampler, uvCoord);\n"
      "}\n"
    ;

    shader = sg_make_shader(&desc);
  }

  sg_pipeline pipeline;
  {
    sg_pipeline_desc desc = {};
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    desc.shader = shader;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;
    desc.rasterizer.cull_mode = SG_CULLMODE_BACK;

    pipeline = sg_make_pipeline(&desc);
  }

  ::PrintUserConfig(userConfig);

  while (!glfwWindowShouldClose(pulcher::gfx::DisplayWindow())) {

    glfwPollEvents();

    simgui_new_frame(
      pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight(), 11.11
    );

    ImGui::ShowDemoWindow();
    ImGui::Begin("Test");
    ImGui::Text("ahhhh");
    if (ImGui::Button("Reload plugins")) {
      pulcher::plugin::UpdatePlugins(plugin);
    }
    ImGui::End();

    plugin.userInterface.Dispatch();

    // -- validate display size in case of resize
    glfwGetFramebufferSize(
      pulcher::gfx::DisplayWindow()
    , &pulcher::gfx::DisplayWidth()
    , &pulcher::gfx::DisplayHeight()
    );

    sg_pass_action passAction;
    passAction.colors[0].action = SG_ACTION_CLEAR;
    passAction.colors[0].val[0] = 0.2f;
    sg_begin_default_pass(
      &passAction
    , pulcher::gfx::DisplayWidth(), pulcher::gfx::DisplayHeight()
    );

    sg_apply_pipeline(pipeline);
    sg_apply_bindings(&bindings);
    /* sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, */ 
    sg_draw(0, 6, 1);

    /* simgui_render(); */
    sg_end_pass();
    sg_commit();
    glfwSwapBuffers(pulcher::gfx::DisplayWindow());
  }

  simgui_shutdown();
  sg_shutdown();

  glfwDestroyWindow(pulcher::gfx::DisplayWindow());
  glfwTerminate();

  return 0;
}
