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

  FMOD_Studio_Bank_LoadSampleData(fmodBank);
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
  auto & audioSystem = scene.AudioSystem();

  auto const origin = scene.playerOrigin;

  // -- dispatch

  for (auto & dispatch : audioSystem.dispatches) {

    if (dispatch.guid == nullptr) { continue; }

    FMOD_STUDIO_EVENTDESCRIPTION * eventDescription;
    FMOD_ASSERT(
      FMOD_Studio_System_GetEventByID(
        ::fmodSystem
      , reinterpret_cast<FMOD_GUID const *>(dispatch.guid)
      , &eventDescription
      )
    , return;
    );

    FMOD_STUDIO_EVENTINSTANCE * eventInstance;

    FMOD_ASSERT(
      FMOD_Studio_EventDescription_CreateInstance(
        eventDescription, &eventInstance
      )
    , return;
    );

    FMOD_ASSERT(FMOD_Studio_EventInstance_Start(eventInstance), return;);

    for (auto & param : dispatch.parameters) {
      FMOD_ASSERT(
        FMOD_Studio_EventInstance_SetParameterByName(
          eventInstance, param.key.c_str(), param.value, true
        )
      ,
        spdlog::error("param: '{}'", param.key);
        continue;
      );
    }

    FMOD_3D_ATTRIBUTES attributes3D;
    attributes3D.position.x = (dispatch.origin.x - origin.x) / 32.0f;
    attributes3D.position.y = (dispatch.origin.y - origin.y) / 32.0f;
    attributes3D.position.z = 0.0f;
    attributes3D.velocity.x = 0.0f;
    attributes3D.velocity.y = 0.0f;
    attributes3D.velocity.z = 0.0f;
    attributes3D.forward.x  = 1.0f;
    attributes3D.forward.y  = 0.0f;
    attributes3D.forward.z  = 1.0f;
    attributes3D.up.x       = 0.0f;
    attributes3D.up.y       = 1.0f;
    attributes3D.up.z       = 0.0f;
    FMOD_Studio_EventInstance_Set3DAttributes(eventInstance, &attributes3D);
  }

  audioSystem.dispatches.clear();

  // -- update fmod
  FMOD_Studio_System_Update(::fmodSystem);
}

PUL_PLUGIN_DECL void Audio_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
}

}
