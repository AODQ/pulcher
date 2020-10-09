#pragma once

#include <pulcher-plugin/enum.hpp>

#include <glm/fwd.hpp>

#include <span>
#include <string>
#include <vector>

namespace pulcher::animation { struct Instance; }
namespace pulcher::animation { struct System; }
namespace pulcher::core { struct SceneBundle; }
namespace pulcher::gfx { struct Image; }
namespace pulcher::physics { struct IntersectionResults; }
namespace pulcher::physics { struct IntersectorPoint; }
namespace pulcher::physics { struct IntersectorRay; }
namespace pulcher::physics { struct Tileset; }
namespace pulcher::plugin { struct Info; }

#if defined(__unix__)
#define PUL_PLUGIN_DECL
#elif defined(_WIN32) || defined(_WIN64)
#define PUL_PLUGIN_DECL __declspec(dllexport)
#endif

namespace pulcher::plugin {
  struct UserInterfaceInfo {
    void (*UiDispatch)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
  };

  struct Animation {
    void (*DestroyInstance)(
      pulcher::animation::Instance &
    ) = nullptr;
    void (*ConstructInstance)(
      pulcher::animation::Instance &
    , pulcher::animation::System &
    , char const * label
    ) = nullptr;
    void (*LoadAnimations)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
    void (*Shutdown)(pulcher::core::SceneBundle &) = nullptr;
    void (*UpdateFrame)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
    void (*RenderAnimations)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
    void (*UpdateCache)(pulcher::animation::Instance & instance) = nullptr;
    void (*UpdateCacheWithPrecalculatedMatrix)(
      pulcher::animation::Instance & instance
    , glm::mat3 const & matrix
    ) = nullptr;
    void (*UiRender)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
  };

  struct Entity {
    void (*StartScene)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
    void (*Shutdown)(pulcher::core::SceneBundle &);
    void (*EntityUpdate)(
      pulcher::plugin::Info const &, pulcher::core::SceneBundle &
    ) = nullptr;
    void (*UiRender)(pulcher::core::SceneBundle &) = nullptr;
  };

  struct Map {
    void (*Load)(
      pulcher::plugin::Info const & info
    , char const * filename
    ) = nullptr;
    void (*Render)(pulcher::core::SceneBundle &) = nullptr;
    void (*UiRender)(pulcher::core::SceneBundle &) = nullptr;
    void (*Shutdown)() = nullptr;
  };

  struct Physics {
    void (*ProcessTileset)(
      pulcher::physics::Tileset &
    , pulcher::gfx::Image const &
    ) = nullptr;
    void (*ClearMapGeometry)() = nullptr;
    void (*LoadMapGeometry)(
      std::vector<pulcher::physics::Tileset const *> const & tilesets
    , std::vector<std::span<size_t>>                 const & mapTileIndices
    , std::vector<std::span<glm::u32vec2>>           const & mapTileOrigins
    ) = nullptr;
    bool (*IntersectionRaycast)(
      pulcher::core::SceneBundle const & scene
    , pulcher::physics::IntersectorRay const & ray
    , pulcher::physics::IntersectionResults & intersectionResults
    ) = nullptr;
    bool (*IntersectionPoint)(
      pulcher::core::SceneBundle const & scene
    , pulcher::physics::IntersectorPoint const & ray
    , pulcher::physics::IntersectionResults & intersectionResults
    ) = nullptr;
    void (*UiRender)(pulcher::core::SceneBundle &) = nullptr;
  };

  struct Info {
    pulcher::plugin::Animation animation;
    pulcher::plugin::Entity entity;
    pulcher::plugin::Map map;
    pulcher::plugin::Physics physics;
    pulcher::plugin::UserInterfaceInfo userInterface;
  };

  bool LoadPlugin(
    pulcher::plugin::Info & plugin
  , pulcher::plugin::Type type
  , std::string const & filename
  );

  void FreePlugins();

  // check if plugins need to be reloaded
  void UpdatePlugins(Info & plugin);
}
