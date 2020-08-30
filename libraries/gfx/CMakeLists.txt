add_library(pulcher-gfx STATIC)

target_include_directories(pulcher-gfx PUBLIC "include/")
target_sources(
  pulcher-gfx
  PRIVATE
    src/pulcher-gfx/context.cpp
)

set_target_properties(
  pulcher-gfx
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-gfx
  PUBLIC
    glfw glad imgui
  PRIVATE
    pulcher-core
)
