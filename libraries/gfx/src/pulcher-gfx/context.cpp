#include <pulcher-gfx/context.hpp>

#include <pulcher-core/log.hpp>
#include <pulcher-core/config.hpp>

#include <glad/glad.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_glfw.hpp>

namespace {
GLFWwindow * displayWindow;

void InitializeSokol() {
  sg_desc description = {};
  sg_setup(&description);

  simgui_desc_t imguiDescr = {};
  imguiDescr.no_default_font = false;
  imguiDescr.ini_filename = "imgui.ini";
  simgui_setup(&imguiDescr);

  /* IMGUI_CHECKVERSION(); */
  /* ImGui::CreateContext(); */
  ImGuiIO & io = ImGui::GetIO();
  io.ConfigFlags =
    0
  | ImGuiConfigFlags_DockingEnable
  ;
  /* io.KeyMap[ImGuiKey_Tab]        = GLFW_KEY_TAB; */
  /* io.KeyMap[ImGuiKey_LeftArrow]  = GLFW_KEY_LEFT; */
  /* io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT; */
  /* io.KeyMap[ImGuiKey_UpArrow]    = GLFW_KEY_UP; */
  /* io.KeyMap[ImGuiKey_DownArrow]  = GLFW_KEY_DOWN; */
  /* io.KeyMap[ImGuiKey_Home]       = GLFW_KEY_HOME; */
  /* io.KeyMap[ImGuiKey_End]        = GLFW_KEY_END; */
  /* io.KeyMap[ImGuiKey_Delete]     = GLFW_KEY_DELETE; */
  /* io.KeyMap[ImGuiKey_Backspace]  = GLFW_KEY_BACKSPACE; */
  /* io.KeyMap[ImGuiKey_Enter]      = GLFW_KEY_ENTER; */
  /* io.KeyMap[ImGuiKey_Escape]     = GLFW_KEY_ESCAPE; */
  /* io.KeyMap[ImGuiKey_A]          = GLFW_KEY_A; */
  /* io.KeyMap[ImGuiKey_C]          = GLFW_KEY_C; */
  /* io.KeyMap[ImGuiKey_V]          = GLFW_KEY_V; */
  /* io.KeyMap[ImGuiKey_X]          = GLFW_KEY_X; */
  /* io.KeyMap[ImGuiKey_Y]          = GLFW_KEY_Y; */
  /* io.KeyMap[ImGuiKey_Z]          = GLFW_KEY_Z; */


  ImGui::StyleColorsDark();

  /* ImGui_ImplGlfw_InitForOpenGL(pulcher::gfx::DisplayWindow(), true); */
  /* ImGui_ImplOpenGL3_Init("#version 330 core"); */
}

int displayWidth, displayHeight;
} // -- namespace

bool pulcher::gfx::InitializeContext(pulcher::core::Config & config) {
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
  glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
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

  return true;
}

int & pulcher::gfx::DisplayWidth() { return ::displayWidth; }
int & pulcher::gfx::DisplayHeight() { return ::displayHeight; }
GLFWwindow * pulcher::gfx::DisplayWindow() { return ::displayWindow; }
