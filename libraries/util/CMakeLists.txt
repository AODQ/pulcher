add_library(pulcher-util STATIC)

target_include_directories(pulcher-util PUBLIC "include/")
target_sources(
  pulcher-util
  PRIVATE
    src/pulcher-util/any.cpp
    src/pulcher-util/enum.cpp
    src/pulcher-util/log.cpp
)

set_target_properties(
  pulcher-util
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-util
  PUBLIC
    spdlog
    glm
)
