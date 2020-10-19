#pragma once

#include <pulcher-core/config.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/pimpl.hpp>

#include <glm/glm.hpp>

namespace entt { enum class entity : std::uint32_t; }
namespace entt { template <typename> class basic_registry; }
namespace entt { using registry = basic_registry<entity>; }
namespace pulcher::animation { struct System; }
namespace pulcher::controls { struct Controller; }
namespace pulcher::physics { struct DebugQueries; }

namespace pulcher::core {
  struct SceneBundle {
    glm::i32vec2 cameraOrigin = {};

    float calculatedMsPerFrame = pulcher::util::MsPerFrame;
    size_t numCpuFrames = 0ul;

    pulcher::core::Config config = {};

    glm::u32vec2 playerCenter = {};

    pulcher::animation::System & AnimationSystem();
    pulcher::controls::Controller & PlayerController();
    pulcher::physics::DebugQueries & PhysicsDebugQueries();

    entt::registry & EnttRegistry();

    struct Impl;
    pulcher::util::pimpl<Impl> impl;

    static SceneBundle Construct();
  };
}
