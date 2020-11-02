#include <pulcher-controls/controls.hpp>

#include <GLFW/glfw3.h>

void pul::controls::UpdateControls(
  GLFWwindow * window
, uint32_t playerCenterX, uint32_t playerCenterY
, pul::controls::Controller & controller
) {

  // move current to previous
  controller.previous = std::move(controller.current);

  auto & current = controller.current;

  { // update looking position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    current.lookDirection = {
      (static_cast<float>(xpos) - playerCenterX)
    , (static_cast<float>(ypos) - playerCenterY)
    };
    current.lookDirection = glm::normalize(current.lookDirection);
    current.lookAngle =
      std::atan2(current.lookDirection.x, current.lookDirection.y);
  }

  current.movementHorizontal =
    static_cast<pul::controls::Controller::Movement>(
      (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) * -1
    + (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) * +1
    );

  current.movementVertical =
    static_cast<pul::controls::Controller::Movement>(
      (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) * -1
    + (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) * +1
    );

  current.movementDirection =
    pul::ToDirection(
      static_cast<float>(current.movementHorizontal)
    , static_cast<float>(current.movementVertical)
    );

  current.weaponSwitchNext = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);
  current.weaponSwitchPrev = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);

  current.jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  current.taunt = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
  current.dash = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  current.walk = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
  current.crouch =
    (
      glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
    || glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS
    );
  current.shootPrimary =
    glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  current.shootSecondary =
    glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}
