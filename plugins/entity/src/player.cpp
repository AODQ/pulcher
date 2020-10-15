#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <imgui/imgui.hpp>

namespace {
  float inputGroundAccelMultiplier = 1.0f;
  float inputAirAccelMultiplier = 0.05f;
  float inputWalkAccelMultiplier = 0.4f;
  float inputCrouchAccelMultiplier = 0.2f;
  float gravity = 0.7f;
  float jumpingHorizontalAccel = 2.0f;
  float jumpingVerticalAccel = 9.5f;
  float jumpingHorizontalTheta = 9.5f;
  float friction = 0.75f;
  float dashMultiplier = 3.0f;
  float dashMinimumVelocity = 16.0f;
  float dashCooldown = 300.0f;
  float horizontalGroundedVelocityStop = 0.5f;
}

void plugin::entity::UpdatePlayer(
  pulcher::plugin::Info const & plugin, pulcher::core::SceneBundle & scene
, pulcher::core::ComponentPlayer & player
, pulcher::animation::ComponentInstance & playerAnim
) {

  auto & registry = scene.EnttRegistry();
  auto & controller = scene.PlayerController().current;
  auto & controllerPrev = scene.PlayerController().previous;

  { // -- process inputs / events

    // -- gravity
    if (!player.grounded)
      { player.velocity.y += ::gravity; }

    // -- process jumping
    player.jumping = controller.jump;

    if (player.grounded && player.jumping) {
      if (
          controller.movementHorizontal
       == pulcher::controls::Controller::Movement::None
      ) {
        player.velocity.y = -::jumpingVerticalAccel;
      } else {

        float thetaRad = glm::radians(::jumpingHorizontalTheta);

        player.velocity.y += -::jumpingHorizontalAccel * glm::sin(thetaRad);

        player.velocity.x +=
          glm::sign(player.velocity.x)
        * ::jumpingHorizontalAccel * glm::cos(thetaRad)
        ;
      }
      player.grounded = false;
    }

    // -- process horizontal movement
    float inputAccel = static_cast<float>(controller.movementHorizontal);

    inputAccel *=
      (player.grounded && !player.jumping) ?
        ::inputGroundAccelMultiplier : ::inputAirAccelMultiplier;

    if (controller.walk) { inputAccel *= ::inputWalkAccelMultiplier; }
    if (controller.crouch) { inputAccel *= ::inputWalkAccelMultiplier; }

    player.velocity.x += inputAccel;

    // -- process friction
    if (player.grounded && !player.jumping) {
      player.velocity.x *= ::friction;
    }

    // -- process horizontal ground stop
    if (
        inputAccel == 0.0f && player.grounded
     && glm::abs(player.velocity.x) < ::horizontalGroundedVelocityStop
    ) {
      player.velocity.x = 0.0f;
    }

    // -- process dashing
    if (player.dashCooldown > 0.0f)
      { player.dashCooldown -= pulcher::util::MsPerFrame(); }

    if (
      !controllerPrev.dash && controller.dash && player.dashCooldown <= 0.0f
    ) {
      float velocityMultiplier = ::dashMultiplier;
      if (glm::length(player.velocity) < ::dashMinimumVelocity) {
        velocityMultiplier = ::dashMinimumVelocity;
      }
      auto direction =
        glm::vec2(
          controller.movementHorizontal, controller.movementVertical
        );

      if (player.grounded) {
        direction.y -= 0.5f;
      }

      spdlog::debug("dir {} nor {} vel {} ", direction, glm::normalize(direction), velocityMultiplier);

      player.velocity += glm::normalize(direction) * velocityMultiplier;
      player.grounded = false;

      player.dashCooldown = ::dashCooldown;
    }
  }

  { // -- apply physics
    glm::vec2 groundedFloorOrigin = player.origin - glm::vec2(0, 2);
    glm::vec2 floorOrigin = player.origin;

    { // free movement check
      auto ray =
        pulcher::physics::IntersectorRay::Construct(
          groundedFloorOrigin
        , groundedFloorOrigin + glm::vec2(player.velocity)
        );
      if (
        pulcher::physics::IntersectionResults results;
        !plugin.physics.IntersectionRaycast(scene, ray, results)
      ) {
        player.origin += player.velocity;
      } else {
        // first 'clamp' the player to some bounds
        if (ray.beginOrigin.x < ray.endOrigin.x) {
          player.origin.x = results.origin.x - 1.0f;
        } else if (ray.beginOrigin.x > ray.endOrigin.x) {
          player.origin.x = results.origin.x + 1.0f;
        }

        if (ray.beginOrigin.y < ray.endOrigin.y) {
          player.origin.y = results.origin.y - 1.0f;
        } else if (ray.beginOrigin.y > ray.endOrigin.y) {
          // the groundedFloorOrigin is -(0, 2), so we need to account for that
          player.origin.y = results.origin.y + 3.0f;
        }

        // then check how the velocity should be redirected
        auto rayY =
          pulcher::physics::IntersectorRay::Construct(
            player.origin + glm::vec2(0.0f, +1.0)
          , player.origin + glm::vec2(0.0f, -3.0f)
          );
        if (
          pulcher::physics::IntersectionResults resultsY;
          plugin.physics.IntersectionRaycast(scene, rayY, resultsY)
        ) {
          // if there is an intersection check
          if (player.origin.y < resultsY.origin.y) {
            player.velocity.y = glm::min(0.0f, player.velocity.y);
          } else if (player.origin.y > resultsY.origin.y) {
            player.velocity.y = glm::max(0.0f, player.velocity.y);
          } else {
            player.velocity.y = 0.0f;
          }
        }

        auto rayX =
          pulcher::physics::IntersectorRay::Construct(
            player.origin + glm::vec2(+2.0f, 0.0)
          , player.origin + glm::vec2(-2.0f, 0.0f)
          );
        if (
          pulcher::physics::IntersectionResults resultsX;
          plugin.physics.IntersectionRaycast(scene, rayX, resultsX)
        ) {
          // if there is an intersection check
          if (player.origin.x < resultsX.origin.x) {
            player.velocity.x = glm::min(0.0f, player.velocity.x);
          } else if (player.origin.x > resultsX.origin.x) {
            player.velocity.x = glm::max(0.0f, player.velocity.x);
          } else {
            player.velocity.x = 0.0f;
          }
        }
      }
    }

    // gravity/ground check
    if (player.velocity.y >= 0.0f)
    {
      auto point =
        pulcher::physics::IntersectorRay::Construct(
          floorOrigin, floorOrigin + glm::vec2(0.0f, 1.0f)
        );
      pulcher::physics::IntersectionResults results;
      player.grounded = plugin.physics.IntersectionRaycast(
        scene, point, results
      );
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

void plugin::entity::UiRenderPlayer(pulcher::core::SceneBundle &) {
  ImGui::Begin("Physics");

  ImGui::Separator();
  ImGui::Separator();

  ImGui::DragFloat("input ground accel", &::inputGroundAccelMultiplier, 0.005f);
  ImGui::DragFloat("input air accel", &::inputAirAccelMultiplier, 0.005f);
  ImGui::DragFloat("input walk accel", &::inputWalkAccelMultiplier, 0.005f);
  ImGui::DragFloat("input crouch accel", &::inputCrouchAccelMultiplier, 0.005f);
  ImGui::DragFloat("gravity", &::gravity, 0.005f);
  ImGui::DragFloat("jump vertical accel", &::jumpingVerticalAccel, 0.005f);
  ImGui::DragFloat("jump hor accel", &::jumpingHorizontalAccel, 0.005f);
  ImGui::DragFloat("jump hor theta", &::jumpingHorizontalTheta, 0.1f);
  ImGui::DragFloat("friction", &::friction, 0.001f);
  ImGui::DragFloat("dashMultiplier", &::dashMultiplier, 0.005f);
  ImGui::DragFloat("dashMinimumVelocity", &dashMinimumVelocity, 0.01f);
  ImGui::DragFloat("dashCooldown (ms)", &::dashCooldown, 0.1f);
  ImGui::DragFloat(
    "horizontal grounded velocity stop"
  , &::horizontalGroundedVelocityStop, 0.005f
  );

  ImGui::Separator();
  ImGui::Separator();

  ImGui::End();
}
