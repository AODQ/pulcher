#pragma once

#include <pulcher-util/pimpl.hpp>

#include <glm/glm.hpp>

namespace entt { enum class entity : std::uint32_t; }
namespace entt { template <typename> class basic_registry; }
namespace entt { using registry = basic_registry<entity>; }
namespace pulcher::animation { struct System; }
namespace pulcher::controls { struct Controller; }
namespace pulcher::physics { struct Queries; }

namespace pulcher::core {
  struct SceneBundle {
    glm::i32vec2 cameraOrigin = {};

    pulcher::animation::System & AnimationSystem();
    pulcher::controls::Controller & PlayerController();
    pulcher::physics::Queries & PhysicsQueries();

    entt::registry & EnttRegistry();

    struct Impl;
    pulcher::util::pimpl<Impl> impl;

    static SceneBundle Construct();
  };
}
