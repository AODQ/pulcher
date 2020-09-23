#pragma once

#include <pulcher-controls/controls.hpp>

#include <glm/glm.hpp>

#include <memory>

namespace entt { template <typename> class basic_registry; }
namespace entt { enum class entity : std::uint32_t; }
namespace entt { using registry = basic_registry<entity>; }

namespace pulcher::core {
  struct SceneBundle {
    glm::i32vec2 cameraOrigin = {};

    pulcher::controls::Controller playerController;

    static SceneBundle Construct();
  };

  entt::registry & Registry();
}
