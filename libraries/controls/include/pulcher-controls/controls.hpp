#pragma once

#include <pulcher-util/consts.hpp>

#include <cstdint>

#include <glm/fwd.hpp>

struct GLFWwindow;

namespace pul::controls {

  struct Controller {
    enum struct Movement : int8_t {
      Left = -1, Right = +1,
      Up = -1, Down = +1,
      None = 0ul, Size = 3ul
    };

    struct Frame {
      pul::Direction movementDirection = pul::Direction::None;
      Movement
        movementHorizontal = Movement::None
      , movementVertical = Movement::None
      ;

      bool jump = false, dash = false, crouch = false, walk = false;

      bool taunt = false;

      bool shootPrimary = false, shootSecondary = false;

      bool weaponSwitchNext = false, weaponSwitchPrev = false;

      glm::vec2 lookDirection = { 0.0f, 0.0f };
      glm::vec2 lookOffset = { 0.0f, 0.0f };
      float lookAngle;
    };

    Frame current, previous;
  };

  void UpdateControls(
    GLFWwindow * window
  , uint32_t playerCenterX, uint32_t playerCenterY
  , pul::controls::Controller & controller
  , bool wantCaptureKeyboard, bool wantCaptureMouse
  );

  struct ComponentController {
    pul::controls::Controller controller;
  };
}
