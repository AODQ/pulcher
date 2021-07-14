#include <plugin-base/bot/creature.hpp>

#include <plugin-base/entity/weapon.hpp>
#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-util/random.hpp>

#include <entt/entt.hpp>

namespace {
  float lumpBaseSpeed = 2.0f;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void plugin::bot::UpdateCreatureLump(
  pul::core::SceneBundle & scene
, entt::entity selfEntity
) {
  using Lump = pul::core::ComponentCreatureLump;

  auto & registry = scene.EnttRegistry();

  auto & self = registry.get<pul::core::ComponentCreatureLump>(selfEntity);
  auto & animation =
    registry.get<pul::animation::ComponentInstance>(selfEntity)
  ;
  auto & damageable = registry.get<pul::core::ComponentDamageable>(selfEntity);
  auto & origin = registry.get<pul::util::ComponentOrigin>(selfEntity).origin;

  auto & animationState = animation.instance.pieceToState["body"];

  for (auto & damage : damageable.frameDamageInfos) {
    self.state =
      Lump::StateDodge {
        .hasInitiated = true,
        .airVelocity = damage.directionForce
      };
    damageable.health = glm::max(damageable.health - damage.damage, 0);
  }
  damageable.frameDamageInfos.clear();

  if (self.checkForGrounded) {
    auto const floorRay =
      pul::physics::IntersectorRay::Construct(
        glm::round(origin + glm::vec2(0.0f, 0.0f))
      , glm::round(origin + glm::vec2(0.0f, +10.0f))
    );
    pul::physics::IntersectionResults floorResults;
    plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

    self.checkForGrounded = !floorResults.collision;

    if (!floorResults.collision) {
      origin.y += 10.0f;
    } else {
      origin.y = floorResults.origin.y;
    }
  }

  auto playerView =
    registry.view<
      pul::core::ComponentPlayer
    , pul::util::ComponentOrigin
    >();

  bool shouldDetectPlayer = true;
  bool shouldAttackPlayer = true;

  if (self.movementStateActive) {
    shouldDetectPlayer = false;
    shouldAttackPlayer = false;
    bool shouldSkipAiState = false;

    std::visit(overloaded {
      [&](Lump::MovementStateFlip & /*flip*/) {
        animationState.Apply("turn");
        if (!animationState.animationFinished) {
          shouldSkipAiState = true;
          return;
        }
        self.movementStateActive = false;
        animationState.flip ^= 1;
      },
      [&](Lump::MovementStateSlide & /*slide*/) {
        animationState.Apply("dodge");
        if (animationState.animationFinished) {
          self.movementStateActive = false;
          return;
        }
        shouldSkipAiState = true;
        shouldDetectPlayer = false;
        shouldAttackPlayer = true;
        float const direction = animationState.flip ? 1.0f : -1.0f;
        origin.x += direction * (1.0f - animationState.componentIt/5.0f);
      },
    }, self.movementState
    );

    if (shouldSkipAiState)
      goto SKIP_AI_STATE_VISIT;
  }

  std::visit(overloaded{
    [&](Lump::StateIdle & idle) {
      animationState.Apply("idle");
      self.hasRecentlyAttacked = false;
      if (idle.timer == 0) {
        idle.timer = 50;
        if (pul::util::RandomBool()) {
          self.state = Lump::StateWalk {};
          return;
        } else {
          self.movementStateActive = true;
          self.movementState = Lump::MovementStateFlip {};
          return;
        }
      }
      idle.timer = glm::max(idle.timer - 1, 0);
    }
  , [&](pul::core::ComponentCreatureLump::StateWalk & walk) {
      animationState.Apply("walk");
      float const direction = animationState.flip ? 1.0f : -1.0f;
      origin.x += direction * +0.25f;
      self.checkForGrounded = true;

      auto const wallRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(origin + glm::vec2(direction*8, -38.0f))
        , glm::round(origin + glm::vec2(direction*8, -4.0f))
        );

      pul::physics::IntersectionResults wallResults;
      plugin::physics::IntersectionRaycast(scene, wallRay, wallResults);

      if (wallResults.collision) {
        self.movementStateActive = true;
        self.movementState = Lump::MovementStateFlip {};
        return;
      }

      if (walk.timerUntilIdle == 0) {
        self.state = Lump::StateIdle {};
        return;
      }

      walk.timerUntilIdle = (walk.timerUntilIdle+1) % 400;
    }
  , [&](pul::core::ComponentCreatureLump::StateRush & rush) {
      animationState.Apply("rush");
      auto & targetOrigin =
        registry.get<pul::util::ComponentOrigin>(rush.targetEntity).origin
      ;
      shouldDetectPlayer = false;
      shouldAttackPlayer = true;

      glm::vec2 const dist = glm::abs(targetOrigin - origin);

      // player too far away
      if (dist.x > 32.0f*7.0f || dist.y > 32.0f*3.0f) {
        self.state = Lump::StateIdle {};
        self.movementStateActive = true;
        self.movementState = Lump::MovementStateSlide {};
        return;
      }

      float const direction = animationState.flip ? 1.0f : -1.0f;
      origin.x +=
        direction * ::lumpBaseSpeed * (rush.willChargePlayer ? 1.5f : 1.0f)
      ;
      self.checkForGrounded = true;

      // if close to player, throw up hitbox
      // TODO this is bugged because it will only target a single player
      if (rush.willChargePlayer) {
        shouldAttackPlayer = false;
        if (
          plugin::entity::WeaponDamageCircle(
              scene
            , origin + glm::vec2(0.0f, -12.0f)
            , 32.0f // radius
            , 25.0f, 7.5f // damage / knockback
            , selfEntity
            , glm::vec2(animationState.flip ? 1.0f : -1.0f, -0.5f)
          )
        ) {
          self.state = Lump::StateDodge {};
        }
      } else if (dist.x < 40.0f) {
        self.state = Lump::StateIdle {};
        self.movementStateActive = true;
        self.movementState = Lump::MovementStateSlide {};
        return;
      }

      // if have to flip around, apply the slide first and then idle
      if (animationState.flip ^ (targetOrigin.x > origin.x)) {
        self.state = Lump::StateIdle {};
        self.movementStateActive = true;
        self.movementState = Lump::MovementStateSlide {};
        return;
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateDodge & dodge) {
      shouldDetectPlayer = false;
      shouldAttackPlayer = false;
      animationState.Apply("jump");
      if (!dodge.hasInitiated) {
        dodge.hasInitiated = true;
        self.checkForGrounded = false;
        dodge.airVelocity =
          glm::vec2((animationState.flip ? 1.0f : -1.0f) * -3.0f, -2.0f)
        ;
      }

      dodge.airVelocity.y = glm::max(-6.0f, dodge.airVelocity.y + 0.1f);

      // TODO ceiling check

      auto const floorRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(origin + glm::vec2(0.0f, 0.0f))
        , glm::round(origin + dodge.airVelocity)
      );
      pul::physics::IntersectionResults floorResults;
      plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

      if (dodge.airVelocity.y > 0.0f && floorResults.collision) {
        origin = floorResults.origin;
        self.state = Lump::StateIdle {};
      } else {
        origin += dodge.airVelocity;
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateAttackPoison &) {
      animationState.Apply("attack-poison");
      shouldDetectPlayer = false;
      shouldAttackPlayer = false;
      if (animationState.animationFinished) {
        self.state = Lump::StateIdle { .timer = 300, };
      }

      if (animationState.componentIt >= 8 && animationState.componentIt <= 10) {
        plugin::entity::WeaponDamageCircle(
          scene
        , origin + glm::vec2(0.0f, -16.0f)
        , 32.0f
        , 25.0f, 5.0f
        , selfEntity
        , glm::vec2(animationState.flip ? 1.0f : -1.0f, -2.5f)
        );
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateAttackKick &) {
      animationState.Apply("attack-kick");
      shouldDetectPlayer = false;
      shouldAttackPlayer = false;
      if (animationState.animationFinished) {
        self.state = Lump::StateIdle { .timer = 300, };
      }

      if (animationState.componentIt >= 3 && animationState.componentIt <= 5) {
        plugin::entity::WeaponDamageCircle(
          scene
        , origin + glm::vec2(0.0f, -16.0f)
        , 32.0f
        , 5.0f, 10.0f
        , selfEntity
        , glm::vec2(animationState.flip ? 1.0f : -1.0f, -0.5f)
        );
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateBackflip & flip) {
      shouldDetectPlayer = false;
      shouldAttackPlayer = false;
      animationState.Apply("backflip");
      if (!flip.hasInitiated) {
        flip.hasInitiated = true;
        self.checkForGrounded = false;
        flip.airVelocity =
          glm::vec2((animationState.flip ? 1.0f : -1.0f) * -1.0f, -3.0f)
        ;
      }

      flip.airVelocity.y = glm::max(-6.0f, flip.airVelocity.y + 0.1f);

      // TODO ceiling check

      auto const floorRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(origin + glm::vec2(0.0f, 0.0f))
        , glm::round(origin + flip.airVelocity)
      );
      pul::physics::IntersectionResults floorResults;
      plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

      if (flip.airVelocity.y > 0.0f && floorResults.collision) {
        origin = floorResults.origin;
        self.state = Lump::StateIdle {};
      } else {
        origin += flip.airVelocity;
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateFlee & flee) {
      // TODO probably merge this with Rush at some point? by making Rush more
      // flexible
      animationState.Apply("rush");
      shouldDetectPlayer = false;

      shouldAttackPlayer = flee.timerUntilIdle < 60;

      if (flee.timerUntilIdle == 0) {
        self.state = Lump::StateIdle {};
      }
      flee.timerUntilIdle = glm::max(flee.timerUntilIdle - 1, 0);

      float const direction = animationState.flip ? 1.0f : -1.0f;
      origin.x += direction * ::lumpBaseSpeed;
      self.checkForGrounded = true;
    }
  }, self.state);

  SKIP_AI_STATE_VISIT:

  // if the AI is flipping then don't allow it to detect/rush the player, but
  // it also happens before the AI gets to change any internal state

  if (shouldDetectPlayer || shouldAttackPlayer) {
    for (auto entity : playerView) {
      /* auto & player = view.get<pul::core::ComponentPlayer>(entity); */
      auto const & targetOrigin =
        playerView.get<pul::util::ComponentOrigin>(entity).origin
      ;
      // TODO align this better to entity centers

      glm::vec2 const dist = glm::abs(targetOrigin - origin);

      if (dist.x < 32.0f && dist.y < 32.0f && shouldAttackPlayer) {
        auto rand = pul::util::RandomInt32(0, 100);

        if (rand < 30) {
          self.state = Lump::StateAttackPoison {};
          self.hasRecentlyAttacked = true;
        } else if (rand < 60) {
          self.state = Lump::StateAttackKick {};
          self.hasRecentlyAttacked = true;
        } else if (rand < 70) {
          self.hasRecentlyAttacked = false;
          self.state = Lump::StateDodge {};
        } else if (rand < 95) {
          self.hasRecentlyAttacked = false;
          self.state = Lump::StateBackflip {};
        } else {
          self.hasRecentlyAttacked = false;
          // turn around to flee
          self.movementStateActive = true;
          if (pul::util::RandomBoolBiased(77))
            self.movementState = Lump::MovementStateFlip {};
          self.state = Lump::StateFlee {
            .timerUntilIdle = pul::util::RandomInt32(30, 60*2),
          };
        }

      } else if (
        dist.x < 32.0f*6.0f && dist.y < 32.0f*2.0f && shouldDetectPlayer
      ) {
        // if we have to turn around first
        if (animationState.flip ^ (targetOrigin.x > origin.x)) {
          self.movementStateActive = true;
          self.movementState = Lump::MovementStateFlip {};
        }

        self.state = Lump::StateRush {
          .targetEntity = entity,
          .willChargePlayer = pul::util::RandomBoolBiased(33)
        };
      }
    }
  }

  animation.instance.origin = origin;
}
