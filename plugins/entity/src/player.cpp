#include <plugin-entity/player.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-controls/controls.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <imgui/imgui.hpp>

namespace {
  int32_t maxAirDashes = 3u;
  float inputRunAccelTarget = 4.0f;
  float slideAccel = 1.0f;
  float slideMinimumVelocity = 6.0f;
  float slideCooldown   = 300.0f;
  float slideFriction = 0.95f;
  float slideFrictionTransitionTime = 500.0f;
  float slideFrictionTransitionPow = 1.0f;
  float slopeStepUpHeight   = 12.0f;
  float slopeStepDownHeight = 12.0f;
  float inputRunAccelTime = 1.0f;

  float inputAirAccelPreThresholdTime = 0.8f;
  float inputAirAccelPostThreshold = 0.02f;
  float inputAirAccelThreshold = 6.0f;

  float inputGravityAccelPreThresholdTime = 1.0f;
  float inputGravityAccelPostThreshold = 0.04f;
  float inputGravityAccelThreshold = 7.0f;

  float inputWalkAccelTarget = 1.0f;
  float inputWalkAccelTime = 0.5f;
  float inputCrouchAccelTarget = 1.0f;
  float inputCrouchAccelTime = 1.0f;
  float jumpAfterFallTime = 200.0f;
  float jumpingHorizontalAccel = 4.0f;
  float jumpingHorizontalAccelMax = 6.0f;
  float jumpingVerticalAccel = 6.5f;
  float jumpingHorizontalTheta = 90.0f;
  float frictionGrounded = 0.8f;
  float dashAccel = 1.0f;
  float dashGravityTime = 200.0f;

  struct TransferPercent {
    float same = 1.0f;
    float reverse = 1.0f;
    float degree90 = 1.0f;
  };
  TransferPercent dashVerticalTransfer;
  TransferPercent dashHorizontalTransfer;
  float dashMinimumVelocity = 6.0f;
  float dashCooldown = 1500.0f;
  float horizontalGroundedVelocityStop = 0.5f;


  std::string debugTransferDetails = {};


/*
  calculate requested dash transfer velocity given a direction and a velocity.
  These must in the range (-1 .. +1). This basically applies some amount of
  velocity to be reduced dependent on the player direction, which is in
  relation to the total magnitude of the velocity. As there are two seperate
  dashes (horizontal and vertical), with some mixture allowed between them, if
  a dash is occuring upwards then vertical would be used. For example, if the
  player is moving right quickly and dashes upwards, it would use
  dashVertical.90degree. However if the player is moving up-right at
  approximately the same velocity, the 90 degree would only apply if they dash
  up-left and down-right; left and right would be considered mixtures of
  90degree and same.

  uses horizontal
                      90 degree    same+90
                         |        /
                         |    /
                         | /
  <<< reverse  ----------*----------- > same       | >>>>>>>>
                         |
                         |
                         |
                      90 degree

  uses (horizontal+vertical)*0.5
            reverse                 same
               \      90d+same  ^^^^
                  \      |       /
                    \    |    /
                      \  | /
  <<< rev+90d  ----------*----------- > 90 degree + same
                         |
                         |
                         |
                      90 degree
*/
float CalculateDashTransferVelocity(
  glm::i32vec2 const direction, glm::i32vec2 const velocity
) {
  PUL_ASSERT_CMP(direction, !=, glm::i32vec2(0), return 0.0f;)

  auto & transferV = dashVerticalTransfer;
  auto & transferH = dashHorizontalTransfer;

  float const angleDirection = std::atan2(direction.y, direction.x);
  float const angleVelocity = std::atan2(velocity.y, velocity.x);

  spdlog::debug("angle dir {:.2f} vel {:.2f}", angleDirection, angleVelocity);

  bool directionAxisX = false, directionAxisY = false;

  directionAxisY = direction.y != 0.0f;
  directionAxisX = direction.x != 0.0f;

  float const angleDiff =
    glm::abs(glm::abs(angleDirection) - glm::abs(angleVelocity));
  float angleSum =
    glm::abs(glm::abs(angleDirection) + glm::abs(angleVelocity));

  if (angleSum >= pul::Pi) { angleSum -= pul::Pi; }

  // if same angle, then just use the axis
  if (angleDirection == angleVelocity) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("same angle | X+Y");
      return transferV.same*0.5f + transferH.same*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("same angle | X");
      return transferH.same;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("same angle | Y");
      return transferV.same;
    }
  }

  // if in 90 degree range (not mix)
  if (
      glm::abs(angleSum - pul::Pi_2) < 0.1f
   || glm::abs(angleDiff - pul::Pi_2) < 0.1f
  ) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("90 degree angle | X+Y");
      return transferV.degree90*0.5f + transferH.degree90*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("90 degree angle | X");
      return transferH.degree90;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("90 degree angle | Y");
      return transferV.degree90;
    }
  }

  // if in between 90 degree range & same angle (45 degrees)
  if (glm::abs(angleDiff - pul::Pi_4) < 0.1f) {
    float const transferVMix = transferV.degree90*0.5f + transferV.same*0.5f;
    float const transferHMix = transferH.degree90*0.5f + transferH.same*0.5f;
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("45 degree angle | X+Y");
      return transferVMix*0.5f + transferHMix*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("45 degree angle | X");
      return transferHMix;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("45 degree angle | Y");
      return transferVMix;
    }
  }

  // if reverse angle
  if (
    glm::abs(glm::abs(angleDirection)+glm::abs(angleVelocity) - pul::Pi) < 0.1f
  ) {
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse angle | X+Y");
      return transferV.reverse*0.5f + transferH.reverse*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("reverse angle | X");
      return transferH.reverse;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse angle | Y");
      return transferV.reverse;
    }
  }

  // if in between 90 degree range & reverse angle (45 degrees)
  if (glm::abs(angleDiff - (pul::Pi_2+pul::Pi_4)) < 0.1f) {
    float const transferVMix = transferV.degree90*0.5f + transferV.reverse*0.5f;
    float const transferHMix = transferV.degree90*0.5f + transferV.reverse*0.5f;
    if (directionAxisX && directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | X+Y");
      return transferVMix*0.5f + transferHMix*0.5f;
    } else if (directionAxisX) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | X");
      return transferHMix;
    } else if (directionAxisY) {
      ::debugTransferDetails = fmt::format("reverse 45 degree angle | Y");
      return transferVMix;
    }
  }

  spdlog::critical(
    "could not find an appropiate angle; dir {} vel {} | diff {} sum {}"
  , angleDirection, angleVelocity, angleDiff, angleSum
  );
  return 0.0f;
}

float CalculateAccelFromTarget(float timeMs, float targetTexels) {
  return targetTexels / timeMs / 90.0f * 2.0f;
}

void ApplyGroundedMovement(
  float const facingDirection, float const playerVelocityX
, float const accelTime, float const accelTarget
, bool & inoutFrictionApplies, float & inoutInputAccel
) {
  inoutInputAccel *= CalculateAccelFromTarget(accelTime, accelTarget);

  // check if we have not reached the target, in which case no
  // friction is applied. However if this frame would reach the target,
  // limit the capacity; which is why we use `<=` check instead of `<`
  if (
      glm::abs(playerVelocityX) <= accelTarget
   && (
        playerVelocityX == 0.0f
          ? true : glm::sign(playerVelocityX) == glm::sign(facingDirection)
      )
  ) {
    inoutFrictionApplies = false;
    if (facingDirection*(playerVelocityX + inoutInputAccel) > accelTarget) {
      inoutInputAccel = facingDirection*accelTarget - playerVelocityX;
    }
  }
}

void UpdatePlayerPhysics(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
) {
  glm::vec2 groundedFloorOrigin = player.origin - glm::vec2(0, 2);

  auto ray =
    pul::physics::IntersectorRay::Construct(
      groundedFloorOrigin
    , groundedFloorOrigin + glm::vec2(player.velocity)
    );
  if (
    pul::physics::IntersectionResults results;
    !plugin.physics.IntersectionRaycast(scene, ray, results)
  ) {
    // check if we can step down a slope
    if (player.grounded && player.velocity.x != 0.0f) {
      glm::vec2 offset =
        groundedFloorOrigin + glm::vec2(player.velocity.x, 0.0f);

      auto stepRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(offset + glm::vec2(0.0f, 0.0f))
        , glm::round(offset + glm::vec2(0.0f, ::slopeStepDownHeight))
        );
      if (
        pul::physics::IntersectionResults stepResults;
          plugin.physics.IntersectionRaycast(scene, stepRay, stepResults)
       &&
          static_cast<int32_t>(stepResults.origin.y)
       != static_cast<int32_t>(glm::round(offset.y))
      ) {
        player.origin.x += player.velocity.x;
        player.origin.y = stepResults.origin.y;
        player.grounded = true;
        return;
      }
    }

    player.origin += player.velocity;

  } else {

    // in case we want to store our velocity to attempt slope climbing
    bool attemptStoreSlopeVelocity = false;

    // check if we can step up to the specified slope
    if (player.grounded && player.velocity.x != 0.0f) {
      glm::vec2 offset = ray.endOrigin;

      auto stepRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(offset + glm::vec2(0.0f, -slopeStepUpHeight))
        , glm::round(offset)
        );

      if (
        pul::physics::IntersectionResults stepResults;
        plugin.physics.IntersectionRaycast(scene, stepRay, stepResults)
      ) {
        // it needs to intersect at a pixel that's not the 'ceil' of the step-up
        // height, as otherwise it is not a floor
        if (
            static_cast<int32_t>(glm::round(stepResults.origin.y))
         != static_cast<int32_t>(glm::round(offset.y - slopeStepUpHeight))
        ) {
          player.origin.y = stepResults.origin.y + 1.0f;

          // store velocity
          player.origin.x += player.velocity.x;

          player.grounded = true;
          return;
        }
      }
    }

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
      pul::physics::IntersectorRay::Construct(
        player.origin + glm::vec2(0.0f, +1.0)
      , player.origin + glm::vec2(0.0f, -3.0f)
      );
    if (
      pul::physics::IntersectionResults resultsY;
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
      pul::physics::IntersectorRay::Construct(
        player.origin + glm::vec2(+2.0f, 0.0)
      , player.origin + glm::vec2(-2.0f, 0.0f)
      );
    if (
      pul::physics::IntersectionResults resultsX;
        !attemptStoreSlopeVelocity
      && plugin.physics.IntersectionRaycast(scene, rayX, resultsX)
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

void UpdatePlayerWeapon(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, pul::animation::ComponentInstance &
) {
  /* auto & registry = scene.EnttRegistry(); */
  auto & controller = scene.PlayerController().current;
  auto & controllerPrev = scene.PlayerController().previous;

  if (!controllerPrev.weaponSwitchNext && controller.weaponSwitchNext) {
    player.inventory.ChangeWeapon(
      static_cast<pul::core::WeaponType>(
        (Idx(player.inventory.currentWeapon)+1)
      % Idx(pul::core::WeaponType::Size)
      )
    );
  }

  if (!controllerPrev.weaponSwitchPrev && controller.weaponSwitchPrev) {
    player.inventory.ChangeWeapon(
      static_cast<pul::core::WeaponType>(
        (
          Idx(pul::core::WeaponType::Size)
        + Idx(player.inventory.currentWeapon)-1
        )
      % Idx(pul::core::WeaponType::Size)
      )
    );
  }

  if (!(controller.shoot && !controllerPrev.shoot)) { return; }

  auto & weapon = player.inventory.weapons[Idx(player.inventory.currentWeapon)];
  (void)weapon;
  switch (player.inventory.currentWeapon) {
    default: break;
    case pul::core::WeaponType::Volnias: {
      auto playerOrigin = player.origin - glm::vec2(0, 32.0f);
      auto ray =
        pul::physics::IntersectorRay::Construct(
          playerOrigin
        , playerOrigin + glm::normalize(controller.lookDirection) * 512.0f
        );
      if (
        pul::physics::IntersectionResults results;
        plugin.physics.IntersectionRaycast(scene, ray, results)
      ) {
        if (glm::length(playerOrigin - glm::vec2(results.origin)) < 64.0f) {
          player.velocity +=
            glm::normalize(playerOrigin - glm::vec2(results.origin)) * 10.0f;
          player.grounded = false;
        }
      }
    } break;
  }
}

}

void plugin::entity::UpdatePlayer(
  pul::plugin::Info const & plugin, pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, pul::animation::ComponentInstance & playerAnim
) {

  auto & registry = scene.EnttRegistry();
  auto & controller = scene.PlayerController().current;
  auto & controllerPrev = scene.PlayerController().previous;

  // error checking
  if (glm::abs(player.velocity.x) > 1000.0f) {
    spdlog::error("player velocity too high (for now)");
    player.velocity.x = 0.0f;
  }

  if (glm::abs(player.velocity.y) > 1000.0f) {
    spdlog::error("player velocity too high (for now)");
    player.velocity.y = 0.0f;
  }

  if (
      glm::isnan(player.velocity.x) || glm::isnan(player.velocity.y)
   || glm::isinf(player.velocity.x) || glm::isinf(player.velocity.y)
  ) {
    spdlog::error("floating point corruption on player velocity");
    player.velocity.x = 0.0f;
    player.velocity.y = 0.0f;
    player.storedVelocity.x = 0.0f;
    player.storedVelocity.y = 0.0f;
  }

  bool
    frameVerticalJump   = false
  , frameHorizontalJump = false
  , frameVerticalDash   = false
  , frameHorizontalDash = false
  , frameWalljump       = false
  ;

  bool const prevGrounded = player.grounded;

  // gravity/ground check
  if (player.velocity.y >= 0.0f)
  {
    pul::physics::IntersectorPoint point;
    point.origin = player.origin + glm::vec2(0.0f, 1.0f);
    pul::physics::IntersectionResults results;
    player.grounded = plugin.physics.IntersectionPoint(scene, point, results);
  }

  player.wallClingLeft = false;
  player.wallClingRight = false;

  if (!player.grounded) {
    pul::physics::IntersectorPoint point;
    point.origin = player.origin + glm::vec2(-3.0f, -4.0f);
    pul::physics::IntersectionResults results;
    player.wallClingLeft =
      plugin.physics.IntersectionPoint(scene, point, results);
  }

  if (!player.grounded) {
    pul::physics::IntersectorPoint point;
    point.origin = player.origin + glm::vec2(+3.0f, -4.0f);
    pul::physics::IntersectionResults results;
    player.wallClingRight =
      plugin.physics.IntersectionPoint(scene, point, results);
  }

  bool const frameStartGrounded = player.grounded;

  using MovementControl = pul::controls::Controller::Movement;

  { // -- process inputs / events

    // -- gravity

    // apply only when in air & only if dash zero gravity isn't in effect
    if (player.dashZeroGravityTime <= 0.0f && !player.grounded) {
      if (player.velocity.y <= ::inputGravityAccelThreshold) {
        player.velocity.y +=
          ::CalculateAccelFromTarget(
            ::inputGravityAccelPreThresholdTime, ::inputGravityAccelThreshold
          );
      } else {
        player.velocity.y += ::inputGravityAccelPostThreshold;
      }
    }

    // -- process crouching
    player.crouching = controller.crouch;
    playerAnim.instance.pieceToState["legs"].angle = 0.0f;
    playerAnim.instance.pieceToState["body"].angle = 0.0f;

    bool const prevCrouchSliding = player.crouchSliding;

    if (
        player.crouchSliding
     && (
          !player.crouching
       || glm::abs(player.velocity.x) < inputCrouchAccelTarget
       || player.jumping
        )
    ) {
      player.crouchSliding = false;
    }

    if (player.crouchSlideCooldown > 0.0f)
      { player.crouchSlideCooldown -= pul::util::MsPerFrame; }

    if (player.slideFrictionTime > 0.0f)
      { player.slideFrictionTime -= pul::util::MsPerFrame; }

    // -- process jumping
    player.jumping = controller.jump;

    // set jump timer if falling this frame
    if (prevGrounded && !player.grounded) {
      player.jumpFallTime = ::jumpAfterFallTime;
    }

    if (player.jumpFallTime > 0.0f)
      { player.jumpFallTime -= pul::util::MsPerFrame; }

    if (!player.jumping) {
      player.storedVelocity = player.velocity;
      player.hasReleasedJump = true;
    }

    // make sure that jumping only happens if player has released jump yet
    if (player.grounded && !player.hasReleasedJump) {
      player.jumping = false;
    }

    if ((player.jumpFallTime > 0.0f || player.grounded) && player.jumping) {
      if (controller.movementHorizontal == MovementControl::None) {
        player.velocity.y = -::jumpingVerticalAccel;
        frameVerticalJump = true;
      } else {

        float thetaRad = glm::radians(::jumpingHorizontalTheta);

        player.velocity.y = -::jumpingHorizontalAccel * glm::sin(thetaRad);
        player.velocity.x = player.storedVelocity.x;

        if (glm::abs(player.velocity.x) < jumpingHorizontalAccelMax) {
          player.velocity.x +=
            + glm::sign(static_cast<float>(controller.movementHorizontal))
            * ::jumpingHorizontalAccel * glm::cos(thetaRad)
          ;
        }

        frameHorizontalJump = true;
      }
      player.grounded = false;
      player.hasReleasedJump = false;
      player.jumpFallTime = 0.0f;
    }

    // -- process walljumping

    // with jump you can hold space, this is not true for walljumps tho
    if (player.jumping && !controllerPrev.jump) {
      if (player.wallClingLeft || player.wallClingRight) {
        player.grounded = false;

        float const totalVel = glm::length(player.velocity) + 4.0f;

        float const thetaRad =
          (player.wallClingRight ? -pul::Pi*0.25f : 0.0f) + glm::radians(-75.0f)
        ;

        player.velocity.y = glm::sin(thetaRad) * totalVel;
        player.velocity.x = glm::cos(thetaRad) * totalVel;

        frameWalljump = true;
      }
    }

    // -- process horizontal movement
    float inputAccel = static_cast<float>(controller.movementHorizontal);
    float const facingDirection = inputAccel;

    bool frictionApplies = true;
    bool applyInputAccel = true;

    if (player.crouchSliding) {
      applyInputAccel = false;
    } else if (player.grounded) {
      if (player.jumping) {
        // this frame the player will jump
        frictionApplies = false;
      } else if (controller.movementHorizontal != MovementControl::None) {
        if (controller.walk) {
          ApplyGroundedMovement(
            facingDirection, player.velocity.x
          , ::inputWalkAccelTime, ::inputWalkAccelTarget
          , frictionApplies, inputAccel
          );
        } else if (controller.crouch) {
          if (
              (prevGrounded ? !controllerPrev.crouch : !prevCrouchSliding)
            && player.crouchSlideCooldown <= 0.0f
          ) {
            player.crouchSliding = true;
            applyInputAccel = false;

            // apply crouch boost, similar to dash boost
            player.velocity.x =
              glm::max(
                ::slideAccel + glm::length(player.velocity.x)
              , ::slideMinimumVelocity
              ) * facingDirection
            ;

            player.crouchSlideCooldown = ::slideCooldown;
            player.slideFrictionTime = ::slideFrictionTransitionTime;
          } else {
            ApplyGroundedMovement(
              facingDirection, player.velocity.x
            , ::inputCrouchAccelTime, ::inputCrouchAccelTarget
            , frictionApplies, inputAccel
            );
          }
        } else {
          ApplyGroundedMovement(
            facingDirection, player.velocity.x
          , ::inputRunAccelTime, ::inputRunAccelTarget
          , frictionApplies, inputAccel
          );
        }
      }
    } else {
      if (facingDirection*player.velocity.x <= ::inputAirAccelThreshold) {
        inputAccel *=
          ::CalculateAccelFromTarget(
            ::inputAirAccelPreThresholdTime
          , ::inputAirAccelThreshold
          );
      } else {
        inputAccel *= ::inputAirAccelPostThreshold;
      }

      frictionApplies = false;

      // friction applies if player is crouching and moving opposite direction
      // while in air
      if (
          glm::sign(player.velocity.x) != 0.0f
       && glm::sign(inputAccel) != 0.0f
       && player.crouching
       && glm::sign(inputAccel) != glm::sign(player.velocity.x)
       ) {
        frictionApplies = true;
       }
    }

    if (applyInputAccel)
      { player.velocity.x += inputAccel; }

    // -- process friction
    if (frictionApplies) {
      if (player.crouchSliding) {
        float const friction =
          glm::mix(
            ::slideFriction, 1.0f
          , player.slideFrictionTime <= 0.0f
              ? 0.0f
              : glm::pow(
                player.slideFrictionTime / ::slideFrictionTransitionTime
              , ::slideFrictionTransitionPow
              )
          );
        player.velocity.x *= friction;
      } else {
        player.velocity.x *= ::frictionGrounded;
      }
    }

    // -- process horizontal ground stop
    if (
        inputAccel == 0.0f && player.grounded
     && glm::abs(player.velocity.x) < ::horizontalGroundedVelocityStop
    ) {
      player.velocity.x = 0.0f;
    }

    // -- process dashing
    for (auto & dashCooldown : player.dashCooldown) {
      if (dashCooldown > 0.0f)
        { dashCooldown -= pul::util::MsPerFrame; }
    }

    // clear dash lock if either we land, or we are jumping on this frame (thus
    // player will not be grounded)
    if (
        !prevGrounded
     && (frameHorizontalJump || frameVerticalJump || player.grounded)
    ) {
      for (auto & dashLock : player.dashLock)
        { dashLock = false; }
    }
    if (player.dashZeroGravityTime > 0.0f) {
      player.dashZeroGravityTime -= pul::util::MsPerFrame;
      // if grounded then gravity time has been nullified
      if (player.grounded || !controller.dash)
        { player.dashZeroGravityTime = 0.0f; }
    }

    // player has a limited amount of dashes in air, so reset that if grounded
    // at the start of frame
    if (frameStartGrounded)
      { player.midairDashesLeft = ::maxAirDashes; }

    if (
      !controllerPrev.dash && controller.dash
    && player.dashCooldown[Idx(controller.movementDirection)] <= 0.0f
    && player.dashLock[Idx(controller.movementDirection)] == false
    && player.midairDashesLeft > 0
    ) {
      glm::vec2 const scaledVelocity =
          player.velocity
        * glm::vec2(1,1
          /* ::dashVerticalHorizontalPercent */
        /* , 1.0f - ::dashVerticalHorizontalPercent */
        );

      float const velocityMultiplier =
        glm::max(
          ::dashAccel + glm::length(scaledVelocity), ::dashMinimumVelocity
        );

      if (controller.movementVertical != MovementControl::None) {
        frameVerticalDash = true;
      } else {
        frameHorizontalDash = true;
      }

      auto direction =
        glm::vec2(
          controller.movementHorizontal, controller.movementVertical
        );

      float const transfers =
        CalculateDashTransferVelocity(
          glm::i32vec2(
            static_cast<int32_t>(controller.movementHorizontal)
          , static_cast<int32_t>(controller.movementVertical)
          )
        , glm::i32vec2(
            static_cast<int32_t>(glm::sign(player.velocity.x))
          , static_cast<int32_t>(glm::sign(player.velocity.y))
          )
        );

      // if no keys are pressed just guess where player wants to go
      if (
          controller.movementHorizontal == MovementControl::None
       && controller.movementVertical == MovementControl::None
      ) {
        direction.x = player.velocity.x >= 0.0f ? +1.0f : -1.0f;
      }

      if (player.grounded) {
        player.origin.y -= 8.0f;
      }

      player.velocity =
        velocityMultiplier * transfers * glm::normalize(direction);
      player.grounded = false;

      player.dashCooldown[Idx(controller.movementDirection)] = ::dashCooldown;
      player.dashLock[Idx(controller.movementDirection)] = true;
      player.dashZeroGravityTime = ::dashGravityTime;
      -- player.midairDashesLeft;
    }
  }

  ::UpdatePlayerPhysics(plugin, scene, player);

  const float velocityXAbs = glm::abs(player.velocity.x);

  { // -- apply animations

    // -- set leg animation
    auto & legInfo = playerAnim.instance.pieceToState["legs"];
    auto & bodyInfo = playerAnim.instance.pieceToState["body"];

    if (player.grounded) { // grounded animations
      if (!player.crouchSliding) {
        bodyInfo.Apply("center");
        bodyInfo.angle = 0.0f;
      }

      if (!player.crouching && (!prevGrounded || player.landing)) {
        player.landing = true;
        auto & stateInfo = playerAnim.instance.pieceToState["legs"];
        stateInfo.Apply("landing");
        if (stateInfo.animationFinished) { player.landing = false; }
      } else {
        // check walk/run animation turns before applying stand/walk/run
        bool const applyTurning = false;

        bool const moving =
          controller.movementHorizontal != MovementControl::None
        ;

        bool const running = !controller.walk && !controller.crouch && moving;
        bool const crouching = controller.crouch && moving;
        bool const walking = controller.walk && !controller.crouch && moving;

        if (legInfo.label == "run-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("run"); }
        } else if (legInfo.label == "walk-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("walk"); }
        } else if (walking && velocityXAbs <= inputWalkAccelTarget) {
          legInfo.Apply(applyTurning ? "walk-turn" : "walk");
        } else if (crouching && velocityXAbs <= inputCrouchAccelTarget) {
          legInfo.Apply("crouch-walk");
        } else if (running && velocityXAbs <= inputRunAccelTarget) {
          legInfo.Apply(applyTurning ? "run-turn" : "run");
        } else {
          if (player.crouchSliding) {
            legInfo.Apply("crouch-slide");
            if (legInfo.componentIt > 0) {
              legInfo.angle = pul::Pi;
              bodyInfo.Apply("crouch-center");
              bodyInfo.angle =
                (player.velocity.x > 0.0f ? -1.0f : +1.0f) * pul::Pi/1.5f;
            } else {
              // first frame is transition crouch
              legInfo.angle =
                (player.velocity.x > 0.0f ? +1.0f : -1.0f) * pul::Pi/2.0f;
              bodyInfo.Apply("crouch-transition");
            }
          } else if (controller.crouch)
            { legInfo.Apply("crouch-idle"); }
          else
            { legInfo.Apply("stand"); }
        }
      }
    } else { // air animations

      playerAnim.instance.pieceToState["body"].Apply("center");

      if (frameVerticalJump) {
        playerAnim.instance.pieceToState["legs"].Apply("jump-high", true);
      } else if (frameHorizontalJump) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "jump-strafe-0" : "jump-strafe-1");
      } else if (frameVerticalDash) {
        playerAnim.instance.pieceToState["legs"].Apply("dash-vertical");
      } else if (frameHorizontalDash) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "dash-horizontal-0" : "dash-horizontal-1");
      } else if (frameWalljump) {
        static bool swap = false;
        swap ^= 1;
        playerAnim
          .instance.pieceToState["legs"]
          .Apply(swap ? "walljump-0" : "walljump-1");
      } else if (prevGrounded) {
        // logically can only have falled down
        playerAnim.instance.pieceToState["legs"].Apply("air-idle");
      } else {
        if (legInfo.label == "dash-vertical" && legInfo.animationFinished) {
          // switch to air idle
          playerAnim.instance.pieceToState["legs"].Apply("air-idle");
        }
      }
    }

    // -- arm animation
    if (player.grounded) {
      if (controller.crouch) {
        playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
        playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
      }
      else if (legInfo.label == "walk" || legInfo.label == "walk-turn") {
        playerAnim.instance.pieceToState["arm-back"].Apply("unequip-walk");
        playerAnim.instance.pieceToState["arm-front"].Apply("unequip-walk");
      }
      else if (legInfo.label == "run" || legInfo.label == "run-turn") {
        playerAnim.instance.pieceToState["arm-back"].Apply("unequip-run");
        playerAnim.instance.pieceToState["arm-front"].Apply("unequip-run");
      } else {
        playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
        playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
      }
    } else {
      playerAnim.instance.pieceToState["arm-back"].Apply("alarmed");
      playerAnim.instance.pieceToState["arm-front"].Apply("alarmed");
    }

    bool playerDirFlip = playerAnim.instance.pieceToState["legs"].flip;

    if (
        controller.movementHorizontal
     == pul::controls::Controller::Movement::Right
    ) {
      playerDirFlip = true;
    }
    else if (
        controller.movementHorizontal
     == pul::controls::Controller::Movement::Left
    ) {
      playerDirFlip = false;
    }

    playerAnim.instance.pieceToState["legs"].flip = playerDirFlip;

    auto const & currentWeaponInfo =
      pul::core::weaponInfo[Idx(player.inventory.currentWeapon)];

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
        registry.get<pul::animation::ComponentInstance>(
          player.weaponAnimation
        ).instance;

      // nothing should render if unarmed
      weaponAnimation.visible =
        player.inventory.currentWeapon != pul::core::WeaponType::Unarmed;

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

  ::UpdatePlayerWeapon(plugin, scene, player, playerAnim);
}

void plugin::entity::UiRenderPlayer(
  pul::core::SceneBundle & scene
, pul::core::ComponentPlayer & player
, pul::animation::ComponentInstance &
) {
  ImGui::Begin("Physics");

  ImGui::Separator();
  ImGui::Separator();

  ImGui::PushItemWidth(74.0f);
  ImGui::DragFloat("run target time", &::inputRunAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while running; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputRunAccelTime, ::inputRunAccelTarget)
  );
  ImGui::DragFloat("run target accel", &::inputRunAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while running, once this has been reached\n"
    "the player will no longer be able to run any faster"
  );
  ImGui::DragFloat("walk target time", &::inputWalkAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while walking; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputWalkAccelTime, ::inputWalkAccelTarget)
  );
  ImGui::DragFloat("walk target accel", &::inputWalkAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while walking, once this has been reached\n"
    "the player will no longer be able to walk any faster"
  );

  ImGui::DragFloat("crouch target time", &::inputCrouchAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach target from a base velocity of 0\n"
    "while crouching; currently {:.3f} texels/frame will be added; milliseconds"
  , ::CalculateAccelFromTarget(::inputCrouchAccelTime, ::inputCrouchAccelTarget)
  );
  ImGui::DragFloat("crouch target accel", &::inputCrouchAccelTarget, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration target while crouching, once this has been reached\n"
    "the player will no longer be able to crouch any faster"
  );

  ImGui::DragFloat(
    "horizontal grounded velocity stop"
  , &::horizontalGroundedVelocityStop, 0.005f
  );
  pul::imgui::ItemTooltip(
    "if the player is grounded and below this threshold, the velocity will be\n"
    "instantly set to 0"
  );

  ImGui::DragFloat(
    "gravity accel pre-threshold time", &inputGravityAccelPreThresholdTime, 0.5f
  );
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach gravity threshold from a base velocity\n"
    "of 0 while in mid-air; currently {:.3f} texels/frame will be added;\n"
    "milliseconds"
  , ::CalculateAccelFromTarget(
      ::inputGravityAccelPreThresholdTime, ::inputGravityAccelThreshold
    )
  );
  ImGui::DragFloat(
    "gravity accel post-threshold", &inputGravityAccelPostThreshold, 0.005f
  );
  pul::imgui::ItemTooltip(
    "acceleration added per frame of gravity when velocity Y is post-threshold"
  );
  ImGui::DragFloat(
    "gravity accel threshold", &inputGravityAccelThreshold, 0.05f
  );
  pul::imgui::ItemTooltip(
    "acceleration target while in mid-air, once this has been reached\n"
    "the player will use the gravity accel post-threshold up to 'infinity'"
  );

  ImGui::DragFloat(
    "air accel pre-threshold time", &inputAirAccelPreThresholdTime, 0.005f
  );
  pul::imgui::ItemTooltip(
    "amount of time it takes to reach air threshold from a base velocity of 0\n"
    "while in mid-air; currently {:.3f} texels/frame will be added;\n"
    "milliseconds"
  , ::CalculateAccelFromTarget(
      ::inputAirAccelPreThresholdTime, ::inputAirAccelThreshold
    )
  );
  ImGui::DragFloat(
    "air accel post-threshold", &inputAirAccelPostThreshold, 0.005f
  );
  pul::imgui::ItemTooltip(
    "acceleration added per frame when velocity in air is post-threshold"
  );
  ImGui::DragFloat(
    "air accel threshold", &inputAirAccelThreshold, 0.05f
  );
  pul::imgui::ItemTooltip(
    "acceleration target while in mid-air, once this has been reached\n"
    "the player will use the air accel post-threshold up to 'infinity'"
  );

  pul::imgui::ItemTooltip(
    "acceleration added downwards per frame from gravity; texels"
  );
  ImGui::DragFloat("jump vertical accel", &::jumpingVerticalAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration upwards set the frame a vertical jump is made; texels"
  );
  ImGui::DragFloat("jump hor theta", &::jumpingHorizontalTheta, 0.1f);
  pul::imgui::ItemTooltip(
    "the angle which horizontal jump is directed towards; degrees"
  );
  ImGui::DragFloat("jump hor accel", &::jumpingHorizontalAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration horizontally set the frame a horizontal jump is made; texels"
  );
  ImGui::DragFloat(
    "jump hor accel limit", &::jumpingHorizontalAccelMax, 0.005f
  );
  pul::imgui::ItemTooltip(
    "maximum horizontal velocity before the 'jump hor accel' is removed\n"
    "thus further horizontal jumps do not add a horizontal acceleration; texels"
  );
  ImGui::DragFloat(
    "jump after fall time", &::jumpAfterFallTime, 0.005f
  );
  pul::imgui::ItemTooltip(
    "maximum time after falling off a ledge to allow a jump to occur; ms\n"
  );

  ImGui::DragFloat("friction grounded", &::frictionGrounded, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is grounded"
  );
  ImGui::DragFloat("dash accel", &::dashAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "amount to add to velocity the frame a dash is made; texels"
  );

  ImGui::SliderFloat(
    "dashVerticalTransfer.same", &dashVerticalTransfer.same, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in same direction"
  );
  ImGui::SliderFloat(
    "dashVerticalTransfer.reverse", &dashVerticalTransfer.reverse, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in reverse direction"
  );
  ImGui::SliderFloat(
    "dashVerticalTransfer.degree90", &dashVerticalTransfer.degree90, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply vertical dash if dashing in 90-degree direction"
  );

  ImGui::SliderFloat(
    "dashHorizontalTransfer.same", &dashHorizontalTransfer.same, 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in same direction"
  );
  ImGui::SliderFloat(
    "dashHorizontalTransfer.reverse", &dashHorizontalTransfer.reverse
  , 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in reverse direction"
  );
  ImGui::SliderFloat(
    "dashHorizontalTransfer.degree90", &dashHorizontalTransfer.degree90
  , 0.0f, 1.0f
  );
  pul::imgui::ItemTooltip(
    "percentage to apply horizontal dash if dashing in 90-degree direction"
  );

  ImGui::DragFloat("dash min velocity", &::dashMinimumVelocity, 0.01f);
  pul::imgui::ItemTooltip(
    "gives a minimal threshold for velocity on dash, thus ensuring that on\n"
    "dash, the player is moving by at least this velocity; texels"
  );
  ImGui::DragInt("max air dashes", &::maxAirDashes, 0.25f, 0, 10);
  pul::imgui::ItemTooltip("maximum amount of dashes that can occur in air");

  ImGui::DragFloat("dash cooldown", &::dashCooldown, 0.1f);
  pul::imgui::ItemTooltip(
    "minimal amount of time needed to transpire between each dash; milliseconds"
  );

  ImGui::DragFloat("dash zero gravity time", &::dashGravityTime, 0.1f);
  pul::imgui::ItemTooltip(
    "amount of time after a dash that no gravity will be applied; ms"
  );

  ImGui::DragFloat("slope step-up height", &::slopeStepUpHeight, 0.005f);
  pul::imgui::ItemTooltip(
    "the amount of texels to check if player can climb up a slope just by\n"
    "traversing it; texels"
  );

  ImGui::DragFloat("slope step-down height", &::slopeStepDownHeight, 0.005f);
  pul::imgui::ItemTooltip(
    "the amount of texels to check if player can climb down a slope just by\n"
    "traversing it; texels"
  );

  ImGui::DragFloat("slide accel", &::slideAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "amount to add to velocity the frame a slide is made; texels"
  );
  ImGui::DragFloat("slide min velocity", &::slideMinimumVelocity, 0.01f);
  pul::imgui::ItemTooltip(
    "gives a minimal threshold for velocity on slide, thus ensuring that on\n"
    "slide, the player is moving by at least this velocity; texels"
  );
  ImGui::DragFloat("slide cooldown", &::slideCooldown, 0.05f);
  pul::imgui::ItemTooltip("minimal amount of time between sliding; ms");

  ImGui::DragFloat("friction slide", &::slideFriction, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is sliding"
  );
  ImGui::DragFloat(
    "slideFrictionTransitionTime", &::slideFrictionTransitionTime, 0.05f
  );
  pul::imgui::ItemTooltip(
    "time it takes to transition from 0-friction to to slide friction\n"
    "while sliding; ms");
  ImGui::DragFloat(
    "slideFrictionTransitionPow", &::slideFrictionTransitionPow, 0.05f
  );
  pul::imgui::ItemTooltip("the pow exponent/curve in friction transition");

  ImGui::PopItemWidth();

  ImGui::Separator();
  ImGui::Separator();

  ImGui::End();

  ImGui::Begin("HUD (debug/wip)");

  pul::imgui::Text(
    "Speed (horizontal) {}", static_cast<int32_t>(player.velocity.x*90.0f)
  );
  pul::imgui::Text(
    "Speed (vertical) {}", static_cast<int32_t>(player.velocity.y*90.0f)
  );
  pul::imgui::Text(
    "Speed (total) {}", static_cast<int32_t>(glm::length(player.velocity)*90.0f)
  );

  pul::imgui::Text("debug transfer details: {}", ::debugTransferDetails);

  static bool showVel = true;

  ImGui::Checkbox("show velocity (might cause lag)", &showVel);

  if (showVel) {
    static std::vector<float> velocities;

    if (velocities.size() == 0ul) { velocities.resize(512); }

    if (scene.numCpuFrames != 0ul) {
      std::rotate(velocities.begin(), velocities.begin()+1, velocities.end());
      velocities.back() = glm::length(player.velocity)*90.0f;
    }

    ImGui::PlotLines("velocity tracker", velocities.data(), velocities.size());
  }

  ImGui::End();
}
