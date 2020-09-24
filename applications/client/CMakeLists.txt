add_executable(pulcher-client)

target_sources(
  pulcher-client
  PRIVATE
    src/source.cpp
)

#target_compile_features(pulcher-client PRIVATE cxx_std_20)

set_target_properties(
  pulcher-client
  PROPERTIES
    COMPILE_FLAGS
      "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic -Wundef"
)

target_link_libraries(
  pulcher-client
  PRIVATE
    cxxopts pulcher-core pulcher-gfx pulcher-plugin pulcher-network
    pulcher-physics spdlog
)

install(
  TARGETS pulcher-client
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT core
)
