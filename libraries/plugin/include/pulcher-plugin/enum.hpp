#pragma once

#include <cstddef>
#include <cstdint>

namespace pul::plugin {
  enum struct Type : size_t {
    Animation
  , Entity
  , Map
  , Physics
  , Size
  , UserInterface
  };
}

char const * ToString(pul::plugin::Type pluginType);
