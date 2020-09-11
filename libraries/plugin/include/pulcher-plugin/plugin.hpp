#pragma once

#include <pulcher-plugin/enum.hpp>

#include <glm/fwd.hpp>

#include <span>
#include <string>
#include <vector>

namespace pulcher::gfx { struct Image; }
namespace pulcher::physics { struct BufferedQueries; }
namespace pulcher::physics { struct Queries; }
namespace pulcher::physics { struct Tileset; }
namespace pulcher::plugin { struct Info; }

namespace pulcher::plugin {
  struct UserInterfaceInfo {
    void (*Dispatch)(
    ) = nullptr;
  };

  struct Map {
    void (*Load)(
      pulcher::plugin::Info const & info
    , char const * filename
    ) = nullptr;
    void (*Render)() = nullptr;
    void (*UiRender)() = nullptr;
    void (*Shutdown)() = nullptr;
  };

  struct Physics {
    pulcher::physics::Tileset (*ProcessTileset)(
      pulcher::gfx::Image const &
    ) = nullptr;
    void (*ClearMapGeometry)() = nullptr;
    void (*LoadMapGeometry)(
      std::vector<pulcher::physics::Tileset const *> const & tilesets
    , std::vector<std::span<size_t>>                 const & mapTileIndices
    , std::vector<std::span<glm::u32vec2>>           const & mapTileOrigins
    ) = nullptr;
    void (*ProcessPhysics)(pulcher::physics::BufferedQueries &) = nullptr;
    void (*UiRender)(pulcher::physics::BufferedQueries &) = nullptr;
  };

  struct Info {
    pulcher::plugin::UserInterfaceInfo userInterface;
    pulcher::plugin::Map map;
    pulcher::plugin::Physics physics;
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
