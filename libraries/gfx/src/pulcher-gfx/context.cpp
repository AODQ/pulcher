#include <pulcher-gfx/context.hpp>

#include <pulcher-core/config.hpp>
#include <pulcher-util/log.hpp>

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>

namespace {
GLFWwindow * displayWindow;

sg_image sceneImageColor = {};
sg_image sceneImageDepth = {};
sg_pass  scenePass  = {};

void InitializeSokol() {
  sg_desc description = {};
  description.buffer_pool_size = 5192;
  sg_setup(&description);

  simgui_desc_t imguiDescr = {};
  imguiDescr.no_default_font = false;
  imguiDescr.ini_filename = "imgui.ini";
  simgui_setup(&imguiDescr);

  ImGuiIO & io = ImGui::GetIO();
  io.ConfigFlags =
    0
  | ImGuiConfigFlags_DockingEnable
  ;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(pul::gfx::DisplayWindow(), true);
}

int displayWidth, displayHeight;
} // -- namespace

bool pul::gfx::InitializeContext(pul::core::Config & config) {
  spdlog::info("initializing graphics context");
  if (!glfwInit()) {
    spdlog::error("failed to initialize GLFW");
    return false;
  }

  glfwSetErrorCallback([](int, char const * errorStr) {
    spdlog::error("glfw error; '{}'", errorStr);
  });

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // set up new window to be as non-intrusive as possible. No maximizing,
  // floating, no moving cursor, no auto-focus, etc.
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
  glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 0);

  glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);

  // -- get render resolution
  if (config.windowWidth > 0ul && config.windowHeight > 0ul) {
    ::displayWidth  = config.windowWidth;
    ::displayHeight = config.windowHeight;
  } else { // use monitor
    int xpos, ypos, width, height;
    glfwGetMonitorWorkarea(
      glfwGetPrimaryMonitor(), &xpos, &ypos, &width, &height
    );

    ::displayWidth = width;
    ::displayHeight = height;

    config.windowWidth  = ::displayWidth;
    config.windowHeight = ::displayHeight;
  }

  #ifdef __unix__
    char const * windowTitle = "Pulcher (Linux)";
  #elif _WIN64
    char const * windowTitle = "Pulcher (Win64)";
  #endif

  ::displayWindow =
    glfwCreateWindow(
      ::displayWidth, ::displayHeight
    , windowTitle, nullptr, nullptr
    );

  if (!::displayWindow) {
    spdlog::error("failed to initialize GLFW window");
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(::displayWindow);

  // initialize glad
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    spdlog::error("failed to initialize GLAD");
    return false;
  }

  glfwSwapInterval(0);

  ::InitializeSokol();

  {
    sg_image_desc desc = {};
    desc.render_target = true;
    desc.width = config.framebufferDim.x;
    desc.height = config.framebufferDim.y;
    desc.num_mipmaps = 1;
    desc.usage = SG_USAGE_IMMUTABLE;
    /* desc.pixel_format = SG_PIXELFORMAT_BGRA8; */
    desc.sample_count = 1;
    desc.min_filter = SG_FILTER_NEAREST;
    desc.mag_filter = SG_FILTER_NEAREST;
    desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    desc.wrap_w = SG_WRAP_CLAMP_TO_EDGE;
    desc.border_color = SG_BORDERCOLOR_OPAQUE_BLACK;
    desc.max_anisotropy = 1;
    desc.min_lod = 0.0f;
    desc.max_lod = std::numeric_limits<float>::max();
    desc.content.subimage[0][0].ptr = nullptr;
    desc.content.subimage[0][0].size = 0;
    ::sceneImageColor = sg_make_image(desc);
  }

  {
    sg_image_desc desc = {};
    desc.render_target = true;
    desc.width = config.framebufferDim.x;
    desc.height = config.framebufferDim.y;
    desc.num_mipmaps = 1;
    desc.usage = SG_USAGE_IMMUTABLE;
    desc.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    desc.sample_count = 1;
    desc.min_filter = SG_FILTER_NEAREST;
    desc.mag_filter = SG_FILTER_NEAREST;
    desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    desc.wrap_w = SG_WRAP_CLAMP_TO_EDGE;
    desc.border_color = SG_BORDERCOLOR_OPAQUE_BLACK;
    desc.max_anisotropy = 1;
    desc.min_lod = 0.0f;
    desc.max_lod = std::numeric_limits<float>::max();
    desc.content.subimage[0][0].ptr = nullptr;
    desc.content.subimage[0][0].size = 0;
    ::sceneImageDepth = sg_make_image(desc);
  }

  {
    sg_pass_desc desc = {};
    desc.color_attachments[0].image = ::sceneImageColor;
    desc.color_attachments[0].mip_level = 0;
    desc.color_attachments[0].face = 0;
    desc.depth_stencil_attachment.image = ::sceneImageDepth;
    desc.depth_stencil_attachment.mip_level = 0;
    desc.depth_stencil_attachment.face = 0;
    ::scenePass = sg_make_pass(desc);
  }

  return true;
}

int & pul::gfx::DisplayWidth() { return ::displayWidth; }
int & pul::gfx::DisplayHeight() { return ::displayHeight; }
GLFWwindow * pul::gfx::DisplayWindow() { return ::displayWindow; }

void pul::gfx::StartFrame(float deltaMs) {
  // -- validate display size in case of resize
  glfwGetFramebufferSize(
    pul::gfx::DisplayWindow()
  , &pul::gfx::DisplayWidth()
  , &pul::gfx::DisplayHeight()
  );

  simgui_new_frame(
    pul::gfx::DisplayWidth(), pul::gfx::DisplayHeight()
  , static_cast<double>(deltaMs/1000.0f)
  );

  ImGui_ImplGlfw_NewFrame();
}

void pul::gfx::EndFrame() {
  glfwSwapBuffers(pul::gfx::DisplayWindow());
}

void pul::gfx::Shutdown() {

  ImGui_ImplGlfw_Shutdown();

  simgui_shutdown();
  sg_shutdown();

  glfwDestroyWindow(pul::gfx::DisplayWindow());
  glfwTerminate();
}

sg_pass & pul::gfx::ScenePass() {
  return ::scenePass;
}

sg_image & pul::gfx::SceneImageColor() { return ::sceneImageColor; }
