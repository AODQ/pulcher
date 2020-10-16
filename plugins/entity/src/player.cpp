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
  int32_t maxAirDashes = 1u;
  float inputGroundAccelTime = 244.0f;
  float inputGroundAccel = 0.335f;
  float inputAirAccel = 0.05f;
  float inputWalkAccelMultiplier = 0.4f;
  float inputCrouchAccelMultiplier = 0.2f;
  float gravity = 0.3f;
  float jumpingHorizontalAccel = 6.0f;
  float jumpingHorizontalAccelMax = 7.0f;
  float jumpingVerticalAccel = 9.0f;
  float jumpingHorizontalTheta = 65.0f;
  float frictionAir = 1.0f;
  float frictionGrounded = 0.9f;
  float dashAccel = 1.0f;
  float dashMinimumVelocity = 6.0f;
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
  ;

  bool const prevGrounded = player.grounded;

  // gravity/ground check
  if (player.velocity.y >= 0.0f)
  {
    pulcher::physics::IntersectorPoint point;
    point.origin = player.origin + glm::vec2(0.0f, 1.0f);
    pulcher::physics::IntersectionResults results;
    player.grounded = plugin.physics.IntersectionPoint(scene, point, results);
  }

  bool const frameStartGrounded = player.grounded;

  using MovementControl = pulcher::controls::Controller::Movement;

  { // -- process inputs / events

    // -- gravity
    if (!player.grounded)
      { player.velocity.y += ::gravity; }

    // -- process jumping
    player.jumping = controller.jump;

    if (!player.jumping) {
      player.storedVelocity = player.velocity;
      player.hasReleasedJump = true;
    }

    // make sure that jumping only happens if player has released jump yet
    if (player.grounded && !player.hasReleasedJump) {
      player.jumping = false;
    }

    if (player.grounded && player.jumping) {
      if (controller.movementHorizontal == MovementControl::None) {
        player.velocity.y = -::jumpingVerticalAccel;
        frameVerticalJump = true;
      } else {

        float thetaRad = glm::radians(::jumpingHorizontalTheta);

        player.velocity.y += -::jumpingHorizontalAccel * glm::sin(thetaRad);
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
    }

    // -- process horizontal movement
    float inputAccel = static_cast<float>(controller.movementHorizontal);

    if (
        controllerPrev.movementHorizontal != controller.movementHorizontal
     || controller.movementHorizontal == MovementControl::None
    ) {
      player.runTimer = 0.0f;
    } else {
      player.runTimer =
        glm::min(
          player.runTimer + pulcher::util::MsPerFrame()
        , ::inputGroundAccelTime
        );
    }

    if (player.grounded) {
      inputAccel *=
        (player.runTimer / ::inputGroundAccelTime) * ::inputGroundAccel
      ;
    } else {
      inputAccel *= ::inputAirAccel;
    }

    if (controller.walk) { inputAccel *= ::inputWalkAccelMultiplier; }
    if (controller.crouch) { inputAccel *= ::inputCrouchAccelMultiplier; }

    player.velocity.x += inputAccel;

    // -- process friction
    player.velocity.x *=
      player.grounded && !player.jumping ? ::frictionGrounded : ::frictionAir
    ;

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

    // player has a limited amount of dashes in air, so reset that if grounded
    // at the start of frame
    if (frameStartGrounded)
      { player.midairDashesLeft = ::maxAirDashes; }

    if (
      !controllerPrev.dash && controller.dash
    && player.dashCooldown <= 0.0f
    && player.midairDashesLeft > 0
    ) {
      float const velocityMultiplier =
        glm::max(
          ::dashAccel + glm::length(player.velocity), ::dashMinimumVelocity
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

      // if no keys are pressed just guess where player wants to go
      if (
          controller.movementHorizontal == MovementControl::None
       && controller.movementVertical == MovementControl::None
      ) {
        direction.x = player.velocity.x >= 0.0f ? +1.0f : -1.0f;
      }

      if (player.grounded) { direction.y -= 0.5f; }

      player.velocity = velocityMultiplier * glm::normalize(direction);
      player.grounded = false;

      player.dashCooldown = ::dashCooldown;
      -- player.midairDashesLeft;
    }
  }

  { // -- apply physics
    glm::vec2 groundedFloorOrigin = player.origin - glm::vec2(0, 2);

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

  }

  const float velocityXAbs = glm::abs(player.velocity.x);

  { // -- apply animations

    // -- set leg animation

    if (player.grounded) { // grounded animations
      if (!prevGrounded || player.landing) {
        player.landing = true;
        auto & stateInfo = playerAnim.instance.pieceToState["legs"];
        stateInfo.Apply("landing");
        if (stateInfo.animationFinished) { player.landing = false; }
      } else if (controller.crouch) {
        if (velocityXAbs < 0.1f) {
          playerAnim.instance.pieceToState["legs"].Apply("crouch-idle");
        } else {
          playerAnim.instance.pieceToState["legs"].Apply("crouch-walk");
        }
      } else {
        // check walk/run animation turns before applying stand/walk/run
        auto & legInfo = playerAnim.instance.pieceToState["legs"];

        bool const applyTurning =
            controller.movementHorizontal != MovementControl::None
         && (
             glm::sign(static_cast<float>(controller.movementHorizontal))
          != glm::sign(player.velocity.x)
            )
         && player.velocity.x < 4.0f
        ;

        if (legInfo.label == "run-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("run"); }
        } else if (legInfo.label == "walk-turn") {
          if (legInfo.animationFinished) { legInfo.Apply("walk"); }
        } else if (velocityXAbs < 0.1f) {
          playerAnim.instance.pieceToState["legs"].Apply("stand");
        }
        else if (velocityXAbs < 1.5f) {
          legInfo.Apply(applyTurning ? "walk-turn" : "walk");
        } else {
          legInfo.Apply(applyTurning ? "run-turn" : "run");
        }
      }
    } else { // air animations

      if (frameVerticalJump) {
        playerAnim.instance.pieceToState["legs"].Apply("jump-high", true);
      } else if (frameHorizontalJump) {
        static bool swap = false;
        swap ^= 1;
        spdlog::debug("swap {}", swap);
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
      } else if (frameStartGrounded) {
        // logically can only have falled down
        playerAnim.instance.pieceToState["legs"].Apply("air-idle");
      } else {
      }
    }

    // -- arm animation
    if (player.grounded) {
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

void plugin::entity::UiRenderPlayer(
  pulcher::core::SceneBundle &
, pulcher::core::ComponentPlayer & player
, pulcher::animation::ComponentInstance & playerAnim
) {
  ImGui::Begin("Physics");

  ImGui::Separator();
  ImGui::Separator();

  ImGui::PushItemWidth(74.0f);
  ImGui::DragFloat("input ground accel", &::inputGroundAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "base acceleration added to velocity per frame when on\n"
    "ground (no matter if walking/crouch); texels"
  );
  ImGui::DragFloat("input ground time", &::inputGroundAccelTime, 0.005f);
  pul::imgui::ItemTooltip(
    "time it takes to reach max ground acceleration; milliseconds"
  );
  ImGui::DragFloat("input walk accel", &::inputWalkAccelMultiplier, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration to multiply 'input ground accel' by when walking"
  );
  ImGui::DragFloat("input crouch accel", &::inputCrouchAccelMultiplier, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration to multiply 'input ground accel' by when crouching"
  );
  ImGui::DragFloat(
    "horizontal grounded velocity stop"
  , &::horizontalGroundedVelocityStop, 0.005f
  );
  pul::imgui::ItemTooltip(
    "if the player is grounded and below this threshold, the velocity will be\n"
    "instantly set to 0"
  );
  ImGui::DragFloat("input air accel", &::inputAirAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "base acceleration added to velocity per frame when in air; texels"
  );
  ImGui::DragFloat("gravity", &::gravity, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration added downwards per frame from gravity; texels"
  );
  ImGui::DragFloat("jump vertical accel", &::jumpingVerticalAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "acceleration upwards set the frame a vertical jump is made; texels"
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
  ImGui::DragFloat("jump hor theta", &::jumpingHorizontalTheta, 0.1f);
  pul::imgui::ItemTooltip(
    "the angle which horizontal jump is directed towards; degrees"
  );
  ImGui::DragFloat("friction grounded", &::frictionGrounded, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is grounded"
  );
  ImGui::DragFloat("friction air", &::frictionAir, 0.001f);
  pul::imgui::ItemTooltip(
    "amount to multiply velocity by when player is in air"
  );
  ImGui::DragFloat("dash accel", &::dashAccel, 0.005f);
  pul::imgui::ItemTooltip(
    "amount to add to velocity the frame a dash is made; texels"
  );
  ImGui::DragFloat("dash min velocity", &dashMinimumVelocity, 0.01f);
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

  static bool showVel = true;

  ImGui::Checkbox("show velocity (might cause lag)", &showVel);

  if (showVel) {
    static std::vector<float> velocities;
    if (velocities.size() == 0ul) velocities.resize(512);
    std::rotate(velocities.begin(), velocities.begin()+1, velocities.end());
    velocities.back() = glm::length(player.velocity)*90.0f;


    ImGui::PlotLines("velocity tracker", velocities.data(), velocities.size());
  }

  ImGui::End();
}
