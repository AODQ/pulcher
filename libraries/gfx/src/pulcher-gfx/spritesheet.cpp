#include <pulcher-gfx/spritesheet.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdouble-promotion"
  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.hpp>
#pragma GCC diagnostic pop

#include <sokol/gfx.hpp>


pulcher::gfx::Spritesheet::~Spritesheet() {
  this->Destroy();
}

pulcher::gfx::Spritesheet::Spritesheet(Spritesheet && other) {
  this->handle = other.handle;
  other.handle = 0ul;
}

pulcher::gfx::Spritesheet &
pulcher::gfx::Spritesheet::operator=(Spritesheet && other) {
  this->handle = other.handle;
  other.handle = 0ul;
  return *this;
}

pulcher::gfx::Spritesheet pulcher::gfx::Spritesheet::Construct(
  char const * filename
) {
  Spritesheet self;

  self.filename = std::string{filename};

  // load image
  stbi_set_flip_vertically_on_load(true);
  int width, height, channels;
  uint8_t * rawByteData =
    stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

  // setup image for sokol
  sg_image_desc desc = {};
  desc.type = SG_IMAGETYPE_2D;
  desc.render_target = false;
  desc.width = width;
  desc.height = height;
  desc.layers = 1;
  desc.num_mipmaps = 0;
  desc.usage = SG_USAGE_IMMUTABLE;
  desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  desc.sample_count = 1;
  desc.min_filter = SG_FILTER_NEAREST;
  desc.mag_filter = SG_FILTER_NEAREST;
  desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  desc.max_anisotropy = 1;
  desc.min_lod = 0.0f;
  desc.max_lod = 0.0f;
  desc.content.subimage[0][0].ptr = rawByteData;
  desc.content.subimage[0][0].size = sizeof(float)*channels*width*height;
  desc.label = filename;

  self.handle = sg_make_image(&desc).id;

  // free stbi mem
  stbi_image_free(rawByteData);

  return self;
}

sg_image pulcher::gfx::Spritesheet::Image() const {
  return sg_image(this->handle);
}

void pulcher::gfx::Spritesheet::Destroy() {
  if (handle)
    { sg_destroy_image(sg_image(handle)); }
  handle = 0ul;
}
