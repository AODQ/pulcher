#pragma once

#include <string>
#include <array>

namespace pul::core {
  struct Config {
    std::string networkIpAddress;
    uint16_t networkPortAddress = 6599u;
    uint16_t windowWidth = 0ul, windowHeight = 0ul;
    uint16_t framebufferWidth = 0ul, framebufferHeight = 0ul;
  };
}
