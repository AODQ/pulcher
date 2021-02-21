#include <pulcher-plugin/plugin.hpp>

#pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wshadow"
  #include <spdlog/spdlog.h>
#pragma GCC diagnostic pop

#include <memory>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
  #include <dlfcn.h>
#elif defined(_WIN32) || defined(_WIN64)
  #include <errhandlingapi.h>
  #include <libloaderapi.h>
#else
  #error "Unsupported operating system"
#endif

namespace {

#if defined(__unix__) || defined(__APPLE__)
  using PluginHandle = void *;
#elif defined(_WIN64) || defined(_WIN32)
  using PluginHandle = ::HMODULE;
#endif

struct Plugin {
  Plugin(std::filesystem::path filepath);
  ~Plugin();

  std::filesystem::path filepath;
  PluginHandle data = nullptr;

  template <typename T> void LoadFunction(T & fn, char const * label);
  void Reload();
  void Close();
  void Open();
};

// list of unique plugins currently loaded by process
std::vector<std::unique_ptr<Plugin>> plugins;

Plugin::Plugin(std::filesystem::path filepath_)
  : filepath(filepath_)
{
  this->Open();
}

Plugin::~Plugin() {
  this->Close();
}

template <typename T> void Plugin::LoadFunction(T & fn, char const * label) {
  #if defined(__unix__) || defined(__APPLE__)
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
  #if defined(__unix__) || defined(__APPLE__)
    spdlog::info("closing plugin {} {}", this->data, this->filepath.string());
    if (::dlclose(this->data)) {
      spdlog::critical(
        "Failed to close plugin '{}': {}", this->filepath.string(), ::dlerror()
      );
    }
  #elif defined(_WIN32) || defined(_WIN64)
    if (::FreeLibrary(this->data)) {
      spdlog::critical(
        "Failed to load plugin '{}'; {}"
      , this->filepath.string(), ::GetLastError()
      );
    }
  #endif
  this->data = nullptr;
}

void Plugin::Open() {
  #if defined(__unix__) || defined(__APPLE__)
    this->data = ::dlopen(this->filepath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    spdlog::info("opening {} : {}", this->data, this->filepath.string());
    if (!this->data) {
      spdlog::critical(
        "Failed to load plugin '{}'; {}", this->filepath.string(), ::dlerror()
      );
    }
  #elif defined(_WIN32) || defined(_WIN64)
    this->data = ::LoadLibraryW(this->filepath.c_str());
    if (!this->data) {
      spdlog::critical(
        "Failed to load plugin '{}'; {}"
      , this->filepath.string(), ::GetLastError()
      );
    }
  #endif

  // check if handle already exists in plugins, as each plugin must be unique
  for (auto & plugin : ::plugins) {
    if (&*plugin == this) { continue; }
    if (plugin->data == this->data) {
      spdlog::critical("plugin {} already loaded", this->filepath.string());
      break;
    }
  }
}

// --

void LoadPluginFunctions(pul::plugin::Info & plugin, Plugin & ctx) {
  spdlog::info("reloading plugins");

  ctx.LoadFunction(
    plugin.UpdateRenderBundleInstance, "Plugin_UpdateRenderBundleInstance"
  );
  ctx.LoadFunction(plugin.DebugUiDispatch, "Plugin_DebugUiDispatch");
  ctx.LoadFunction(plugin.Initialize, "Plugin_Initialize");
  ctx.LoadFunction(plugin.Interpolate, "Plugin_Interpolate");
  ctx.LoadFunction(plugin.LoadMap, "Plugin_LoadMap");
  ctx.LoadFunction(plugin.LogicUpdate, "Plugin_LogicUpdate");
  ctx.LoadFunction(plugin.RenderInterpolated, "Plugin_RenderInterpolated");
  ctx.LoadFunction(plugin.Shutdown, "Plugin_Shutdown");
}

} // -- anon namespace

bool pul::plugin::LoadPlugin(
  pul::plugin::Info & plugin
, std::filesystem::path const & file
) {
  // first find if the plugin has already been loaded, if that's the case then
  // error
  for (auto & pluginIt : plugins) {
    if (pluginIt->filepath == file) {
      spdlog::error("Plugin '{}' already loaded", pluginIt->filepath.string());
      return false;
    }
  }

  // -- load plugin
  ::plugins.emplace_back(std::make_unique<Plugin>(file));
  auto & pluginEnd = ::plugins.back();

  // check plugin loaded
  if (!pluginEnd->data) {
    ::plugins.pop_back();
    spdlog::error(
      "shared object file {} could not load correctly"
    , file.string()
    );
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
