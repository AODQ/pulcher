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
  #include <errhandlingapi.h>
  #include <libloaderapi.h>
#else
  #error "Unsupported operating system"
#endif

namespace {

#ifdef __unix__
  using PluginHandle = void *;
#elif defined(_WIN64) || defined(_WIN32)
  using PluginHandle = ::HMODULE;
#endif

struct Plugin {
  Plugin(char const * filename, pulcher::plugin::Type type);
  ~Plugin();

  std::string filename;
  PluginHandle data = nullptr;
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
    fn = reinterpret_cast<T>(::dlsym(this->data, label));
    if (!fn) {
      spdlog::critical(
        "Failed to load function '{}' for plugin '{}'", label, ::dlerror()
      );
    }
  #elif defined(_WIN32) || defined(_WIN64)
    fn =
      reinterpret_cast<T>(
        reinterpret_cast<void *>(::GetProcAddress(this->data, label))
      );
    if (!fn) {
      spdlog::critical(
        "Failed to load function '{}' for plugin '{}'", label, ::GetLastError()
      );
    }
  #endif
}

void Plugin::Reload() {
  spdlog::info("Reloading plugin '{}'", this->filename.c_str());
  this->Close();
  this->Open();
}

void Plugin::Close() {
  if (!this->data) { return; }
  #if defined(__unix__)
    ::dlclose(this->data);
  #elif defined(_WIN32) || defined(_WIN64)
    ::FreeLibrary(this->data);
  #endif
  this->data = nullptr;
}

void Plugin::Open() {
  #if defined(__unix__)
    this->data = ::dlopen(this->filename.c_str(), RTLD_LAZY | RTLD_LOCAL);
    spdlog::debug("Opened plugin {}; {}", this->filename.c_str(), this->data);
    if (!this->data) {
      spdlog::critical(
        "Failed to load plugin '{}'; {}", this->filename, ::dlerror()
      );
    }
  #elif defined(_WIN32) || defined(_WIN64)
    this->data = ::LoadLibraryA(this->filename.c_str());
    if (!this->data) {
      spdlog::critical(
        "Failed to load plugin '{}'; {}", this->filename, ::GetLastError()
      );
    }
  #endif
}

// --

std::vector<std::unique_ptr<Plugin>> plugins;

void LoadPluginFunctions(pulcher::plugin::Info & plugin, Plugin & ctx) {
  switch (ctx.type) {
    default: spdlog::critical("Unknown type in LoadPluginFunctions"); break;
    case pulcher::plugin::Type::Animation: {
      auto & unit = plugin.animation;
      ctx.LoadFunction(unit.ConstructInstance, "ConstructInstance");
      ctx.LoadFunction(unit.LoadAnimations,   "LoadAnimations");
      ctx.LoadFunction(unit.Shutdown,         "Shutdown");
      ctx.LoadFunction(unit.UpdateFrame, "UpdateFrame");
      ctx.LoadFunction(unit.RenderAnimations, "RenderAnimations");
      ctx.LoadFunction(unit.UiRender,         "UiRender");
    } break;
    case pulcher::plugin::Type::Entity: {
      auto & unit = plugin.entity;
      ctx.LoadFunction(unit.StartScene,   "StartScene");
      ctx.LoadFunction(unit.Shutdown,     "Shutdown");
      ctx.LoadFunction(unit.EntityUpdate, "EntityUpdate");
      ctx.LoadFunction(unit.UiRender,     "UiRender");
    } break;
    case pulcher::plugin::Type::UserInterface: {
      auto & unit = plugin.userInterface;
      ctx.LoadFunction(unit.UiDispatch, "UiDispatch");
    } break;
    case pulcher::plugin::Type::Map: {
      auto & unit = plugin.map;
      ctx.LoadFunction(unit.Load,     "Load");
      ctx.LoadFunction(unit.Render,   "Render");
      ctx.LoadFunction(unit.UiRender, "UiRender");
      ctx.LoadFunction(unit.Shutdown, "Shutdown");
    } break;
    case pulcher::plugin::Type::Physics: {
      auto & unit = plugin.physics;
      ctx.LoadFunction(unit.ProcessTileset,   "ProcessTileset");
      ctx.LoadFunction(unit.ClearMapGeometry, "ClearMapGeometry");
      ctx.LoadFunction(unit.LoadMapGeometry,  "LoadMapGeometry");
      ctx.LoadFunction(unit.ProcessPhysics,   "ProcessPhysics");
      ctx.LoadFunction(unit.UiRender,         "UiRender");
    } break;
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
