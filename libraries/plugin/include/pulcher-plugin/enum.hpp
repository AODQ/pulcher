#pragma once

#include <cstddef>
#include <cstdint>

namespace pul::plugin {
  enum struct Type : size_t {
    Animation
  , Audio
  , Entity
  , Map
  , Physics
  , UserInterface
  , Base
  , Size
  };
}

char const * ToString(pul::plugin::Type pluginType);
