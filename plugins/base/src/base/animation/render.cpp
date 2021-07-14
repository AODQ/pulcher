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

  // set capacity and set size to 0
  if (bufferData.capacity() == 0ul)
    { bufferData.reserve(animationBufferMaxSize / sizeof(glm::vec4)); }
  bufferData.resize(0); // doesn't affect capacity

  // record each component to a buffer
  for (auto & interpolant : interpolants) {

    auto & instance = interpolant.instance;

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

    auto const offset =
      sg_append_buffer(
        *animationSystem.sgBuffer
      , bufferData.data(), bufferData.size() * sizeof(glm::vec4)
      );

    float textureResolution[2];
    textureResolution[0] = instance.animator->spritesheet.width;
    textureResolution[1] = instance.animator->spritesheet.height;
    sg_apply_uniforms(
      SG_SHADERSTAGE_FS
    , 0
    , &textureResolution[0]
    , sizeof(float) * 2ul
    );

    auto bindings = animationSystem.sgBindings;
    bindings.vertex_buffer_offsets[0] = offset;
    bindings.vertex_buffer_offsets[1] = offset;
    bindings.fs_images[0] = instance.animator->spritesheet.Image();
    sg_apply_bindings(bindings);

    sg_draw(0, bufferData.size() / 2, 1);

    bufferData.resize(0);
  }
}

void plugin::animation::Interpolate(
  const float msDeltaInterp
, InterpolantMap<plugin::animation::Interpolant> const & interpolantsPrev
, InterpolantMap<plugin::animation::Interpolant> const & interpolantsCurr
, std::vector<plugin::animation::Interpolant> & interpolantsOut
) {
  interpolantsOut.reserve(interpolantsPrev.size());

  for (auto & interpolantPair : interpolantsPrev) {

    auto const interpolantId = std::get<0>(interpolantPair);

    auto const & previous = std::get<1>(interpolantPair).instance;

    // locate current, if it doesn't exist then this object has been destroyed
    auto currentPtr = interpolantsCurr.find(interpolantId);
    if (currentPtr == interpolantsCurr.end()) { continue; }

    auto & current  = std::get<1>(*currentPtr).instance;

    // TODO
    /* if (instance.automaticCachedMatrixCalculation) */
    /*   { instance.hasCalculatedCachedInfo = false; } */

    // copy instance
    auto interpolant = plugin::animation::Interpolant { previous };
    auto & instance = interpolant.instance;

    // create an interpolated instance to compute vertices from
    instance.origin = glm::mix(previous.origin, current.origin, msDeltaInterp);

    for (auto & piecePair : instance.pieceToState) {
      auto const & label = std::get<0>(piecePair);
      auto & state = std::get<1>(piecePair);
      auto const & statePrev = previous.pieceToState.at(label);
      auto const & stateCurr = current.pieceToState.at(label);

      state.angle = glm::mix(statePrev.angle, stateCurr.angle, msDeltaInterp);
    }

    // compute vertices
    plugin::animation::ComputeCache(
      instance, instance.animator->skeleton, glm::mat3(1.0f), false, 0.0f
    );

    plugin::animation::ComputeVertices(instance, true);

    interpolantsOut.emplace_back(std::move(interpolant));
  }
}
