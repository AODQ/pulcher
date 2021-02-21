#pragma once

#include <span>
#include <vector>

namespace pul::controls { struct Controller; }
namespace pul::core { enum class TileOrientation : size_t; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct SceneBundle; }
namespace pul::physics { struct Tileset; }
namespace pul::plugin { struct Info; }

namespace plugin::bot {

  void BuildNavigationMap(
    std::vector<pul::physics::Tileset const *> const & tilesets
  , std::vector<std::span<size_t>>             const & mapTileIndices
  , std::vector<std::span<glm::u32vec2>>       const & mapTileOrigins
  , std::vector<
      std::span<pul::core::TileOrientation>
    > const & mapTileOrientations
  );

  void ApplyInput(
    pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
  , pul::controls::Controller & controls
  , pul::core::ComponentPlayer const & bot
  , glm::vec2 const & botOrigin
  );
}
