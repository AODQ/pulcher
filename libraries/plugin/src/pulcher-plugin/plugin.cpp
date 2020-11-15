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
  Plugin(char const * filename, pul::plugin::Type type);
  ~Plugin();

  std::string filename;
  PluginHandle data = nullptr;
  pul::plugin::Type type;

  template <typename T> void LoadFunction(T & fn, char const * label);
  void Reload();
  void Close();
  void Open();
};

Plugin::Plugin(char const * filename_, pul::plugin::Type type_)
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

void LoadPluginFunctions(pul::plugin::Info & plugin, Plugin & ctx) {
  switch (ctx.type) {
    default: spdlog::critical("Unknown type in LoadPluginFunctions"); break;
    case pul::plugin::Type::Animation: {
      auto & unit = plugin.animation;
      ctx.LoadFunction(unit.ConstructInstance, "Animation_ConstructInstance");
      ctx.LoadFunction(unit.LoadAnimations,    "Animation_LoadAnimations");
      ctx.LoadFunction(unit.Shutdown,          "Animation_Shutdown");
      ctx.LoadFunction(unit.UpdateFrame,       "Animation_UpdateFrame");
      ctx.LoadFunction(unit.RenderAnimations,  "Animation_RenderAnimations");
      ctx.LoadFunction(unit.UpdateCache,       "Animation_UpdateCache");
      ctx.LoadFunction(
        unit.UpdateCacheWithPrecalculatedMatrix
      , "Animation_UpdateCacheWithPrecalculatedMatrix"
      );
      ctx.LoadFunction(unit.UiRender,          "Animation_UiRender");
    } break;
    case pul::plugin::Type::Audio: {
      auto & unit = plugin.audio;
      ctx.LoadFunction(unit.LoadAudio, "Audio_LoadAudio");
      ctx.LoadFunction(unit.Update,    "Audio_Update");
      ctx.LoadFunction(unit.Shutdown,  "Audio_Shutdown");
      ctx.LoadFunction(unit.UiRender,  "Audio_UiRender");
    } break;
    case pul::plugin::Type::Entity: {
      auto & unit = plugin.entity;
      ctx.LoadFunction(unit.EntityRender, "Entity_EntityRender");
      ctx.LoadFunction(unit.EntityUpdate, "Entity_EntityUpdate");
      ctx.LoadFunction(unit.Shutdown,     "Entity_Shutdown");
      ctx.LoadFunction(unit.StartScene,   "Entity_StartScene");
      ctx.LoadFunction(unit.UiRender,     "Entity_UiRender");
    } break;
    case pul::plugin::Type::UserInterface: {
      auto & unit = plugin.userInterface;
      ctx.LoadFunction(unit.UiDispatch, "Ui_UiDispatch");
    } break;
    case pul::plugin::Type::Map: {
      auto & unit = plugin.map;
      ctx.LoadFunction(unit.LoadMap,     "Map_LoadMap");
      ctx.LoadFunction(unit.Render,   "Map_Render");
      ctx.LoadFunction(unit.UiRender, "Map_UiRender");
      ctx.LoadFunction(unit.Shutdown, "Map_Shutdown");
    } break;
    case pul::plugin::Type::Physics: {
      auto & unit = plugin.physics;
      ctx.LoadFunction(
        unit.EntityIntersectionCircle,
        "Physics_EntityIntersectionCircle"
      );
      ctx.LoadFunction(unit.ProcessTileset,      "Physics_ProcessTileset");
      ctx.LoadFunction(unit.ClearMapGeometry,    "Physics_ClearMapGeometry");
      ctx.LoadFunction(unit.LoadMapGeometry,     "Physics_LoadMapGeometry");
      ctx.LoadFunction(unit.IntersectionRaycast, "Physics_IntersectionRaycast");
      ctx.LoadFunction(
        unit.InverseSceneIntersectionRaycast
      , "Physics_InverseSceneIntersectionRaycast"
      );
      ctx.LoadFunction(unit.IntersectionPoint,   "Physics_IntersectionPoint");
      ctx.LoadFunction(unit.RenderDebug,         "Physics_RenderDebug");
      ctx.LoadFunction(unit.UiRender,            "Physics_UiRender");
    } break;
  }
}

} // -- anon namespace

bool pul::plugin::LoadPlugin(
  pul::plugin::Info & plugin
, pul::plugin::Type type
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

void pul::plugin::FreePlugins() {
  ::plugins.clear();
}

void pul::plugin::UpdatePlugins(pul::plugin::Info & plugin) {
  for (auto & pluginIt : ::plugins) {
    pluginIt->Reload();

    ::LoadPluginFunctions(plugin, *pluginIt);
  }
}
