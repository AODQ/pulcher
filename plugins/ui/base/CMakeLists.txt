add_library(ui-base SHARED)
target_sources(ui-base PRIVATE src/source.cpp)

target_link_libraries(
  ui-base
  PRIVATE
    pulcher-core pulcher-gfx pulcher-plugin pulcher-controls
)

set_target_properties(
  ui-base
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS ui-base
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
