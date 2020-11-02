#pragma once

#include <sokol/gfx.hpp>

namespace pul::gfx {
  struct SgBuffer {
    SgBuffer() {}
    ~SgBuffer();

    SgBuffer(SgBuffer const &) = delete;
    SgBuffer & operator=(SgBuffer const &) = delete;

    SgBuffer(SgBuffer &&);
    SgBuffer & operator=(SgBuffer &&);

    operator sg_buffer & ();

    void Destroy();

    sg_buffer buffer = {};
  };
}
