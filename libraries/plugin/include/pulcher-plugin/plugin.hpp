#pragma once

#include <pulcher-plugin/enum.hpp>

#include <glm/fwd.hpp>

#include <span>
#include <string>
#include <vector>

namespace pul::animation { struct Instance; }
namespace pul::animation { struct System; }
namespace pul::core { enum class TileOrientation : size_t; }
namespace pul::core { struct SceneBundle; }
namespace pul::gfx { struct Image; }
namespace pul::physics { struct EntityIntersectionResults; }
namespace pul::physics { struct IntersectionResults; }
namespace pul::physics { struct IntersectorCircle; }
namespace pul::physics { struct IntersectorPoint; }
namespace pul::physics { struct IntersectorRay; }
namespace pul::physics { struct Tileset; }
namespace pul::plugin { struct Info; }

#if defined(__unix__)
#define PUL_PLUGIN_DECL
#elif defined(_WIN32) || defined(_WIN64)
#define PUL_PLUGIN_DECL __declspec(dllexport)
#endif

namespace pul::plugin {
  struct UserInterfaceInfo {
    void (*UiDispatch)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
  };

  struct Animation {
    void (*ConstructInstance)(
      pul::core::SceneBundle & scene
    , pul::animation::Instance &
    , pul::animation::System &
    , char const * label
    ) = nullptr;
    void (*LoadAnimations)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*Shutdown)(pul::core::SceneBundle &) = nullptr;
    void (*UpdateFrame)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*RenderAnimations)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*UpdateCache)(pul::animation::Instance & instance) = nullptr;
    void (*UpdateCacheWithPrecalculatedMatrix)(
      pul::animation::Instance & instance
    , glm::mat3 const & matrix
    ) = nullptr;
    void (*UiRender)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
  };

  struct Audio {
    void (*LoadAudio)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*Update)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*Shutdown)(pul::core::SceneBundle &) = nullptr;
    void (*UiRender)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
  };

  struct Entity {
    void (*StartScene)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*Shutdown)(pul::core::SceneBundle &) = nullptr;
    void (*EntityRender)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*EntityUpdate)(
      pul::plugin::Info const &, pul::core::SceneBundle &
    ) = nullptr;
    void (*UiRender)(pul::core::SceneBundle &) = nullptr;
  };

  struct Map {
    void (*LoadMap)(
      pul::plugin::Info const & info, pul::core::SceneBundle &
    , char const * filename
    ) = nullptr;
    void (*Render)(pul::core::SceneBundle &) = nullptr;
    void (*UiRender)(pul::core::SceneBundle &) = nullptr;
    void (*Shutdown)() = nullptr;
  };

  struct Physics {
    void (*EntityIntersectionCircle)(
      pul::core::SceneBundle & scene
    , pul::physics::IntersectorCircle const & circle
    , pul::physics::EntityIntersectionResults & intersectionResults
    ) = nullptr;
    void (*ProcessTileset)(
      pul::physics::Tileset &
    , pul::gfx::Image const &
    ) = nullptr;
    void (*ClearMapGeometry)() = nullptr;
    void (*LoadMapGeometry)(
      std::vector<pul::physics::Tileset const *> const & tilesets
    , std::vector<std::span<size_t>>                 const & mapTileIndices
    , std::vector<std::span<glm::u32vec2>>           const & mapTileOrigins
    , std::vector<
        std::span<pul::core::TileOrientation>
      > const & mapTileOrientations
    ) = nullptr;
    bool (*IntersectionRaycast)(
      pul::core::SceneBundle & scene
    , pul::physics::IntersectorRay const & ray
    , pul::physics::IntersectionResults & intersectionResults
    ) = nullptr;
    bool (*InverseSceneIntersectionRaycast)(
      pul::core::SceneBundle & scene
    , pul::physics::IntersectorRay const & ray
    , pul::physics::IntersectionResults & intersectionResults
    ) = nullptr;
    bool (*IntersectionPoint)(
      pul::core::SceneBundle & scene
    , pul::physics::IntersectorPoint const & ray
    , pul::physics::IntersectionResults & intersectionResults
    ) = nullptr;
    void (*RenderDebug)(pul::core::SceneBundle &) = nullptr;
    void (*UiRender)(pul::core::SceneBundle &) = nullptr;
  };

  struct Info {
    pul::plugin::Animation animation;
    pul::plugin::Audio audio;
    pul::plugin::Entity entity;
    pul::plugin::Map map;
    pul::plugin::Physics physics;
    pul::plugin::UserInterfaceInfo userInterface;
  };

  bool LoadPlugin(
    pul::plugin::Info & plugin
  , pul::plugin::Type type
  , std::string const & filename
  );

  void FreePlugins();

  // check if plugins need to be reloaded
  void UpdatePlugins(Info & plugin);
}
