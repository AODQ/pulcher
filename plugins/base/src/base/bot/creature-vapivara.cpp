#include <plugin-base/bot/creature.hpp>

#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>
#include <pulcher-util/random.hpp>

#include <imgui/imgui.hpp>

namespace {
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**

  ********                **************
  * calm *        ___.--> * aggressive *
  ********       /        **************
     ||          ^              |
  .>Idle [start]<+<--------.  StandUp --> AttackClose
  |  |           ^         |    |   ^------'
  |  V [random]  |         '- StandDown
  | Roam ---------
	|
	'-------------------> RollAttack

**/

void plugin::bot::ImGuiCreatureVapivara(
  pul::core::SceneBundle & scene
, entt::entity selfEntity
) {
  using Vapivara = pul::core::ComponentCreatureVapivara;

  auto & registry = scene.EnttRegistry();

  auto & self = registry.get<pul::core::ComponentCreatureVapivara>(selfEntity);
  auto & origin = registry.get<pul::util::ComponentOrigin>(selfEntity).origin;

  ImGui::DragFloat2("origin", &origin.x, 1.0f);

  std::visit(overloaded {
    [&](Vapivara::StateCalm & calm) {
      ImGui::Text(" --- CALM");
      std::visit(overloaded {
        [&](Vapivara::StateCalm::StateIdle & idle) {
          ImGui::Text(" *** --- IDLE");
          ImGui::Text("    timer: %d", idle.timer);
        },
        [&](Vapivara::StateCalm::StateRoam & roam) {
          ImGui::Text(" *** --- WALK");
          ImGui::Text("    timer: %d", roam.timer);
          ImGui::Text("    mustTurn: %d", roam.mustTurn);
          ImGui::Text("    direction: %d", roam.direction);
        },
      }, calm.state);
    },
    [&](Vapivara::StateIntimidate & intimidate) {
      ImGui::Text(" --- INTIMIDATE");
      std::visit(overloaded {
        [&](Vapivara::StateIntimidate::StateStandUp &) {
          ImGui::Text(" *** --- STANDING UP");
        },
        [&](Vapivara::StateIntimidate::StateStandDown &) {
          ImGui::Text(" *** --- STANDING DOWN");
        },
        [&](Vapivara::StateIntimidate::StateAttackClose &) {
          ImGui::Text(" *** --- ATTACK CLOSE");
        },
      }, intimidate.state);
    },
    [&](Vapivara::StateRollAttack & rollAttack) {
      ImGui::Text(" --- ROLL ATTACK");
      ImGui::Text("     timer: %d",       rollAttack.timer);
      ImGui::Text("     halting: %d",     rollAttack.isHalting);
      ImGui::Text("     precharging: %d", rollAttack.isPrecharging);
    },
  }, self.state);
}

void plugin::bot::UpdateCreatureVapivara(
  pul::core::SceneBundle & scene
, entt::entity selfEntity
) {
  using Vapivara = pul::core::ComponentCreatureVapivara;

  auto & registry = scene.EnttRegistry();

  auto & self = registry.get<pul::core::ComponentCreatureVapivara>(selfEntity);

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

  // TODO maybe check for grounded?
  {
    auto const floorRay =
      pul::physics::IntersectorRay::Construct(
        glm::round(origin + glm::vec2(0.0f, 0.0f))
      , glm::round(origin + glm::vec2(0.0f, +10.0f))
    );
    pul::physics::IntersectionResults floorResults;
    plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

    if (!floorResults.collision) {
      origin.y += 10.0f;
    } else {
      origin.y = floorResults.origin.y;
    }
  }

  std::visit(overloaded {
    [&](Vapivara::StateCalm & calm) {
      std::visit(overloaded {
        [&](Vapivara::StateCalm::StateIdle & idle) {
          animationState.Apply("idle");

          if (idle.timer <= 0) {
            auto const newDirection = pul::util::RandomBool() ? -1 : +1;
            calm.state = Vapivara::StateCalm::StateRoam {
              .mustTurn = animationState.flip != (newDirection > 0),
              .timer = 16*10,
              .direction = newDirection,
            };
            return;
          }

          idle.timer -= 1;
        },
        [&](Vapivara::StateCalm::StateRoam & roam) {

          if (roam.mustTurn) {
            animationState.Apply("turn-crouched");
            if (animationState.animationFinished)
              roam.mustTurn = false;
            return;
          }

          animationState.Apply("walk");

          if (roam.timer <= 0) {
            calm.state = Vapivara::StateCalm::StateIdle {
              .timer = pul::util::RandomInt32(16*5, 16*25),
            };
            return;
          }

          animationState.flip = roam.direction > 0;

          origin.x += roam.direction * 0.2f;

          roam.timer -= 1;

          // check for players in view, to see if we should enter intimidate
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

            const bool isFacing =
              ((targetOrigin.x < origin.x) ^ animationState.flip)
            ;

            if (
                isFacing && dist.x < 255.0f && pul::util::RandomBoolBiased(15)
            ) {
              self.state = Vapivara::StateIntimidate {
                Vapivara::StateIntimidate::StateStandUp {
                  .timer = pul::util::RandomInt32(60, 240),
                },
              };
            }

            if (
                isFacing && dist.x < 855.0f && pul::util::RandomBoolBiased(5)
            ) {
              self.state = Vapivara::StateRollAttack { };
            }

            /* if (glm::length(dist) < 640.0f) { */
            /*   playerSpottedEntity = entity; */
            /*   break; */
            /* } */
          }
        },
      }, calm.state);
    },
    [&](Vapivara::StateIntimidate & intimidate) {
      std::visit(overloaded {
        [&](Vapivara::StateIntimidate::StateStandUp & stand) {
          animationState.Apply("stand-up");

          if (!animationState.animationFinished)
            return;

          if (stand.timer <= 0) {
            intimidate.state = Vapivara::StateIntimidate::StateStandDown { };
            return;
          }

          // see if player is close enough to attack
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

            const bool isFacing =
              ((targetOrigin.x < origin.x) ^ animationState.flip)
            ;

            if (isFacing && dist.x < 32.0f) {
              intimidate.state = Vapivara::StateIntimidate::StateAttackClose {};
              return;
            }
          }

          stand.timer -= 1;
        },
        [&](Vapivara::StateIntimidate::StateStandDown &) {
          animationState.Apply("stand-down");

          if (!animationState.animationFinished)
            return;

          self.state = Vapivara::StateCalm {
            Vapivara::StateCalm::StateIdle {
              .timer = pul::util::RandomInt32(16*5, 16*25),
            },
          };
        },
        [&](Vapivara::StateIntimidate::StateAttackClose &) {
          animationState.Apply("attack-close");

          if (!animationState.animationFinished)
            return;

          self.state = Vapivara::StateCalm {
            Vapivara::StateCalm::StateIdle {
              .timer = pul::util::RandomInt32(16*5, 16*25),
            },
          };
        },
      }, intimidate.state);
    },
    [&](Vapivara::StateRollAttack & rollAttack) {
      if (rollAttack.isPrecharging) {
        animationState.Apply("precharge");
        rollAttack.isPrecharging = !animationState.animationFinished;
        return;
      }

      const float direction = animationState.flip ? +1.0f : -1.0f;

      if (rollAttack.isHalting) {
        animationState.Apply("halt");
        rollAttack.isHalting = !animationState.animationFinished;

        origin.x += direction * (1.5f - animationState.componentIt/5.0f);

        if (!rollAttack.isHalting) {
          self.state = Vapivara::StateCalm { };
        }

        return;
      }

      // charging
      animationState.Apply("charge-loop");

      origin.x += direction * 3.75f;

      if (rollAttack.timer <= 0) {
        rollAttack.isHalting = true;
      }

      rollAttack.timer -= 1;
    },
  }, self.state);

  animation.instance.origin = origin;
}
