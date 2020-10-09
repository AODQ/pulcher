#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>

void plugin::entity::UpdatePlayer(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
, pulcher::core::ComponentPlayer & player
, pulcher::animation::ComponentInstance & playerAnim
) {

  auto & registry = scene.EnttRegistry();

  // -- process inputs / events

  auto & current = scene.PlayerController().current;
  player.velocity.x = static_cast<float>(current.movementHorizontal);
  player.velocity.y = static_cast<float>(current.movementVertical);
  if (current.dash) { player.velocity *= 4.0f; }

  player.jumping = current.jump;
  if (player.jumping) {
    player.velocity.y -= 3.5f;
  }

  const float velocityXAbs = glm::abs(player.velocity.x);

  bool grounded = true;

  // -- set leg animation
  if (grounded) {
    if (current.crouch) {
      if (velocityXAbs < 0.1f) {
        playerAnim.instance.pieceToState["legs"].Apply("crouch-idle");
      } else {
        playerAnim.instance.pieceToState["legs"].Apply("crouch-walk");
      }
    }
    else if (velocityXAbs < 0.1f) {
      playerAnim.instance.pieceToState["legs"].Apply("stand");
    }
    else if (velocityXAbs < 1.5f) {
      playerAnim.instance.pieceToState["legs"].Apply("walk");
    }
    else {
      playerAnim.instance.pieceToState["legs"].Apply("run");
    }
  } else {
    playerAnim.instance.pieceToState["legs"].Apply("air-idle");
  }

  // -- arm animation
  if (grounded) {
    if (current.crouch) {
      playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
      playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
    }
    else if (velocityXAbs < 0.1f) {
      playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
      playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
    }
    else if (velocityXAbs < 1.5f) {
      playerAnim.instance.pieceToState["arm-back"].Apply("unequip-walk");
      playerAnim.instance.pieceToState["arm-front"].Apply("unequip-walk");
    }
    else {
      playerAnim.instance.pieceToState["arm-back"].Apply("unequip-run");
      playerAnim.instance.pieceToState["arm-front"].Apply("unequip-run");
    }
  } else {
    playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
    playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
  }

  if (
      current.movementHorizontal
   == pulcher::controls::Controller::Movement::Right
  ) {
    playerAnim.instance.pieceToState["legs"].flip = true;
  }
  else if (
      current.movementHorizontal
   == pulcher::controls::Controller::Movement::Left
  ) {
    playerAnim.instance.pieceToState["legs"].flip = false;
  }

  float const angle =
    std::atan2(current.lookDirection.x, current.lookDirection.y);

  playerAnim.instance.pieceToState["arm-back"].angle = angle;
  playerAnim.instance.pieceToState["arm-front"].angle = angle;

  playerAnim.instance.pieceToState["head"].angle = angle;

  // center camera on this
  scene.cameraOrigin = glm::i32vec2(player.origin);
  playerAnim.instance.origin = glm::vec2(0.0f);

  // center weapon origin, first have to update cache for this animation to
  // get the hand position
  {
    plugin.animation.UpdateCache(playerAnim.instance);
    auto & handState = playerAnim.instance.pieceToState["weapon-placeholder"];

    auto & weaponAnimation =
      registry.get<pulcher::animation::ComponentInstance>(
        player.weaponAnimation
      );
    auto & weaponState = weaponAnimation.instance.pieceToState["pericaliya"];

    weaponAnimation.instance.origin = playerAnim.instance.origin;

    weaponState.angle = playerAnim.instance.pieceToState["arm-front"].angle;
    weaponState.flip = playerAnim.instance.pieceToState["legs"].flip;

    plugin.animation.UpdateCacheWithPrecalculatedMatrix(
      weaponAnimation.instance, handState.cachedLocalSkeletalMatrix
    );
  }
}
