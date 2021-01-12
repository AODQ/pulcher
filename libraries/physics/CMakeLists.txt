add_library(pulcher-physics STATIC)

target_include_directories(pulcher-physics PUBLIC "include/")
target_sources(
  pulcher-physics
  PRIVATE
    src/pulcher-physics/tileset.cpp
    src/pulcher-physics/intersections.cpp
)

set_target_properties(
  pulcher-physics
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-physics
    PUBLIC glm pulcher-util EnTT pulcher-core
)
