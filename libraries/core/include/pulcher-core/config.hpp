#pragma once

#include <string>
#include <array>

namespace pulcher::core {
  struct Config {
    std::string networkIpAddress;
    uint16_t networkPortAddress = 6599u;
    uint16_t windowWidth = 0ul, windowHeight = 0ul;
  };
}
