#pragma once

#include <pulcher-util/consts.hpp>

#include <glm/fwd.hpp>

#include <cstdint>
#include <map>
#include <vector>

struct GLFWwindow;

namespace pul::controls {

  struct Controller {
    enum struct Movement : int8_t {
      Left = -1, Right = +1,
      Up = -1, Down = +1,
      None = 0ul, Size = 3ul
    };

    enum class ControlOutputType {
      Jump, Dash, Crouch, Walk
    , Taunt, ShootPrimary, ShootSecondary
    , Up, Down, Left, Right
    , WeaponSwitchNext, WeaponSwitchPrev
    // must match to pul::core::WeaponType
    , WeaponSwitchManshredder
    , WeaponSwitchDopplerBeam
    , WeaponSwitchVolnias
    , WeaponSwitchGrannibal
    , WeaponSwitchZeusStinger
    , WeaponSwitchBadFetus
    , WeaponSwitchPericaliya
    , WeaponSwitchWallbanger
    , WeaponSwitchPMF
    , WeaponSwitchUnarmed
    , Noclip
    };

    struct Keymap {
      enum Type {
        Keyboard
      , Mouse
      , MouseWheel
      };

      Type type = Type::Keyboard;
      uint32_t value = 0;
      std::vector<ControlOutputType> outputs = {};
    };

    std::vector<Keymap> keymappings;

    struct Frame {
      pul::Direction movementDirection = pul::Direction::None;
      Movement
        movementHorizontal = Movement::None
      , movementVertical = Movement::None
      ;

      bool jump = false, dash = false, crouch = false, walk = false;

      bool taunt = false;

      bool noclip = false;

      bool shootPrimary = false, shootSecondary = false;

      // below two lines should be unused, they're cached internally to avoid
      //   button repeats
      bool weaponSwitchPrev = false, weaponSwitchNext = false;
      uint32_t weaponSwitchToTypeRequested = -1u;

      uint32_t weaponSwitchToType = -1u;

      int16_t weaponSwitch = 0;

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

  void LoadControllerConfig(
    GLFWwindow * window
  , pul::controls::Controller & controller
  );
}
