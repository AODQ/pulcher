add_library(pulcher-controls STATIC)

target_include_directories(pulcher-controls PUBLIC "include/")
target_sources(
  pulcher-controls
  PRIVATE
    src/pulcher-controls/controls.cpp
)

set_target_properties(
  pulcher-controls
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-controls
  PUBLIC
    glm pulcher-util
  PRIVATE
    glfw cjson
    imgui # for mousewheel/input callbacks
)
