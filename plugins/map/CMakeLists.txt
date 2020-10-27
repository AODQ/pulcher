add_library(plugin-map SHARED)
target_sources(plugin-map PRIVATE src/source.cpp)

target_link_libraries(
  plugin-map
  PRIVATE
    cjson pulcher-core pulcher-gfx pulcher-physics pulcher-plugin
    pulcher-animation glm EnTT
)

set_target_properties(
  plugin-map
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-map
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
