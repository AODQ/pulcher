// --

#include <plugin-base/animation/animation.hpp>
#include <plugin-base/bot/bot.hpp>
#include <plugin-base/debug/renderer.hpp>
#include <plugin-base/entity/entity.hpp>
#include <plugin-base/map/map.hpp>
#include <plugin-base/physics/physics.hpp>
#include <plugin-base/ui/ui.hpp>

#include <pulcher-audio/system.hpp>
#include <pulcher-core/plugin-macro.hpp>
#include <pulcher-core/scene-bundle.hpp>

namespace pul::core { struct SceneBundle; }

extern "C" {

PUL_PLUGIN_DECL void Plugin_LogicUpdate(
  pul::core::SceneBundle & scene
) {
  plugin::entity::Update(scene);
  plugin::animation::UpdateFrame(scene);
}

PUL_PLUGIN_DECL void Plugin_Initialize(pul::core::SceneBundle & scene) {
  plugin::animation::LoadAnimations(scene);
  scene.AudioSystem().Initialize();
  plugin::map::LoadMap(scene, scene.config.mapPath.string().c_str());

  // last thing so all previous information has been loaded up
  plugin::entity::StartScene(scene);

  // initialize debug
  plugin::debug::ShapesRenderInitialize();
}

PUL_PLUGIN_DECL void Plugin_LoadMap(
  pul::core::SceneBundle & scene
) {
  plugin::map::LoadMap(scene, scene.config.mapPath.string().c_str());
}

PUL_PLUGIN_DECL void Plugin_Shutdown(pul::core::SceneBundle & scene) {
  plugin::animation::Shutdown(scene);
  scene.AudioSystem().Shutdown();
  plugin::map::Shutdown();
  plugin::physics::ClearMapGeometry();
  plugin::entity::Shutdown(scene);
  plugin::bot::Shutdown();
  plugin::debug::ShapesRenderShutdown();
}

PUL_PLUGIN_DECL void Plugin_DebugUiDispatch(pul::core::SceneBundle & scene) {
  plugin::ui::DebugUiDispatch(scene);
}

}
