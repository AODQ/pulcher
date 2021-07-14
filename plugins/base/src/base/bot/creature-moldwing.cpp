#include <plugin-base/bot/creature.hpp>

#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-util/random.hpp>

namespace {
  float mwSpeed = 2.0f;
  float mwAttackSpeed = 4.0f;
  float mwAttackTime  = 1000.0f;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void plugin::bot::UpdateCreatureMoldWing(
  pul::core::SceneBundle & scene
, entt::entity selfEntity
) {

  using MoldWing = pul::core::ComponentCreatureMoldWing;

  auto & registry = scene.EnttRegistry();

  auto & self = registry.get<pul::core::ComponentCreatureMoldWing>(selfEntity);

  auto & animation =
    registry.get<pul::animation::ComponentInstance>(selfEntity)
  ;
  auto & damageable = registry.get<pul::core::ComponentDamageable>(selfEntity);
  auto & origin = registry.get<pul::util::ComponentOrigin>(selfEntity).origin;

  auto & animationState = animation.instance.pieceToState["body"];

  for (auto & damage : damageable.frameDamageInfos) {
    damageable.health = glm::max(damageable.health - damage.damage, 0);
  }
  damageable.frameDamageInfos.clear();

  entt::entity playerSpottedEntity = entt::null;

  auto playerView =
    registry.view<
      pul::core::ComponentPlayer
    , pul::util::ComponentOrigin
    >();

  for (auto entity : playerView) {
    auto const & targetOrigin =
      playerView.get<pul::util::ComponentOrigin>(entity).origin
    ;

    glm::vec2 const dist = glm::abs(targetOrigin - origin);

    if (glm::length(dist) < 640.0f) {
      playerSpottedEntity = entity;
      break;
    }
  }

  std::visit(overloaded {
    [&](MoldWing::StateIdle & idle) {
      // enter hanging state if player is spotted

      idle.hanging = idle.hanging || playerSpottedEntity != entt::null;

      if (!idle.hanging) {
        animationState.Apply("idle");

        // TODO add timer
        // sometimes (rarely) the bat will leave of its own volition
        if (pul::util::RandomBoolBiased(1)) {
          idle.hanging = true;
        }
        return;
      }

      // in hanging state
      animationState.Apply("hanging-drop");
      if (animationState.animationFinished) {

        glm::vec2 const target =
          glm::vec2(
            pul::util::RandomInt32(-512, +512),
            pul::util::RandomInt32(-32.0f, -8.0f)
          )
        ;

        self.state = MoldWing::StateRoam {
          .targetOrigin = origin + target,
        };
      }
    },
    [&](MoldWing::StateRoam & roam) {
      animationState.Apply("fly");

      animationState.flip = roam.targetOrigin.x > origin.x;
      origin += glm::sign(roam.targetOrigin - origin) * ::mwSpeed;

      glm::vec2 playerTargetOrigin = glm::vec2(-9999.0f);
      if (playerSpottedEntity != entt::null) {
        playerTargetOrigin =
          playerView.get<pul::util::ComponentOrigin>(playerSpottedEntity).origin
        ;
      }

      glm::vec2 const playerDifference = playerTargetOrigin - origin;
      if (
          playerSpottedEntity != entt::null
       && glm::abs(playerDifference).x < 128.0f
       && playerDifference.y > 0 && playerDifference.y < 32.0f
      ) {
        self.state = MoldWing::StateRush {
          .targetDirection = glm::sign(origin.x - roam.targetOrigin.x),
          .timer = 0.0f,
        };
        return;
      }

      if (glm::length(roam.targetOrigin - origin) < 8.0f) {
        roam.targetOrigin =
          origin
        + glm::vec2(pul::util::RandomInt32(-512, +512), 0.0f)
        ;
      }
    },
    [&](MoldWing::StateRush & rush) {
      animationState.Apply("dash");

      // create an arc for the moldwing to travel
      rush.timer += scene.calculatedMsPerFrame;
      float theta = pul::Pi_2 + rush.timer / ::mwAttackTime * pul::Pi;

      if (rush.targetDirection < 0.0f)
        theta = theta * -1.0f;

      origin +=
          glm::vec2(glm::cos(theta), glm::sin(theta))
        * rush.targetDirection * ::mwAttackSpeed
      ;

      if (rush.timer >= ::mwAttackTime) {
        self.state = MoldWing::StateRoam {
          .targetOrigin =
            origin + glm::vec2(rush.targetDirection, 0.0f) * 150.0f,
        };
      }
    },
  }, self.state);

  animation.instance.origin = origin;
}
