#pragma once

namespace pul::core { struct SceneBundle; }

namespace plugin::entity {
  void StartScene(pul::core::SceneBundle & scene);
  void Shutdown(pul::core::SceneBundle & scene);
  void Update(pul::core::SceneBundle & scene);
  void DebugUiDispatch(pul::core::SceneBundle & scene);
}
