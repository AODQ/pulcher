#pragma once

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-physics/intersections.hpp>

#include <glm/glm.hpp>

namespace entt { template <typename> class basic_registry; }
namespace entt { enum class entity : std::uint32_t; }
namespace entt { using registry = basic_registry<entity>; }

namespace pulcher::core {
  struct SceneBundle {
    glm::i32vec2 cameraOrigin = {};

    pulcher::animation::System animationSystem;
    pulcher::controls::Controller playerController;
    pulcher::physics::Queries physicsQueries;

    static SceneBundle Construct();
  };

  entt::registry & Registry();
}
