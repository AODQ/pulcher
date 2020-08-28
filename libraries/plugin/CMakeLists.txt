add_library(pulcher-plugin STATIC)

target_include_directories(pulcher-plugin PUBLIC "include/")
target_sources(
  pulcher-plugin
  PRIVATE
    src/pulcher-plugin/plugin.cpp
)

set_target_properties(
  pulcher-plugin
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(
  pulcher-plugin
  spdlog::spdlog
)
