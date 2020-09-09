#pragma once

struct sg_image;

namespace pulcher::gfx {
  struct Spritesheet {
    uint32_t handle;

    static Spritesheet Construct(char const * filename);

    sg_image Image() const;
  };
}
