add_library(pulcher-audio STATIC)

target_include_directories(pulcher-audio PUBLIC "include/")
target_sources(
  pulcher-audio
  PRIVATE
    src/pulcher-audio/system.cpp
)

set_target_properties(
  pulcher-audio
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-audio
  PUBLIC
    pulcher-util
  PRIVATE
    fmod
)
