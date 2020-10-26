#pragma once

namespace pul::audio {
  struct System {
    bool playerJumped = false;
    bool playerDashed = false;
    bool playerSlided = false;
    bool playerStepped = false;
    bool playerTaunted = false;
    bool playerLanded = false;
    size_t envLanded = -1ul;
  };
}
