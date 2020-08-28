#pragma once

#include <pulcher-plugin/enum.hpp>

#include <string>

namespace pulcher::plugin {
  struct UserInterfaceInfo {
    void (*Dispatch)(
    ) = nullptr;
  };

  struct Info {
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
