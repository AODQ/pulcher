# Pulcher

![pulcher](https://images-wixmp-ed30a86b8c4ca887773594c2.wixmp.com/f/9387ecea-2531-4df3-b66c-525d1350353a/d4detpn-33cf19d3-c6c4-48f4-ae1d-60ebb356ec7a.png?token=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1cm46YXBwOiIsImlzcyI6InVybjphcHA6Iiwib2JqIjpbW3sicGF0aCI6IlwvZlwvOTM4N2VjZWEtMjUzMS00ZGYzLWI2NmMtNTI1ZDEzNTAzNTNhXC9kNGRldHBuLTMzY2YxOWQzLWM2YzQtNDhmNC1hZTFkLTYwZWJiMzU2ZWM3YS5wbmcifV1dLCJhdWQiOlsidXJuOnNlcnZpY2U6ZmlsZS5kb3dubG9hZCJdfQ.U9N8wGR2Ki-kMPPQGhqKaI2YFKqdXUeYOulYthxEVqg)

2D cross-platform (Windows/Linux) game inspired by Quake. This is the source-code repository, pre-built binaries exist at https://github.com/AODQ/pulcher-binaries , which provides rolling-release updates for every code commit.

The game is processed at a logical 90 frames per second, with 1 frame of latency to allow for interpolation to render at 90+ frames. The engine is built "from scratch", though it mostly just ties multiple libraries together. The engine is basically not seperable from the game, or, a different perspective, they are the same entity. However, it's not some hard-coded nightmare, and in the future it is definitely possible that the two are seperable.

Sokol is used to render OpenGL3.2 & EnTT is used for the entity-component system. These both drive the majority of Pulcher functionality. ImGui is used for development to allow prototyping of functionality to be very fast, which combined with hot-reloadable plugins make developing Pulcher very painless & easy.

[![physx](https://img.youtube.com/vi/M27mRWE0eLg/0.jpg)](https://www.youtube.com/watch?v=M27mRWE0eLg&feature=youtu.be)

# Code Philosphy

Code should be simple and data-oriented. There should be minimal implicit behavior, including hidden heap allocations. OO patterns like polymorphism & inheritance are also discouraged. It should be fast to compile code, so headers should generally avoid including other headers, favoring forward-declarations. Also dependencies should be minimal & carefully chosen, and built along-side the project. All code, with an exception to files that exist to hold data, should be 80-character width.

Some definitions of ambigous terminology;

  - 'size' in consts will usually refer to the number of bytes, whereas 'length'
    will refer to the number of elements. Though I have recently decided to
    stick with 'byteSize' to avoid confusion with std::vector::size.

# Layout

Applications directory contains directories of every executable built by Pulcher. Right now it is just the client, but in the future it could be a server, launcher, etc. They provide minimal code, mostly just setting up environment from command line, initiating plugins, then using library/plugin code to process logic/graphics/etc.

Assets directory is private, if you want to create your own assets the structure is;

 - `assets/base/config.json` ; json file for pulcher configurations, not meant to be modified by user (optional)
 - `assets/base/controller.config` ; json file for controller input (meant to be modified by user)
 - `assets/base/map/*.json` ; Tiled maps
 - `assets/base/spritesheets/data.json` ; json list of files that contain spritesheet information

Docs directory provides documentation on the codebase, game-related documentation goes to https://github.com/AODQ/pulcher-binaries/wiki .

Libraries directory contains code to build static libraries that are useful to Pulcher in a generic manner;

  - `libraries/animation` contains the structure of the animation system
  - `libraries/audio` provides a system to allow multiple places of the codebase to trigger audio
  - `libraries/controls` parses controller configuration and performs controller logic
  - `libraries/core` provides core Pulcher functionality, such as entity structs for EnTT useage, configurations, enums, scene-bundle to pass Pulcher data around, etc
  - `libraries/gfx` provides useful utilities for GLFW, OpenGL, Sokol, spritesheets, etc
  - `libraries/enet` provides network utilities that are shared for client/server communication
  - `libraries/physics` provides a generic interface for physics interactions
  - `libraries/plugin` gives plugin information, such as function pointers, along with being able to load, reload, and free plugins
  - `libraries/util` provides generic non-Pulcher specific functionality

Plugins directory contains dynamic libraries that provide known C-ABI functions to Pulcher, which allows for hot-reloading and quick prototyping. Right now there is one single base plugin, which provides functionality for the base engine; extensions can be created by modders in the future to create their own plugins, which I assume will grab assets from `assets/plugin-name/`. See Creating Plugins section for more details;

 - `plugins/base/animation` contains the implementation of the animation system, which is very flexible & drives most of the animated components of Pulcher.
 - `plugins/base/audio` contains implementation of audio
 - `plugins/base/entity` contains implementation of Pulcher entities (weapons, players, etc)
 - `plugins/base/map` contains implementation of the Pulcher map, including loading & rendering.
 - `plugins/base/ui` contains `DebugUiDispatch` function, that along with containing some ImGui behavior (diagnostics, controls, etc) will dispatch every plugins `DebugUiDispatch`

The scripts directory contains generic scripts, mostly to quickly set up a development environment, updating the binaries repository, & testing that each binary launches.

`third-party` ; submodule repository of third-party libraries; some point to other submodules. Many of them have modifications.

# Building

The build system uses only CMake. It relies on pulling from a lot of third-party repos, even if the library already exists on the system, which is not the best way to handle CMake in the majority of usecases. However, since Pulcher plans to be released in binary-form, it's most convenient for developers/modders IMHO that the libraries under `third-party` is built along-side Pulcher.

To build on Linux, you can either set up the environment manually or use the `script/configure.py` script to setup the build environment for multiple platforms (Linux, Windows 32-bit, Windows 64-bit). You will require Clang, MinGw (for cross-compilation to Windows) and Ninja to use them. You can set up the build environment manually too, just be sure to pass `PULCHER_PLATFORM=Linux` to CMake.

Simple example to build Linux binary on Linux;
```
mkdir pulcher ; cd pulcher
git clone https://github.com/aodq/pulcher repo
cd repo
git submodule update --init --recursive
cd ../
mkdir build install
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=Release -DPULCHER_PLATFORM=Linux ../repo
```

Pulcher is officially built against Clang, but should also build against GCC without warnings.

To build on Mac, use clang and CMake, similar to the above example for building on Linux, except specify the `PULCHER_PLATFORM` as macOS
```
cmake -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=Release -DPULCHER_PLATFORM=macOS ../repo
```

MacOS support is limited, and there maybe be dependency issues, in particular be sure to install/update GLFW on your system with homebrew.

I haven't tried building Pulcher on Windows, though besides possible MSVC errors, it shouldn't be difficult to set it up. But you probably won't be able to produce Linux binaries with MSVC, your best option is most likely to be to use the Windows Subsystem for Linux to build Linux binaries.

# Creating Plugins

The best way to mod Pulcher is to use plugins. This is still in the design phase. Most likely how this will work is that for a plugin `PluginTest`, it must provide a C-ABI compatible functions;

  - `void DebugUiDispatch(pul::core::SceneBundle &);` - updates ImGui for developer purposes
  - `void Initialize(pul::core::SceneBundle &); - called when plugin is initialized
  - `void Shutdown(pul::core::SceneBundle &); - unloads the plugin
  - `void LoadMap(pul::core::SceneBundle &); - called when map is initialized, will probably also contain map information to load custom entities
  - `void LogicUpdate(pul::core::SceneBundle &);` - updates the game @ pul::util::MsPerFrame (16 ms/frame), used to update game-related logic
  - UpdateRenderBundleInstance(pul::core::SceneBundle &, pul::core::RenderBundleInstance &);
      puts logical information into a render bundle for rendering purposes
  - `void Interpolate(float const msDeltaInterp, pul::core::RenderBundleInstance const & previousBundle, pul::core::RenderBundleInstance const & currentBundle, pul::core::RenderBundleInstance::outputBundle);`
     outputs an interpolated bundle based on current/previous bundle and current msDeltaInterp; ei; mix(previous.playerPosition, current.playerPosition, msDeltaInterp);
  - `void RenderInterpolated(pul::core::SceneBundle &);` - used to provide custom rendering of the scene, can be called at any varying rate, thus the plugin
                                                 itself must handle its own interpolation, but items within SceneBundle will already be interpolated


thus a plugin writer should generally expect this behavior;
```

void main() {
  // create scene
  ...

  plugin_Initialize(scene);

  // setup and create map
  ...

  plugin_LoadMap(scene);

  while ( ... ) {
    // update timing ...

    if (readyToComputeLogicalFrame) {
      plugin_Update(scene);

      plugin_UpdateRenderBundleInstance(scene, current);
    }

    // prepare interpolated bundle & rendering
    ...

    plugin_Interpolate(msDeltaInterpolate, previous, current, output);

    // setup rendering
    ...

    plugin_RenderInterpolated(scene, output);

    ...

    plugin_DebugUiDispatch(scene);
  }

  plugin_Shutdown(scene);

  // destroy scene
  ...
}

```

Plugins should generally otherwise communicate to Pulcher by using entt
