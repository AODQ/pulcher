#pragma once

#include <span>
#include <vector>

namespace pul::core { enum class TileOrientation : size_t; }
namespace pul::core { struct SceneBundle; }
namespace pul::gfx { struct Image; }
namespace pul::physics { struct EntityIntersectionResults; }
namespace pul::physics { struct IntersectionResults; }
namespace pul::physics { struct IntersectorAabb; }
namespace pul::physics { struct IntersectorCircle; }
namespace pul::physics { struct IntersectorPoint; }
namespace pul::physics { struct IntersectorRay; }
namespace pul::physics { struct TilemapLayer; }
namespace pul::physics { struct Tileset; }

namespace plugin::physics {
  void EntityIntersectionRaycast(
    pul::core::SceneBundle & scene
  , pul::physics::IntersectorRay const & ray
  , pul::physics::EntityIntersectionResults & intersectionResults
  );

  void EntityIntersectionCircle(
    pul::core::SceneBundle & scene
  , pul::physics::IntersectorCircle const & circle
  , pul::physics::EntityIntersectionResults & intersectionResults
  );

  void ProcessTileset(
    pul::physics::Tileset & tileset
  , pul::gfx::Image const & image
  );

  void ClearMapGeometry();

  void LoadMapGeometry(
    std::vector<pul::physics::Tileset const *> const & tilesets
  , std::vector<std::span<size_t>>             const & mapTileIndices
  , std::vector<std::span<glm::u32vec2>>       const & mapTileOrigins
  , std::vector<std::span<pul::core::TileOrientation>> const & mapTileOrientations
  );

  bool InverseSceneIntersectionRaycast(
    pul::core::SceneBundle &
  , pul::physics::IntersectorRay const & ray
  , pul::physics::IntersectionResults & intersectionResults
  );

  bool IntersectionRaycast(
    pul::core::SceneBundle &
  , pul::physics::IntersectorRay const & ray
  , pul::physics::IntersectionResults & intersectionResults
  );

  pul::physics::TilemapLayer * TilemapLayer();

  bool IntersectionAabb(
    pul::core::SceneBundle &
  , pul::physics::IntersectorAabb const &
  , pul::physics::IntersectionResults &
  );

  bool IntersectionPoint(
    pul::core::SceneBundle &
  , pul::physics::IntersectorPoint const & point
  , pul::physics::IntersectionResults & intersectionResults
  );

  void DebugUiDispatch(pul::core::SceneBundle &);
}
