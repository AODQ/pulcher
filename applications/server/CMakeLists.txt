# only build this target for linux
if (PULCHER_PLATFORM STREQUAL "Linux")
  add_executable(pulcher-server)

  target_sources(
    pulcher-server
    PRIVATE
      src/source.cpp
  )

  target_compile_features(pulcher-server PRIVATE cxx_std_20)

  set_target_properties(
    pulcher-server
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
  )

  target_link_libraries(
    pulcher-server
    PRIVATE
      pulcher-network
      pulcher-util
  )

  install(
    TARGETS pulcher-server
    RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT core
  )
endif()
