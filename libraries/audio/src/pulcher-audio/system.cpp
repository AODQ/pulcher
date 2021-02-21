#include <pulcher-audio/system.hpp>

#include <pulcher-core/scene-bundle.hpp>

#include <pulcher-util/log.hpp>

#include <fmod_errors.h>
#include <fmod_studio.h>

pul::audio::EventInstance::~EventInstance() {
  if (instance) {
    FMOD_Studio_EventInstance_Release(instance);
    instance = nullptr;
  }
}

pul::audio::EventInstance::EventInstance(EventInstance && other) {
  instance = other.instance;
  other.instance = nullptr;
}

size_t pul::audio::System::DispatchEvent(pul::audio::EventInfo const &) {

  // TODO
  return 0ul;
  // lookup for empty dispatches
  /* this->dispatches */
}

pul::audio::EventInstance pul::audio::System::DispatchEventOneOff(
  pul::audio::EventInfo const & event
) {
  auto const & description = this->eventDescriptions[Idx(event.event)];

  FMOD_STUDIO_EVENTINSTANCE * instance = nullptr;

  // -- create instance & start
  FMOD_ASSERT(
    FMOD_Studio_EventDescription_CreateInstance(
      description, &instance
    )
  , return {};
  );

  FMOD_ASSERT(FMOD_Studio_EventInstance_Start(instance), return {};);

  // -- parameters
  for (auto & param : event.params) {
    FMOD_ASSERT(
      FMOD_Studio_EventInstance_SetParameterByName(
        instance
      , std::get<0>(param).c_str()
      , std::get<1>(param)
      , false
      )
    , spdlog::error("param: {}", std::get<0>(param)); continue;
    );
    /* FMOD_ASSERT( */
    /*   FMOD_Studio_EventInstance_SetParameterByID( */
    /*     instance */
    /*   , reinterpret_cast<FMOD_GUID const *>( */
    /*       &pul::audio::param::guids[Idx(std::get<0>(param))] */
    /*     ) */
    /*   , std::get<1>(param) */
    /*   , false */
    /*   ) */
    /* ); */
  }

  // -- attributes
  FMOD_3D_ATTRIBUTES attributes3D;
  attributes3D.position =
    { event.origin.x / 32.0f, event.origin.y / 32.0f, 0.0f };
  attributes3D.velocity = { 0.0f, 0.0f, 0.0f };
  attributes3D.forward  = { 1.0f, 0.0f, 0.0f };
  attributes3D.up       = { 0.0f, 1.0f, 0.0f };
  FMOD_ASSERT(
    FMOD_Studio_EventInstance_Set3DAttributes(instance, &attributes3D)
  , return {};
  );

  return { instance };
}

// -- fmod specific ------------------------------------------------------------

#define FMOD_ASSERT(X, ...) \
  if (auto result = X; result != FMOD_OK) { \
    spdlog::critical( \
      "FMOD error; {}@{}; '{}': {}" \
    , __FILE__, __LINE__, #X , FMOD_ErrorString(result) \
    ); \
    __VA_ARGS__ \
  }

void pul::audio::System::Initialize() {
  { // -- initialize system
    FMOD_ASSERT(
      FMOD_Studio_System_Create(&this->fmodSystem, FMOD_VERSION), return;
    );
    FMOD_ASSERT(
      FMOD_Studio_System_Initialize(
        this->fmodSystem,
        512,
        FMOD_STUDIO_INIT_LIVEUPDATE,
        FMOD_INIT_NORMAL,
        nullptr
      ), return;);

    FMOD_ASSERT(
      FMOD_Studio_System_LoadBankFile(
        this->fmodSystem
      , "assets/base/audio/fmod/Build/Desktop/Master.bank"
      , FMOD_STUDIO_LOAD_BANK_NORMAL
      , &this->fmodBank
      ), return;
    );

    FMOD_ASSERT(FMOD_Studio_Bank_LoadSampleData(this->fmodBank), return;);

    FMOD_ASSERT(
      FMOD_Studio_System_SetNumListeners(this->fmodSystem, 1), return;
    );
  }


  // -- load event descriptions
  for (size_t it = 0; it < Idx(pul::audio::event::Type::Size); ++ it) {
    FMOD_STUDIO_EVENTDESCRIPTION * eventDescription;
    FMOD_ASSERT(
      FMOD_Studio_System_GetEventByID(
        this->fmodSystem
      , reinterpret_cast<FMOD_GUID const *>(&pul::audio::event::guids[it])
      , &eventDescription
      )
    , spdlog::error("event: {}", it); continue;
    );

    this->eventDescriptions[it] = eventDescription;
  }
}

void pul::audio::System::Shutdown() {
  FMOD_Studio_Bank_Unload(this->fmodBank);
  this->fmodBank = nullptr;

  FMOD_Studio_System_Release(this->fmodSystem);
  this->fmodSystem = nullptr;
}

void pul::audio::System::Update(pul::core::SceneBundle & scene) {
  { // -- update listener
    auto const origin = scene.playerOrigin;
    FMOD_3D_ATTRIBUTES attributes3D;
    attributes3D.position    = { origin.x/32.0f, origin.y/32.0f, 0.0f };
    attributes3D.velocity    = { 0.0f, 0.0f, 0.0f };
    attributes3D.forward     = { 1.0f, 0.0f, 0.0f };
    attributes3D.up          = { 0.0f, 1.0f, 0.0f };
    FMOD_ASSERT(
      FMOD_Studio_System_SetListenerAttributes(
        this->fmodSystem, 0
      , &attributes3D, nullptr
      )
    , return;
    );
  }

  // -- update fmod
  FMOD_Studio_System_Update(this->fmodSystem);
}
