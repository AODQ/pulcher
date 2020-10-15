#include <pulcher-controls/controls.hpp>

#include <GLFW/glfw3.h>

void pulcher::controls::UpdateControls(
  GLFWwindow * window
, uint32_t displayWidth, uint32_t displayHeight
, pulcher::controls::Controller & controller
) {

  // move current to previous
  controller.previous = std::move(controller.current);

  auto & current = controller.current;

  { // update looking position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    current.lookDirection = {
      (static_cast<float>(xpos) - displayWidth*0.5f) / displayWidth
    , (static_cast<float>(ypos) - displayHeight*0.5f) / displayHeight
    };
  }

  current.movementHorizontal =
    static_cast<pulcher::controls::Controller::Movement>(
      (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) * -1
    + (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) * +1
    );

  current.movementVertical =
    static_cast<pulcher::controls::Controller::Movement>(
      (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) * -1
    + (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) * +1
    );

  current.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  current.dash = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  current.walk = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
  current.crouch =
    (
      glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
    || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS
    );
  current.shoot = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
}
