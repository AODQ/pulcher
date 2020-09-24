add_library(pulcher-network STATIC)

target_include_directories(pulcher-network PUBLIC "include/")
target_sources(
  pulcher-network
  PRIVATE
    src/pulcher-network/shared.cpp
)

set_target_properties(
  pulcher-network
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
  pulcher-network
  PUBLIC
    enet
)
