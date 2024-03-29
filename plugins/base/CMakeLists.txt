add_library(plugin-base SHARED)

target_include_directories(plugin-base PUBLIC "include/")
target_sources(
  plugin-base
  PRIVATE
    src/base/animation/animation.cpp
    src/base/animation/render.cpp
    src/base/base.cpp
    src/base/bot/bot.cpp
    src/base/bot/creature-lump.cpp
    src/base/bot/creature-moldwing.cpp
    src/base/bot/creature-vapivara.cpp
    src/base/debug/renderer.cpp
    src/base/entity/config.cpp
    src/base/entity/cursor.cpp
    src/base/entity/entity.cpp
    src/base/entity/player.cpp
    src/base/entity/weapon.cpp
    src/base/interpolation.cpp
    src/base/map/map.cpp
    src/base/physics/physics.cpp
    src/base/ui/ui.cpp
)

target_link_libraries(
  plugin-base
  PRIVATE
    chaiscript
    EnTT cjson pulcher-core pulcher-gfx pulcher-physics box2d
    pulcher-controls pulcher-animation pulcher-audio
    fmod
    micropather
)

set_target_properties(
  plugin-base
    PROPERTIES
      COMPILE_FLAGS
        "-Wshadow -Wdouble-promotion -Wall -Wformat=2 -Wextra -Wpedantic \
         -Wundef"
      SUFFIX ".pulcher-plugin"
      PREFIX ""
)

install(
  TARGETS plugin-base
  LIBRARY NAMELINK_SKIP
  LIBRARY
    DESTINATION bin/plugins/
    COMPONENT plugin
  RUNTIME
    DESTINATION bin/plugins/
    COMPONENT plugin
)
