#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

void plugin::entity::UpdatePlayer(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
, pulcher::core::ComponentPlayer & player
, pulcher::animation::ComponentInstance & playerAnim
) {

  auto & registry = scene.EnttRegistry();
  auto & controller = scene.PlayerController().current;

  { // -- process inputs / events
    player.velocity.x = static_cast<float>(controller.movementHorizontal);
    player.velocity.y = static_cast<float>(controller.movementVertical);
    if (controller.dash) { player.velocity *= 4.0f; }

    if (player.grounded) {
      player.acceleration.y = 0.0f;
    }

    player.jumping = controller.jump;
    if (player.grounded && player.jumping) {
      player.acceleration.y = -16.0f;
    }

    if (!player.grounded)
      { player.acceleration.y += 1.0f; }

    player.velocity += player.acceleration;
    player.acceleration *= 0.95f;

    // if grounded and velocity downwards, redirect it horizontally i guess
    if (player.grounded && player.velocity.y > 0.0f) {
      player.acceleration.x += glm::sign(player.acceleration.x) * glm::abs(player.velocity.y);
      player.velocity.y = 0.0f;
    }
  }

  { // -- apply physics

    float groundedPotential = 0.0f;

    glm::vec2 centerOrigin = player.origin - glm::vec2(0.0f, 30.0f);
    glm::vec2 groundedFloorOrigin = player.origin - glm::vec2(0, 2);
    glm::vec2 floorOrigin = player.origin;

    if (player.grounded) {
      {
        // first check if we can move freely horizontally
        pulcher::physics::IntersectorRay ray;
        ray.beginOrigin = groundedFloorOrigin;
        ray.endOrigin =
          groundedFloorOrigin + glm::vec2(player.velocity.x, 0.0f);
        if (
          pulcher::physics::IntersectionResults results;
          !plugin.physics.IntersectionRaycast(scene, ray, results)
        ) {
          player.origin.x += player.velocity.x;
        } else {
          player.velocity.x = 0.0f;
        }
      }

      {
        // check for vertical movement
        pulcher::physics::IntersectorRay ray;
        ray.beginOrigin = groundedFloorOrigin;
        ray.endOrigin =
          groundedFloorOrigin + glm::vec2(player.velocity.y, 0.0f);
        if (
          pulcher::physics::IntersectionResults results;
          !plugin.physics.IntersectionRaycast(scene, ray, results)
        ) {
          player.origin.y += player.velocity.y;
        } else {
          groundedPotential = player.velocity.y;
          player.velocity.y = 0.0f;
        }
      }
    } else {
      {
        // first check if we can move freely
        pulcher::physics::IntersectorRay ray;
        ray.beginOrigin = glm::round(floorOrigin);
        ray.endOrigin = glm::round(floorOrigin + player.velocity);
        if (
          pulcher::physics::IntersectionResults results;
          !plugin.physics.IntersectionRaycast(scene, ray, results)
        ) {
          player.origin += player.velocity;
        } else {
          groundedPotential = player.velocity.y;
          player.velocity = glm::vec2(0.0f);
        }
      }

      if (groundedPotential != 0.0f) {
        pulcher::physics::IntersectorRay ray;
        ray.beginOrigin = floorOrigin;
        ray.endOrigin = floorOrigin + glm::vec2(0.0f, groundedPotential);
        if (
          pulcher::physics::IntersectionResults results;
          plugin.physics.IntersectionRaycast(scene, ray, results)
        ) {
          player.origin.y = results.origin.y;
        }
      }
    }

    { // gravity/ground check
      pulcher::physics::IntersectorPoint point;
      point.origin = floorOrigin + glm::vec2(0.0f, 1.0f);
      pulcher::physics::IntersectionResults results;
      player.grounded = plugin.physics.IntersectionPoint(scene, point, results);
    }

  }

  const float velocityXAbs = glm::abs(player.velocity.x);

  bool grounded = true;

  { // -- apply animations

    // -- set leg animation
    if (grounded) {
      if (controller.crouch) {
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
      if (controller.crouch) {
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
        controller.movementHorizontal
     == pulcher::controls::Controller::Movement::Right
    ) {
      playerAnim.instance.pieceToState["legs"].flip = true;
    }
    else if (
        controller.movementHorizontal
     == pulcher::controls::Controller::Movement::Left
    ) {
      playerAnim.instance.pieceToState["legs"].flip = false;
    }

    float const angle =
      std::atan2(controller.lookDirection.x, controller.lookDirection.y);

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

      char const * weaponStr = ToStr(player.inventory.currentWeapon);

      auto & weaponAnimation =
        registry.get<pulcher::animation::ComponentInstance>(
          player.weaponAnimation
        ).instance;

      // nothing should render if unarmed
      weaponAnimation.visible =
        player.inventory.currentWeapon != pulcher::core::WeaponType::Unarmed;

      auto & weaponState = weaponAnimation.pieceToState["weapons"];

      weaponState.Apply(weaponStr);

      weaponAnimation.origin = playerAnim.instance.origin;

      weaponState.angle = playerAnim.instance.pieceToState["arm-front"].angle;
      weaponState.flip = playerAnim.instance.pieceToState["legs"].flip;

      plugin.animation.UpdateCacheWithPrecalculatedMatrix(
        weaponAnimation, handState.cachedLocalSkeletalMatrix
      );
    }
  }
}
