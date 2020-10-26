add_library(plugin-audio SHARED)
target_sources(plugin-audio PRIVATE src/source.cpp)

target_link_libraries(
  plugin-audio
  PRIVATE
    EnTT cjson pulcher-core pulcher-gfx pulcher-plugin pulcher-audio fmod
)

set_target_properties(
  plugin-audio
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-audio
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
