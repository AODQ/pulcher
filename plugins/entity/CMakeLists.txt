add_library(plugin-entity SHARED)

target_include_directories(plugin-entity PUBLIC "include/")
target_sources(plugin-entity PRIVATE src/source.cpp src/player.cpp)

target_link_libraries(
  plugin-entity
  PRIVATE
    EnTT cjson pulcher-core pulcher-gfx pulcher-physics pulcher-plugin
    pulcher-controls pulcher-animation pulcher-audio
)

set_target_properties(
  plugin-entity
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-entity
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
