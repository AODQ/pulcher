add_library(pulcher-core STATIC)

target_include_directories(pulcher-core PUBLIC "include/")
target_sources(
  pulcher-core
  PRIVATE
    src/pulcher-core/log.cpp
)

set_target_properties(
  pulcher-core
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-core
  PUBLIC
    glm spdlog
    pulcher-util
)
