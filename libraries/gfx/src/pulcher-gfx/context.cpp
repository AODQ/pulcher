#include <pulcher-gfx/context.hpp>

#include <pulcher-core/log.hpp>
#include <pulcher-core/config.hpp>

#include <glad/glad.hpp>

#include <GLFW/glfw3.h>

namespace {
GLFWwindow * displayWindow;

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

  ::displayWindow =
    glfwCreateWindow(
      ::displayWidth, ::displayHeight
    , "pulcher", nullptr, nullptr
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

  return true;
}

int & pulcher::gfx::DisplayWidth() { return ::displayWidth; }
int & pulcher::gfx::DisplayHeight() { return ::displayHeight; }
GLFWwindow * pulcher::gfx::DisplayWindow() { return ::displayWindow; }
