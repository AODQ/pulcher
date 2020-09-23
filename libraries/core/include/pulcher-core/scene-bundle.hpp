#pragma once

#include <glm/glm.hpp>

#include <memory>

namespace entt { template <typename> class basic_registry; }
namespace entt { enum class entity : std::uint32_t; }
namespace entt { using registry = basic_registry<entity>; }

namespace pulcher::core {
  struct SceneBundle {
    glm::i32vec2 cameraOrigin = {};

    static SceneBundle Construct();
  };

  entt::registry & Registry();
}
