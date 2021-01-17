#pragma once

#include <pulcher-core/config.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/pimpl.hpp>

#include <glm/glm.hpp>

namespace entt { enum class entity : std::uint32_t; }
namespace entt { template <typename> class basic_registry; }
namespace entt { using registry = basic_registry<entity>; }
namespace pul::animation { struct System; }
namespace pul::audio { struct System; }
namespace pul::controls { struct Controller; }
namespace pul::core { struct ComponentOrigin; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct PlayerMetaInfo; }
namespace pul::core { struct HudInfo; }
namespace pul::physics { struct DebugQueries; }

namespace pul::core {
  struct SceneBundle final {
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

    struct Impl;
    pul::util::pimpl<Impl> impl;

    static SceneBundle Construct();
  };

  struct RenderBundleInstance final {
    glm::vec2 playerOrigin;
    glm::vec2 cameraOrigin;

    glm::vec2 playerCenter;

    // 0 .. 1, ms delta interpolation. only used from instances created by
    //   RenderBundle::Interpolate
    float msDeltaInterp;
  };

  struct RenderBundle final {
    RenderBundleInstance previous, current;

    static RenderBundle Construct(SceneBundle const &);
    void Update(SceneBundle const &);
    RenderBundleInstance Interpolate(float const msDeltaInterp);
  };
}
