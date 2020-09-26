#include <pulcher-gfx/image.hpp>

#include <pulcher-util/log.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdouble-promotion"
  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.hpp>
#pragma GCC diagnostic pop

pulcher::gfx::Image pulcher::gfx::Image::Construct(char const * filename) {
  pulcher::gfx::Image self;

  stbi_set_flip_vertically_on_load(true);
  int width, height, channels;
  uint8_t * rawByteData =
    stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

  if (!rawByteData) {
    spdlog::error(
      "STBI could not image '{}': '{}'", filename, stbi_failure_reason()
    );
    return self;
  }

  self.width  = static_cast<size_t>(width);
  self.height = static_cast<size_t>(height);
  self.filename = std::string{filename};

  self.data.resize(width*height);
  for (size_t i = 0u; i < self.data.size(); ++ i) {
    size_t x = i%self.width, y = i/self.width;

    uint8_t const * byte =
      reinterpret_cast<uint8_t const *>(rawByteData)
    + self.Idx(x, y)*sizeof(uint8_t)*4
    ;

    // load all color channels, in case of R, RGB, etc set the unused channels
    //   to 1.0f
    for (decltype(channels) j = 0; j < 4; ++ j)
      { self.data[i][j] = channels >= j ? *(byte + j) : 255; }
  }

  stbi_image_free(rawByteData);

  return self;
}
