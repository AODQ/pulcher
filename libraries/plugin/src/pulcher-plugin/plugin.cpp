#include <pulcher-plugin/plugin.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <memory>
#include <vector>

#if defined(__unix__)
  #include <dlfcn.h>
#elif defined(_WIN32) || defined(_WIN64)
#else
  #error "Unsupported operating system"
#endif

namespace {

struct Plugin {
  Plugin(char const * filename, pulcher::plugin::Type type);
  ~Plugin();

  std::string filename;
  void * data = nullptr;
  pulcher::plugin::Type type;

  template <typename T> void LoadFunction(T & fn, char const * label);
  void Reload();
  void Close();
  void Open();
};

Plugin::Plugin(char const * filename_, pulcher::plugin::Type type_)
  : filename(filename_), type(type_)
{
  this->Open();
}

Plugin::~Plugin() {
  this->Close();
}

template <typename T> void Plugin::LoadFunction(T & fn, char const * label) {
  #if defined(__unix__)
    fn = reinterpret_cast<T>(dlsym(this->data, label));
    spdlog::critical(
      "Failed to load function '{}' for plugin '{}'", label, dlerror()
    );
  #elif defined(_WIN32) || defined(_WIN64)
  #endif

  spdlog::debug("Loaded function '{}'", reinterpret_cast<void*>(fn));
}

void Plugin::Reload() {
  spdlog::debug("Reloading plugin '{}'", this->filename.c_str());
  this->Close();
  this->Open();
}

void Plugin::Close() {
  if (!this->data) { return; }
  #if defined(__unix__)
    dlclose(this->data);
  #elif defined(_WIN32) || defined(_WIN64)
  #endif
  this->data = nullptr;
}

void Plugin::Open() {
  #if defined(__unix__)
    this->data = dlopen(this->filename.c_str(), RTLD_NOW);
  #elif defined(_WIN32) || defined(_WIN64)
  #endif
}

// --

std::vector<std::unique_ptr<Plugin>> plugins;

void LoadPluginFunctions(pulcher::plugin::Info & plugin, Plugin & ctx) {
  switch (ctx.type) {
    default: spdlog::critical("Unknown type in LoadPluginFunctions"); break;
    case pulcher::plugin::Type::UserInterface:
      auto & unit = plugin.userInterface;
      ctx.LoadFunction(unit.Dispatch, "Dispatch");
    break;
  }
}

} // -- anon namespace

bool pulcher::plugin::LoadPlugin(
  pulcher::plugin::Info & plugin
, pulcher::plugin::Type type
, std::string const & file
) {
  // first find if the plugin has already been loaded, if that's the case then
  // error
  for (auto & pluginIt : plugins) {
    if (pluginIt->filename == file) {
      spdlog::error("Plugin '{}' already loaded", pluginIt->filename);
      return false;
    }
  }

  // -- load plugin
  ::plugins.emplace_back(std::make_unique<Plugin>(file.c_str(), type));
  auto & pluginEnd = ::plugins.back();

  // check plugin loaded
  if (!pluginEnd->data) {
    ::plugins.pop_back();
    spdlog::error("shared object file could not load correctly");
    return false;
  }

  // -- load functions to respective plugin type
  ::LoadPluginFunctions(plugin, *pluginEnd);

  return true;
}

void pulcher::plugin::FreePlugins() {
  ::plugins.clear();
}

void pulcher::plugin::UpdatePlugins(pulcher::plugin::Info & plugin) {
  for (auto & pluginIt : ::plugins) {
    pluginIt->Reload();

    ::LoadPluginFunctions(plugin, *pluginIt);
  }
}
