#include <pulcher-audio/system.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <fmod.hpp>
#include <fmod_errors.h>

namespace {

FMOD_SYSTEM * fmodSystem = nullptr;
FMOD_SOUND * fmodSound = nullptr;

#define FMOD_ASSERT(X, ...) \
  if (auto result = X; result != FMOD_OK) { \
    spdlog::critical( \
      "FMOD error; {}@{}; '{}': {}" \
    , __FILE__, __LINE__, #X , FMOD_ErrorString(result) \
    ); \
    __VA_ARGS__ \
  }

void InitializeSystem() {

  FMOD_ASSERT(FMOD_System_Create(&fmodSystem), return;);
  FMOD_ASSERT(FMOD_System_Init(fmodSystem, 512, FMOD_INIT_NORMAL, 0), return;);
}

}

extern "C" {

PUL_PLUGIN_DECL void Audio_LoadAudio(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
  ::InitializeSystem();

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/misc/kill-enemy.wav"
    , FMOD_DEFAULT
    , nullptr
    , &::fmodSound
    ), ;
  );
}

PUL_PLUGIN_DECL void Audio_Shutdown(
  pul::core::SceneBundle &
) {
  FMOD_System_Release(::fmodSystem);
  ::fmodSystem = nullptr;
}

PUL_PLUGIN_DECL void Audio_Update(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
  FMOD_System_Update(::fmodSystem);

  static size_t F = 0;
  if (F == 0) {
    F = 1;

    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSound
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }
}

PUL_PLUGIN_DECL void Audio_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
}

}
