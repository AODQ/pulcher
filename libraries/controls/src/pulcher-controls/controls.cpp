#include <pulcher-controls/controls.hpp>

#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui_impl_glfw.hpp>

#include <filesystem>
#include <fstream>

namespace
{
  int16_t glfwMouseWheel;

  bool imguiHidden = false;
}

void pul::controls::UpdateControls(
  GLFWwindow * window
, uint32_t playerCenterX, uint32_t playerCenterY
, pul::controls::Controller & controller
, bool /*wantCaptureKeyboard*/, bool wantCaptureMouse
) {

  static bool prevF = false;

  if (!prevF && glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    imguiHidden ^= 1;
    glfwSetInputMode(
      window, GLFW_CURSOR
    , imguiHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
    );
  }

  prevF = glfwGetKey(window, GLFW_KEY_F);

  // move current to previous
  controller.previous = std::move(controller.current);

  auto & current = controller.current;

  { // clear current control buffer

    // save previous looking position, if mouse goes out of screen
    auto lo = current.lookOffset;
    auto ld = current.lookDirection;
    auto la = current.lookAngle;
    current = {};
    current.lookOffset = lo;
    current.lookDirection = ld;
    current.lookAngle = la;
  }

  { // update looking position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    current.lookOffset = {
      (static_cast<float>(xpos) - playerCenterX)
    , (static_cast<float>(ypos) - playerCenterY)
    };
    current.lookDirection = glm::normalize(current.lookOffset);
    current.lookAngle =
      std::atan2(current.lookDirection.x, current.lookDirection.y);
    // circle is capped to a radius
    float length = glm::min(glm::length(current.lookOffset), 400.0f);
    current.lookOffset = current.lookDirection * length;

    // make sure mouse stays in radius
    if (length == 400.0f && imguiHidden) {
      glfwSetCursorPos(
        window,
        static_cast<double>(current.lookOffset.x + playerCenterX),
        static_cast<double>(current.lookOffset.y + playerCenterY)
      );
    }
  }

  // -0- process inputs
  for (auto const & keymap : controller.keymappings) {
    bool active = false;

    switch (keymap.type) {
      case Controller::Keymap::Type::Keyboard:
        active = glfwGetKey(window, keymap.value) == GLFW_PRESS;
      break;
      case Controller::Keymap::Type::Mouse:
        active =
            !wantCaptureMouse
         && glfwGetMouseButton(window, keymap.value) == GLFW_PRESS
        ;
      break;
      case Controller::Keymap::Type::MouseWheel:
        // TODO
      break;
    }

    for (auto const & output : keymap.outputs) {
      using Type = Controller::ControlOutputType;
      switch (output) {
        case Type::Jump:             current.jump             |= active; break;
        case Type::Dash:             current.dash             |= active; break;
        case Type::Crouch:           current.crouch           |= active; break;
        case Type::Walk:             current.walk             |= active; break;
        case Type::Taunt:            current.taunt            |= active; break;
        case Type::Noclip:           current.noclip           |= active; break;
        case Type::ShootPrimary:     current.shootPrimary     |= active; break;
        case Type::ShootSecondary:   current.shootSecondary   |= active; break;
        case Type::WeaponSwitchNext:
          if (!active) { break; }
          if (!controller.previous.weaponSwitchNext) {
            current.weaponSwitch += 1;
          }
          current.weaponSwitchNext = true;
        break;
        case Type::WeaponSwitchPrev:
          if (!active) { break; }
          if (!controller.previous.weaponSwitchPrev) {
            current.weaponSwitch -= 1;
          }
          current.weaponSwitchPrev = true;
        break;
        case Type::Up:
          if (!active) { break; }
          current.movementVertical = pul::controls::Controller::Movement::Up;
        break;
        case Type::Down:
          if (!active) { break; }
          current.movementVertical = pul::controls::Controller::Movement::Down;
        break;
        case Type::Left:
          if (!active) { break; }
          current.movementHorizontal =
            pul::controls::Controller::Movement::Left;
        break;
        case Type::Right:
          if (!active) { break; }
          current.movementHorizontal =
            pul::controls::Controller::Movement::Right;
        break;

        case Type::WeaponSwitchManshredder:
        case Type::WeaponSwitchDopplerBeam:
        case Type::WeaponSwitchVolnias:
        case Type::WeaponSwitchGrannibal:
        case Type::WeaponSwitchZeusStinger:
        case Type::WeaponSwitchBadFetus:
        case Type::WeaponSwitchPericaliya:
        case Type::WeaponSwitchWallbanger:
        case Type::WeaponSwitchPMF:
        case Type::WeaponSwitchUnarmed:
          if (!active) { break; }
          if (current.weaponSwitchToTypeRequested != -1u) { break; }
          current.weaponSwitchToTypeRequested =
              static_cast<uint32_t>(output)
            - static_cast<uint32_t>(Type::WeaponSwitchManshredder)
          ;

          if (
              controller.previous.weaponSwitchToTypeRequested
           == current.weaponSwitchToTypeRequested
          ) {
            break;
          }

          current.weaponSwitchToType = current.weaponSwitchToTypeRequested;
        break;
      }
    }

    current.weaponSwitch += ::glfwMouseWheel;
    ::glfwMouseWheel = 0;
  }

  // -0- process input computations

  current.movementDirection =
    pul::ToDirection(
      static_cast<float>(current.movementHorizontal)
    , static_cast<float>(current.movementVertical)
    );
}

namespace {
cJSON * LoadJsonFile(std::string const & filename) {
  // load file
  auto file = std::ifstream{filename};
  if (file.eof() || !file.good()) {
    spdlog::error("could not load spritesheet '{}'", filename);
    return nullptr;
  }

  auto str =
    std::string {
      std::istreambuf_iterator<char>(file)
    , std::istreambuf_iterator<char>()
    };

  auto fileDataJson = cJSON_Parse(str.c_str());

  if (fileDataJson == nullptr) {
    spdlog::critical(
      " -- failed to parse json for '{}'; '{}'"
    , filename
    , cJSON_GetErrorPtr()
    );
  }

  return fileDataJson;
}
}

extern "C" {
void ScrollCallback(GLFWwindow * window, double xoffset, double yoffset) {

  // update imgui or game
  if (!::imguiHidden) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    return;
  }

  ::glfwMouseWheel += static_cast<int16_t>(yoffset);
}
}

void pul::controls::LoadControllerConfig(
  GLFWwindow * window
, pul::controls::Controller & controller
) {
  // -0- set scroll callback
  glfwSetScrollCallback(window, ::ScrollCallback);

  // -0- get file, might have to copy one from assets if it doesn't exist

  spdlog::debug("loading controller config");

  std::string const filename = "controller.config";

  // check if file exists, if not create it by copying over file in assets
  if (!std::filesystem::exists(filename)) {
    std::filesystem::copy(
      "assets/base/controller.config"
    , filename
    );
  }

  cJSON * config = ::LoadJsonFile(filename);

  if (!config) {
    spdlog::error("could not load '{}'", filename);
    return;
  }

  // clear out old keymaps
  controller.keymappings = {};

  cJSON * keymap;
  cJSON_ArrayForEach(
    keymap, cJSON_GetObjectItemCaseSensitive(config, "keymaps")
  ) {
    Controller::Keymap controllerKeymap;

    std::string const keymapTypeStr =
      std::string{cJSON_GetObjectItemCaseSensitive(keymap, "type")->valuestring}
    ;

    if (keymapTypeStr == "keyboard") {
      controllerKeymap.type = Controller::Keymap::Keyboard;
    } else if (keymapTypeStr == "mouse") {
      controllerKeymap.type = Controller::Keymap::Mouse;
    } else if (keymapTypeStr == "mousewheel") {
      controllerKeymap.type = Controller::Keymap::MouseWheel;
    } else {
      spdlog::error("unknown keymap type '{}'", keymapTypeStr);
      continue;
    }

    controllerKeymap.value =
      cJSON_GetObjectItemCaseSensitive(keymap, "value")->valueint
    ;

    cJSON * outputType;
    cJSON_ArrayForEach(
      outputType, cJSON_GetObjectItemCaseSensitive(keymap, "outputs")
    ) {
      std::string const outputStr =
        std::string{
          cJSON_GetObjectItemCaseSensitive(outputType, "value")->valuestring
        }
      ;

      static std::map<std::string, Controller::ControlOutputType> const
        strToOutput {{
          { "jump",             Controller::ControlOutputType::Jump            }
        , { "dash",             Controller::ControlOutputType::Dash            }
        , { "crouch",           Controller::ControlOutputType::Crouch          }
        , { "walk",             Controller::ControlOutputType::Walk            }
        , { "taunt",            Controller::ControlOutputType::Taunt           }
        , { "noclip",           Controller::ControlOutputType::Noclip          }
        , { "shoot-primary",    Controller::ControlOutputType::ShootPrimary    }
        , { "shoot-secondary",  Controller::ControlOutputType::ShootSecondary  }
        , {"weapon-switch-next",Controller::ControlOutputType::WeaponSwitchNext}
        , {"weapon-switch-prev",Controller::ControlOutputType::WeaponSwitchPrev}
        , { "up",               Controller::ControlOutputType::Up              }
        , { "down",             Controller::ControlOutputType::Down            }
        , { "left",             Controller::ControlOutputType::Left            }
        , { "right",            Controller::ControlOutputType::Right           }

        , {
            "weapon-switch-type-manshredder",
            Controller::ControlOutputType::WeaponSwitchManshredder
          }
        , {
            "weapon-switch-type-doppler-beam",
            Controller::ControlOutputType::WeaponSwitchDopplerBeam
          }
        , {
            "weapon-switch-type-volnias",
            Controller::ControlOutputType::WeaponSwitchVolnias
          }
        , {
            "weapon-switch-type-grannibal",
            Controller::ControlOutputType::WeaponSwitchGrannibal
          }
        , {
            "weapon-switch-type-zeus-stinger",
            Controller::ControlOutputType::WeaponSwitchZeusStinger
          }
        , {
            "weapon-switch-type-bad-fetus",
            Controller::ControlOutputType::WeaponSwitchBadFetus
          }
        , {
            "weapon-switch-type-pericaliya",
            Controller::ControlOutputType::WeaponSwitchPericaliya
          }
        , {
            "weapon-switch-type-wallbanger",
            Controller::ControlOutputType::WeaponSwitchWallbanger
          }
        , {
            "weapon-switch-type-pmf",
            Controller::ControlOutputType::WeaponSwitchPMF
          }
        , {
            "weapon-switch-type-unarmed",
            Controller::ControlOutputType::WeaponSwitchUnarmed
          }
        }};

      if (auto val = strToOutput.find(outputStr); val != strToOutput.end()) {
        controllerKeymap.outputs.emplace_back(val->second);
      } else {
        spdlog::error("unknown controller output type '{}'", outputStr);
      }
    }

    if (controllerKeymap.outputs.size() > 0ul) {
      controller.keymappings.emplace_back(std::move(controllerKeymap));
    }
  }
}
