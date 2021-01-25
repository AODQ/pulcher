#pragma once

#include <pulcher-audio/fmod-studio-guids.hpp>

#include <pulcher-core/pickup.hpp>
#include <pulcher-util/enum.hpp>

#include <vector>

struct FMOD_STUDIO_EVENTDESCRIPTION;
struct FMOD_STUDIO_EVENTINSTANCE;

#define FMOD_ASSERT(X, ...) \
  if (auto result = X; result != FMOD_OK) { \
    spdlog::critical( \
      "FMOD error; {}@{}; '{}': {}" \
    , __FILE__, __LINE__, #X , FMOD_ErrorString(result) \
    ); \
    __VA_ARGS__ \
  }

namespace pul::audio {

  struct EventInfo {
    pul::audio::event::Type event;
    /* std::vector<std::tuple<pul::audio::param::Type, float>> params; */
    std::vector<std::tuple<std::string, float>> params;
    glm::vec2 origin;
  };

  struct EventInstance {
    EventInstance() = default;
    EventInstance(FMOD_STUDIO_EVENTINSTANCE * instance_) : instance{instance_}
      {}
    ~EventInstance();
    EventInstance(EventInstance &&);
    EventInstance(EventInstance const &) = delete;

    FMOD_STUDIO_EVENTINSTANCE * instance = nullptr;
  };

  struct System {
    std::array<
      FMOD_STUDIO_EVENTDESCRIPTION *
    , Idx(pul::audio::event::Type::Size)
    > eventDescriptions;

    std::vector<FMOD_STUDIO_EVENTINSTANCE *> dispatches;

    // creates audio event
    // returns idx into dispatches, the audio must be manually managed now
    size_t DispatchEvent(pul::audio::EventInfo const & event);

    // creates audio event, returns an RAII structure where you can make
    // changes to the instance before it gets released (audio will still play)
    EventInstance DispatchEventOneOff(pul::audio::EventInfo const & event);

    // -- deprecated --
    bool volniasHit = false;
    bool volniasChargePrimary = false;
    bool volniasChargeSecondary = false;
    bool volniasEndPrimary = false;
    bool volniasPrefirePrimary = false;
    bool volniasPrefireSecondary = false;
    size_t volniasFire = -1ul;
  };
}
