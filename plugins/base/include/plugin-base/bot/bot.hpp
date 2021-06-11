#pragma once

#include <span>
#include <vector>

namespace pul::controls { struct Controller; }
namespace pul::core { enum class TileOrientation : size_t; }
namespace pul::core { struct ComponentPlayer; }
namespace pul::core { struct SceneBundle; }
namespace pul::physics { struct Tileset; }

namespace plugin::bot {

  void BuildNavigationMap(char const * filename);

  void SaveNavigationMap(char const * filename);

  void Shutdown();

  void DebugRender();

  void ApplyInput(
    pul::core::SceneBundle & scene
  , pul::controls::Controller & controls
  , pul::core::ComponentPlayer const & bot
  , glm::vec2 const & botOrigin
  );
}
