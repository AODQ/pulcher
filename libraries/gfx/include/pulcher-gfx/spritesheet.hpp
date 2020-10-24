#pragma once

#include <string>

namespace pul::gfx { struct Image; }
struct sg_image;

namespace pul::gfx {
  struct Spritesheet {
    uint32_t handle;
    size_t width, height;
    std::string filename;

    Spritesheet() = default;
    ~Spritesheet();
    Spritesheet(Spritesheet const &) = delete;
    Spritesheet(Spritesheet &&);
    Spritesheet & operator=(Spritesheet const &) = delete;
    Spritesheet & operator=(Spritesheet &&);

    static Spritesheet Construct(pul::gfx::Image const &);

    sg_image Image() const;
    glm::vec2 InvResolution();

    void Destroy();
  };
}
