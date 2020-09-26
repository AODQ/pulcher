#pragma once

#include <cstddef>
#include <cstdint>

namespace pulcher::plugin {
  enum struct Type : size_t {
    Animation
  , Entity
  , Map
  , Physics
  , Size
  , UserInterface
  };
}

char const * ToString(pulcher::plugin::Type pluginType);
