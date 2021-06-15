#include <plugin-base/bot/creature.hpp>

#include <plugin-base/physics/physics.hpp>

#include <pulcher-animation/animation.hpp>
#include <pulcher-core/creature.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/random.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-physics/intersections.hpp>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void plugin::bot::UpdateCreatureLump(
  pul::core::SceneBundle & scene
, pul::core::ComponentCreatureLump & self
, pul::animation::ComponentInstance & animation
) {

  if (self.checkForGrounded) {
    auto const floorRay =
      pul::physics::IntersectorRay::Construct(
        glm::round(self.origin + glm::vec2(0.0f, 0.0f))
      , glm::round(self.origin + glm::vec2(0.0f, +10.0f))
    );
    pul::physics::IntersectionResults floorResults;
    plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

    self.checkForGrounded = !floorResults.collision;

    if (!floorResults.collision) {
      self.origin.y += 10.0f;
    } else {
      self.origin.y = floorResults.origin.y;
    }
  }

  using Lump = pul::core::ComponentCreatureLump;

  auto & animationState = animation.instance.pieceToState["body"];
  auto & registry = scene.EnttRegistry();

  auto playerView =
    registry.view<
      pul::core::ComponentPlayer
    , pul::core::ComponentOrigin
    >();

  bool shouldDetectPlayer = true;

  if (self.stateFlip.active) {
    animationState.Apply("turn");
    shouldDetectPlayer = false;
    if (!animationState.animationFinished) { goto SKIP_AI_STATE_VISIT; }
    self.stateFlip.active = false;
    animationState.flip ^= 1;
  }

  std::visit(overloaded{
    [&](Lump::StateIdle & idle) {
      animationState.Apply("idle");
      if (idle.timer == 0) {
        if (pul::core::RandomBool()) {
          self.state = Lump::StateWalk {};
          return;
        } else {
          self.stateFlip.active = true;
          idle.timer = 1;
          return;
        }
      }
      idle.timer = (idle.timer + 1) % 100;
    }
  , [&](pul::core::ComponentCreatureLump::StateWalk & walk) {
      animationState.Apply("walk");
      float const direction = animationState.flip ? 1.0f : -1.0f;
      self.origin.x += direction * +0.25f;
      self.checkForGrounded = true;

      auto const wallRay =
        pul::physics::IntersectorRay::Construct(
          glm::round(self.origin + glm::vec2(direction*8, -38.0f))
        , glm::round(self.origin + glm::vec2(direction*8, -4.0f))
        );

      pul::physics::IntersectionResults wallResults;
      plugin::physics::IntersectionRaycast(scene, wallRay, wallResults);

      if (wallResults.collision) {
        self.stateFlip.active = true;
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
      auto & origin =
        registry.get<pul::core::ComponentOrigin>(rush.targetEntity).origin
      ;
      shouldDetectPlayer = false;

      // check if have to flip around first
      if (animationState.flip ^ (origin.x > self.origin.x)) {
        self.stateFlip.active = true;
        return;
      }

      float const direction = animationState.flip ? 1.0f : -1.0f;
      self.origin.x += direction * 1.0f;
      self.checkForGrounded = true;

      if (glm::abs(self.origin.x - origin.x) < 18.0f) {
        self.state = Lump::StateDodge {};
      }
    }
  , [&](pul::core::ComponentCreatureLump::StateDodge & dodge) {
      shouldDetectPlayer = false;
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
          glm::round(self.origin + glm::vec2(0.0f, 0.0f))
        , glm::round(self.origin + dodge.airVelocity)
      );
      pul::physics::IntersectionResults floorResults;
      plugin::physics::IntersectionRaycast(scene, floorRay, floorResults);

      if (dodge.airVelocity.y > 0.0f && floorResults.collision) {
        self.origin = floorResults.origin;
        self.state = Lump::StateIdle {};
      } else {
        self.origin += dodge.airVelocity;
      }
  }
  }, self.state
  );

  SKIP_AI_STATE_VISIT:

  // if the AI is flipping then don't allow it to detect/rush the player, but
  // it also happens before the AI gets to change any internal state

  if (shouldDetectPlayer) {
    for (auto entity : playerView) {
      /* auto & player = view.get<pul::core::ComponentPlayer>(entity); */
      auto const & origin =
        playerView.get<pul::core::ComponentOrigin>(entity).origin
      ;
      // TODO align this better to entity centers

      glm::vec2 const dist = glm::abs(origin - self.origin);

      if (dist.x < 32.0f*16.0f && dist.y < 32.0f*6.0f) {
        // if we have to turn around first
        self.stateFlip.active =
          animationState.flip ^ (origin.x > self.origin.x)
        ;

        self.state = Lump::StateRush { .targetEntity = entity, };
      }
    }
  }

  animation.instance.origin = self.origin;
}
