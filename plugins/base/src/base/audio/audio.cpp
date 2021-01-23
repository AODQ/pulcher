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
FMOD_STUDIO_EVENTDESCRIPTION * eventDescriptionLand;
FMOD_STUDIO_EVENTINSTANCE * eventInstanceLand;

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
    )
  );

  FMOD_Studio_Bank_LoadSampleData(fmodBank);


  int count;
  FMOD_ASSERT(
    FMOD_Studio_Bank_GetEventCount(
      fmodBank
    , &count
    )
  );

  spdlog::info("event count: {}", count);

  std::vector<FMOD_STUDIO_EVENTDESCRIPTION *> eventDescriptions;
  eventDescriptions.resize(count);

  FMOD_ASSERT(
    FMOD_Studio_Bank_GetEventList(
      fmodBank
    , eventDescriptions.data()
    , count
    , &count
    )
  );

  eventDescriptionLand = eventDescriptions[0];
}

}

extern "C" {

PUL_PLUGIN_DECL void Audio_LoadAudio(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
  ::InitializeSystem();
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
  FMOD_Studio_System_Update(::fmodSystem);

  auto & audioSystem = scene.AudioSystem();

  if (audioSystem.playerJumped) {
    FMOD_ASSERT(
      FMOD_Studio_EventDescription_CreateInstance(
        eventDescriptionLand, &eventInstanceLand
      )
    );

    FMOD_ASSERT(FMOD_Studio_EventInstance_Start(eventInstanceLand));

    FMOD_ASSERT(FMOD_Studio_EventInstance_SetParameterByName(
      eventInstanceLand, "landing.type", 0.0f, true
    ));
    FMOD_ASSERT(FMOD_Studio_EventInstance_SetParameterByName(
      eventInstanceLand, "velocity.y", 0.5f, true
    ));
  }

  audioSystem.playerJumped = false;
}

PUL_PLUGIN_DECL void Audio_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
}

}
