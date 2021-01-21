#include <plugin-base/animation/render.hpp>

#include <plugin-base/animation/animation.hpp>
#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>

static size_t animationBufferMaxSize = 4096*4096*5; // ~50MB

void plugin::animation::RenderInterpolated(
  pul::core::SceneBundle const & scene
, pul::core::RenderBundleInstance const & interpolatedBundle
, std::vector<plugin::animation::Interpolant> const & interpolants
) {
  // -- render animations
  auto & animationSystem = scene.AnimationSystem();

  // bind pipeline & global uniforms
  sg_apply_pipeline(animationSystem.sgPipeline);

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 1
  , &scene.config.framebufferDimFloat.x
  , sizeof(float) * 2ul
  );

  auto cameraOrigin = glm::vec2(interpolatedBundle.cameraOrigin);

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 2
  , &cameraOrigin.x
  , sizeof(float) * 2ul
  );

  static std::vector<glm::vec4> bufferData;
  /* static std::vector<size_t> imageData; */

  // set capacity and set size to 0
  if (bufferData.capacity() == 0ul)
    { bufferData.reserve(animationBufferMaxSize / sizeof(glm::vec4)); }
  bufferData.resize(0); // doesn't affect capacity

  // DEBUG
  /* debugRenderingInstances.resize(0); */

  auto const
    cullBoundLeft  = cameraOrigin.x - scene.config.framebufferDimFloat.x/2.0f
  , cullBoundRight = cameraOrigin.x + scene.config.framebufferDimFloat.x/2.0f
  , cullBoundUp    = cameraOrigin.y - scene.config.framebufferDimFloat.y/2.0f
  , cullBoundDown  = cameraOrigin.y + scene.config.framebufferDimFloat.y/2.0f
  ;

  // record each component to a buffer
  for (auto & interpolant : interpolants) {

    auto & instance = interpolant.instance;
    // check if visible / cull / ready to render
    if (
        !instance.visible
     || instance.drawCallCount == 0ul
     || cullBoundLeft > instance.origin.x
     || cullBoundRight < instance.origin.x
     || cullBoundUp > instance.origin.y
     || cullBoundDown < instance.origin.y
    ) { continue; }

    // DEBUG
    /* debugRenderingInstances.emplace_back(&instance); */

    for (size_t it = 0; it < instance.originBufferData.size(); ++ it) {
      auto const origin =
        instance.originBufferData[it]
      + glm::vec3(instance.origin, 0.0f)
      ;

      bufferData.emplace_back(glm::vec4(origin, 0.0f));
      bufferData.emplace_back(
        glm::vec4(instance.uvCoordBufferData[it], 0.0f, 0.0f)
      );
    }
  }

  sg_update_buffer(
    *animationSystem.sgBuffer
  , bufferData.data(), bufferData.size() * sizeof(glm::vec4)
  );

  sg_apply_bindings(animationSystem.sgBindings);

  /* glm::vec2 const resolution = */
  /*   glm::vec2( */
  /*     self.instance.animator->spritesheet.width */
  /*   , self.instance.animator->spritesheet.height */
  /*   ); */

  /* sg_apply_uniforms( */
  /*   SG_SHADERSTAGE_VS */
  /* , 3 */
  /* , &resolution.x */
  /* , sizeof(float) * 2ul */
  /* ); */

  sg_draw(0, bufferData.size() / 2, 1);

    /* sg_draw(0, self.instance.drawCallCount, 1); */
  /* } */
}

void plugin::animation::Interpolate(
  const float msDeltaInterp
, std::vector<plugin::animation::Interpolant> const & interpolantsPrev
, std::vector<plugin::animation::Interpolant> const & interpolantsCurr
, std::vector<plugin::animation::Interpolant> & interpolantsOut
) {
  if (interpolantsPrev.size() != interpolantsCurr.size())
    return;

  interpolantsOut.reserve(interpolantsPrev.size());

  for (size_t it = 0ul; it < interpolantsPrev.size(); ++ it) {

    auto & previous = interpolantsPrev[it].instance;
    auto & current  = interpolantsCurr[it].instance;

    // TODO
    /* if (instance.automaticCachedMatrixCalculation) */
    /*   { instance.hasCalculatedCachedInfo = false; } */

    // copy instance
    auto instance = previous;

    if (previous.originBufferData.size() != current.originBufferData.size())
      continue;

    // create an interpolated instance to compute vertices from
    instance.origin = glm::mix(previous.origin, current.origin, msDeltaInterp);

    for (auto & piecePair : instance.pieceToState) {
      auto const & label = std::get<0>(piecePair);
      auto & state = std::get<1>(piecePair);
      auto const & statePrev = previous.pieceToState.at(label);
      auto const & stateCurr = current.pieceToState.at(label);

      state.deltaTime =
        glm::mix(statePrev.deltaTime, stateCurr.deltaTime, msDeltaInterp);
      state.angle = glm::mix(statePrev.angle, stateCurr.angle, msDeltaInterp);
      state.flip = stateCurr.flip;
      state.visible = stateCurr.visible;
    }

    // compute vertices
    plugin::animation::ComputeCache(
      instance, instance.animator->skeleton, glm::mat3(1.0f), false, 0.0f
    );

    plugin::animation::ComputeVertices(instance, true);

    interpolantsOut.emplace_back(std::move(instance));
  }
}
