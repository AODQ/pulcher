add_library(plugin-animation SHARED)
target_sources(plugin-animation PRIVATE src/source.cpp)

target_link_libraries(
  plugin-animation
  PRIVATE
    EnTT cjson pulcher-core pulcher-gfx pulcher-plugin pulcher-animation
)

set_target_properties(
  plugin-animation
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-animation
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
