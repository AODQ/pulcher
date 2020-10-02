add_library(pulcher-animation STATIC)

target_include_directories(pulcher-animation PUBLIC "include/")
target_sources(
  pulcher-animation
  PRIVATE
    src/pulcher-animation/animation.cpp
)

set_target_properties(
  pulcher-animation
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-animation
  PUBLIC
    pulcher-gfx
  PRIVATE
    pulcher-util sokol
)
