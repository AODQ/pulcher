#include <pulcher-gfx/spritesheet.hpp>

#include <pulcher-gfx/image.hpp>

#include <sokol/gfx.hpp>

pul::gfx::Spritesheet::~Spritesheet() {
  this->Destroy();
}

pul::gfx::Spritesheet::Spritesheet(Spritesheet && other) {
  this->handle = other.handle;
  this->filename = std::move(other.filename);
  this->width = other.width;
  this->height = other.height;
  other.handle = 0ul;
}

pul::gfx::Spritesheet &
pul::gfx::Spritesheet::operator=(Spritesheet && other) {
  this->handle = other.handle;
  this->filename = std::move(other.filename);
  this->width = other.width;
  this->height = other.height;
  other.handle = 0ul;
  return *this;
}

pul::gfx::Spritesheet pul::gfx::Spritesheet::Construct(
  pul::gfx::Image const & image
) {
  Spritesheet self;

  self.filename = image.filename;
  self.width = image.width;
  self.height = image.height;

  // setup image for sokol
  sg_image_desc desc = {};
  desc.type = SG_IMAGETYPE_2D;
  desc.render_target = false;
  desc.width = image.width;
  desc.height = image.height;
  desc.layers = 1;
  desc.num_mipmaps = 0;
  desc.usage = SG_USAGE_IMMUTABLE;
  desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  desc.sample_count = 0;
  desc.min_filter = SG_FILTER_NEAREST;
  desc.mag_filter = SG_FILTER_NEAREST;
  desc.wrap_u = SG_WRAP_REPEAT; // TODO temporary while beams need
  desc.wrap_v = SG_WRAP_REPEAT; // to wrap image
  desc.max_anisotropy = 0;
  desc.min_lod = 0.0f;
  desc.max_lod = 0.0f;
  desc.content.subimage[0][0].ptr = image.data.data();
  desc.content.subimage[0][0].size = sizeof(uint8_t)*image.data.size()*4ul;
  desc.label = self.filename.c_str();

  self.handle = sg_make_image(&desc).id;

  return self;
}

sg_image pul::gfx::Spritesheet::Image() const {
  sg_image image;
  image.id = this->handle;
  return image;
}

glm::vec2 pul::gfx::Spritesheet::InvResolution() {
  return glm::vec2(1.0f) / glm::vec2(width, height);
}

void pul::gfx::Spritesheet::Destroy() {
  if (handle)
    { sg_destroy_image(this->Image()); }
  handle = 0ul;
}
