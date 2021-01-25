#include <pulcher-audio/system.hpp>

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
