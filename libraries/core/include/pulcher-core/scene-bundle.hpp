#pragma once

#include <pulcher-core/config.hpp>
#include <pulcher-util/any.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/pimpl.hpp>

#include <glm/glm.hpp>

#include <string>

namespace entt { enum class entity : std::uint32_t; }
namespace entt { template <typename> class basic_registry; }
namespace entt { using registry = basic_registry<entity>; }
namespace pul::animation { struct System; }
namespace pul::audio { struct System; }
namespace pul::controls { struct Controller; }
namespace pul::core { struct ComponentOrigin; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct HudInfo; }
namespace pul::core { struct PlayerMetaInfo; }
namespace pul::physics { struct DebugQueries; }
namespace pul::plugin { struct Info; }

namespace pul::core {
  struct SceneBundle {
    glm::vec2 playerOrigin = {};
    glm::i32vec2 cameraOrigin = {};

    float calculatedMsPerFrame = pul::util::MsPerFrame;
    size_t numCpuFrames = 0ul;

    bool debugFrameBufferHovered = false;

    pul::core::Config config = {};

    glm::u32vec2 playerCenter = {};

    pul::animation::System & AnimationSystem();
    pul::controls::Controller & PlayerController();
    pul::core::PlayerMetaInfo & PlayerMetaInfo();
    pul::physics::DebugQueries & PhysicsDebugQueries();
    pul::audio::System & AudioSystem();
    pul::core::HudInfo & Hud();


    // store player between reloads
    pul::core::ComponentPlayer & StoredDebugPlayerComponent();
    pul::core::ComponentOrigin & StoredDebugPlayerOriginComponent();

    entt::registry & EnttRegistry();

    pul::animation::System const & AnimationSystem() const;
    pul::controls::Controller const & PlayerController() const;
    entt::registry const & EnttRegistry() const;
    pul::core::HudInfo const & Hud() const;

    struct Impl;
    pul::util::pimpl<Impl> impl;

    static SceneBundle Construct();
  };

  struct RenderBundleInstance {
    glm::vec2 playerOrigin;
    glm::vec2 cameraOrigin;

    glm::vec2 playerCenter;

    // allows plugins to query plugin data
    std::map<std::string, std::shared_ptr<pul::util::Any>> pluginBundleData;

    // 0 .. 1, ms delta interpolation. only used from instances created by
    //   RenderBundle::Interpolate
    float msDeltaInterp;
  };

  struct RenderBundle {
    RenderBundleInstance previous, current;

    bool debugUseInterpolation = true;

    // constructs dummy render bundle (previous = current = scene), this is so
    // that the first couple of frames of rendering are valid & do not have
    // garbage memory
    static RenderBundle Construct(
      pul::plugin::Info const & plugin, SceneBundle &
    );

    // stores current into previous, and then updates current with scene
    void Update(pul::plugin::Info const & plugin, SceneBundle &);

    // constructs a render bundle instance from the ms-delta interpolation
    // value, from 0 to 1. Most likely `accumulatedMs / totalMsPerFrame`.
    // equivalent of pseudo-code `mix(previous, current, msDeltaInterp)`
    RenderBundleInstance Interpolate(
      pul::plugin::Info const & plugin, float const msDeltaInterp
    );
  };
}
