#pragma once

#include <cstddef>
#include <cstdint>

namespace pulcher::plugin {
  enum struct Type : size_t {
    UserInterface
  , Map
  , Physics
  , Size
  };
}

char const * ToString(pulcher::plugin::Type pluginType);
