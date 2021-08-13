#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace pul::core {
  struct Config {
    std::filesystem::path mapPath;
    std::string networkIpAddress;
    uint16_t networkPortAddress = 6599u;
    uint16_t windowWidth = 0ul, windowHeight = 0ul;
    glm::u16vec2 sceneResolution;
    glm::u16vec2 framebufferDim;
    glm::vec2 framebufferDimFloat;
  };
}
