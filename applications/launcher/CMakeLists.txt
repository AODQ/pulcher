add_executable(pulcher-launcher)

target_sources(
  pulcher-launcher
  PRIVATE
    src/source.cpp
)

set_target_properties(
  pulcher-launcher
  PROPERTIES
    COMPILE_FLAGS
      "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic -Wundef"
)

set(CMAKE_EXE_LINKER_FLAGS "-static")

target_link_libraries(
  pulcher-launcher
  PRIVATE
    cxxopts pulcher-util pulcher-network tiny-process-library
)

install(
  TARGETS pulcher-launcher
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT core
)
