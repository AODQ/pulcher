#include <pulcher-audio/system.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <fmod_errors.h>
#include <fmod_studio.h>

namespace {

FMOD_STUDIO_SYSTEM * fmodSystem = nullptr;
FMOD_STUDIO_BANK * fmodBank = nullptr;
/* FMOD_STUDIO_EVENTDESCRIPTION * eventDescriptionLand; */

/* std::vector<FMOD_STUDIO_EVENTINSTANCE *> */
/* FMOD_STUDIO_EVENTINSTANCE * eventInstanceLand; */

#define FMOD_ASSERT(X, ...) \
  if (auto result = X; result != FMOD_OK) { \
    spdlog::critical( \
      "FMOD error; {}@{}; '{}': {}" \
    , __FILE__, __LINE__, #X , FMOD_ErrorString(result) \
    ); \
    __VA_ARGS__ \
  }

void InitializeSystem() {

  FMOD_ASSERT(FMOD_Studio_System_Create(&fmodSystem, FMOD_VERSION), return;);
  FMOD_ASSERT(
    FMOD_Studio_System_Initialize(
      fmodSystem,
      512,
      FMOD_STUDIO_INIT_LIVEUPDATE,
      FMOD_INIT_NORMAL,
      nullptr
    ), return;);

  FMOD_ASSERT(
    FMOD_Studio_System_LoadBankFile(
      fmodSystem
    , "assets/base/audio/fmod/Build/Desktop/Master.bank"
    , FMOD_STUDIO_LOAD_BANK_NORMAL
    , &fmodBank
    ), return;
  );

  FMOD_ASSERT(FMOD_Studio_Bank_LoadSampleData(fmodBank), return;);

  FMOD_ASSERT(FMOD_Studio_System_SetNumListeners(fmodSystem, 1), return;);
}

}

extern "C" {

PUL_PLUGIN_DECL void Audio_LoadAudio(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  ::InitializeSystem();

  auto & audioSystem = scene.AudioSystem();

  // -- load event descriptions
  for (size_t it = 0; it < Idx(pul::audio::event::Type::Size); ++ it) {
    FMOD_STUDIO_EVENTDESCRIPTION * eventDescription;
    FMOD_ASSERT(
      FMOD_Studio_System_GetEventByID(
        ::fmodSystem
      , reinterpret_cast<FMOD_GUID const *>(&pul::audio::event::guids[it])
      , &eventDescription
      )
    , spdlog::error("event: {}", it); continue;
    );

    audioSystem.eventDescriptions[it] = eventDescription;
  }
}

PUL_PLUGIN_DECL void Audio_Shutdown(
  pul::core::SceneBundle &
) {
  FMOD_Studio_Bank_Unload(::fmodBank);
  ::fmodBank   = nullptr;

  FMOD_Studio_System_Release(::fmodSystem);
  ::fmodSystem = nullptr;
}

PUL_PLUGIN_DECL void Audio_Update(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  { // -- update listener
    auto const origin = scene.playerOrigin;
    FMOD_3D_ATTRIBUTES attributes3D;
    attributes3D.position    = { origin.x/32.0f, origin.y/32.0f, 0.0f };
    attributes3D.velocity    = { 0.0f, 0.0f, 0.0f };
    attributes3D.forward     = { 1.0f, 0.0f, 0.0f };
    attributes3D.up          = { 0.0f, 1.0f, 0.0f };
    FMOD_ASSERT(
      FMOD_Studio_System_SetListenerAttributes(
        ::fmodSystem, 0
      , &attributes3D, nullptr
      )
    , return;
    );
  }

  // -- dispatch

  // -- update fmod
  FMOD_Studio_System_Update(::fmodSystem);
}

PUL_PLUGIN_DECL void Audio_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
}

}
