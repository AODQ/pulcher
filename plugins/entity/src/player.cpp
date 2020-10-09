#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

#include <imgui/imgui.hpp>

namespace {
  float inputAccelMultiplier = 0.5f;
  float jumpingAccel = -16.0f;
  float gravity = 1.0f;
  float friction = 0.95f;
  float horizontalGroundedVelocityStop = 0.2f;
}

void plugin::entity::UpdatePlayer(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
, pulcher::core::ComponentPlayer & player
, pulcher::animation::ComponentInstance & playerAnim
) {

  auto & registry = scene.EnttRegistry();
  auto & controller = scene.PlayerController().current;

  { // -- process inputs / events
    float inputAccel = static_cast<float>(controller.movementHorizontal);
    inputAccel *= ::inputAccelMultiplier;

    player.velocity += inputAccel;
    /* if (controller.dash) { player.velocity *= 4.0f; } */

    if (player.grounded) {
      player.velocity.y = 0.0f;
    }

    player.jumping = controller.jump;
    if (player.grounded && player.jumping) {
      player.velocity.y = ::jumpingAccel;
    }

    if (!player.grounded)
      { player.velocity.y += ::gravity; }

    if (player.grounded) {
      player.velocity.x *= ::friction;
    }

    if (
        inputAccel == 0.0f && player.grounded
     && glm::abs(player.velocity.x) < ::horizontalGroundedVelocityStop
    ) {
      player.velocity.x = 0.0f;
    }

    // if grounded and velocity downwards, redirect it horizontally i guess
    /* if (player.grounded && player.velocity.y > 0.0f) { */
    /*   player.velocity.x += */
    /*     glm::sign(player.velocity.x) * glm::abs(player.velocity.y); */
    /*   player.velocity.y = 0.0f; */
    /* } */
  }

  { // -- apply physics

    float groundedPotential = 0.0f;

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
        } else {
          spdlog::debug("adding velocity {}", player.velocity.y);
          player.origin.y += groundedPotential;
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

    bool playerDirFlip = playerAnim.instance.pieceToState["legs"].flip;

    if (
        controller.movementHorizontal
     == pulcher::controls::Controller::Movement::Right
    ) {
      playerDirFlip = true;
    }
    else if (
        controller.movementHorizontal
     == pulcher::controls::Controller::Movement::Left
    ) {
      playerDirFlip = false;
    }

    playerAnim.instance.pieceToState["legs"].flip = playerDirFlip;

    auto const & currentWeaponInfo =
      pulcher::core::weaponInfo[Idx(player.inventory.currentWeapon)];

    switch (currentWeaponInfo.requiredHands) {
      case 0: break;
      case 1:
        if (playerDirFlip)
          playerAnim.instance.pieceToState["arm-back"].Apply("equip-1H");
        else
          playerAnim.instance.pieceToState["arm-front"].Apply("equip-1H");
      break;
      case 2:
        playerAnim.instance.pieceToState["arm-back"].Apply("equip-2H");
        playerAnim.instance.pieceToState["arm-front"].Apply("equip-2H");
      break;
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

void plugin::entity::UiRenderPlayer(pulcher::core::SceneBundle & scene) {
  ImGui::Begin("Physics");

  ImGui::Separator();
  ImGui::Separator();

  ImGui::SliderFloat(
    "input accel multiplier", &::inputAccelMultiplier, 0.001f, 2.0f
  );
  ImGui::SliderFloat("gravity", &::gravity, 0.001f, 2.0f);
  ImGui::SliderFloat("jumping accel", &::jumpingAccel, -0.001f, -64.0f);
  ImGui::SliderFloat("friction", &::friction, 0.5f, 1.0f);
  ImGui::SliderFloat(
    "horizontal grounded velocity stop"
  , &::horizontalGroundedVelocityStop, 0.001f, 2.0f
  );

  ImGui::Separator();
  ImGui::Separator();

  ImGui::End();
}
