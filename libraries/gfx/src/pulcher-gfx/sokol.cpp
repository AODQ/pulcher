#include <pulcher-gfx/sokol.hpp>

#include <pulcher-util/log.hpp>

pul::gfx::SgBuffer::~SgBuffer() {
  Destroy();
}

pul::gfx::SgBuffer::SgBuffer(pul::gfx::SgBuffer && mv) {
  buffer = mv.buffer;
  mv.buffer = {};
}

pul::gfx::SgBuffer & pul::gfx::SgBuffer::operator=(pul::gfx::SgBuffer && mv) {
  this->buffer = mv.buffer;
  mv.buffer = {};
  return *this;
}


void pul::gfx::SgBuffer::Destroy() {
  if (buffer.id) { sg_destroy_buffer(buffer); }
  buffer = {};
}

pul::gfx::SgBuffer::operator sg_buffer & () { return buffer; }
