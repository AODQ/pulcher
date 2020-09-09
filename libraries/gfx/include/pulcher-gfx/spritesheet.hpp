#pragma once

#include <string>

struct sg_image;

namespace pulcher::gfx {
  struct Spritesheet {
    uint32_t handle;
    std::string filename;

    Spritesheet() = default;
    ~Spritesheet();
    Spritesheet(Spritesheet const &) = delete;
    Spritesheet(Spritesheet &&);
    Spritesheet & operator=(Spritesheet const &) = delete;
    Spritesheet & operator=(Spritesheet &&);

    static Spritesheet Construct(char const * filename);

    sg_image Image() const;

    void Destroy();
  };
}
