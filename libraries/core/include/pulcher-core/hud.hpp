#pragma once

namespace pul::core {
  struct HudPlayerInfo {
    uint8_t health, armor;
  };

  struct HudInfo {
    HudPlayerInfo player;
  };
}
