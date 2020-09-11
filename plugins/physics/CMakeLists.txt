add_library(plugin-physics SHARED)
target_sources(plugin-physics PRIVATE src/source.cpp)

target_link_libraries(
  plugin-physics
  PRIVATE
    pulcher-core pulcher-gfx pulcher-physics
)

set_target_properties(
  plugin-physics
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-physics
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
