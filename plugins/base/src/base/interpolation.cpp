//--

#include <plugin-base/animation/animation.hpp>
#include <plugin-base/animation/render.hpp>
#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>

#include <glm/glm.hpp>

namespace {

struct BaseRenderBundle {
  InterpolantMap<plugin::animation::Interpolant> animationInterpolants;

  // only used as output TODO maybe make a different struct for outputs?
  std::vector<plugin::animation::Interpolant> animationInterpolantOutputs;

  static void Deallocate(void * data) {
    delete reinterpret_cast<BaseRenderBundle *>(data);
  }
};

}

extern "C" {

PUL_PLUGIN_DECL void Plugin_UpdateRenderBundleInstance(
  pul::core::SceneBundle & scene
, pul::core::RenderBundleInstance & instance
) {
  auto & registry = scene.EnttRegistry();

  InterpolantMap<plugin::animation::Interpolant> animationInterpolants;

  { // -- store animation information

    auto cameraOrigin = glm::vec2(scene.cameraOrigin);
    auto const
      cullBoundLeft  = cameraOrigin.x - scene.config.framebufferDimFloat.x/2.0f
    , cullBoundRight = cameraOrigin.x + scene.config.framebufferDimFloat.x/2.0f
    , cullBoundUp    = cameraOrigin.y - scene.config.framebufferDimFloat.y/2.0f
    , cullBoundDown  = cameraOrigin.y + scene.config.framebufferDimFloat.y/2.0f
    ;

    auto view = registry.view<pul::animation::ComponentInstance>();
    for (auto entity : view) {
      auto & self = view.get<pul::animation::ComponentInstance>(entity);

      if (self.instance.automaticCachedMatrixCalculation)
        { self.instance.hasCalculatedCachedInfo = false; }

      // check if visible / cull / ready to render
      if (
          !self.instance.visible
       || self.instance.drawCallCount == 0ul
       || cullBoundLeft  > self.instance.origin.x
       || cullBoundRight < self.instance.origin.x
       || cullBoundUp    > self.instance.origin.y
       || cullBoundDown  < self.instance.origin.y
      ) { continue; }

      // DEBUG
      /* debugRenderingInstances.emplace_back(&self.instance); */

      // copy the entire instance
      auto instance = self.instance;

      // compute cache
      plugin::animation::ComputeCache(
        instance, instance.animator->skeleton, glm::mat3(1.0f), false, 0.0f
      );
      animationInterpolants.emplace(
        static_cast<size_t>(entity), std::move(instance)
      );
    }
  }

  { // -- store data into the instance bundle
    auto & instanceBundleDataAny = instance.pluginBundleData["base"];

    if (!instanceBundleDataAny) {
      instanceBundleDataAny = std::make_shared<pul::util::Any>();
      instanceBundleDataAny->Deallocate = ::BaseRenderBundle::Deallocate;
      instanceBundleDataAny->userdata = new ::BaseRenderBundle;
    }

    auto & instanceBundleData =
      *reinterpret_cast<::BaseRenderBundle*>(instanceBundleDataAny->userdata);

    instanceBundleData.animationInterpolants = std::move(animationInterpolants);
  }
}

PUL_PLUGIN_DECL void Plugin_Interpolate(
  float const msDeltaInterp
, pul::core::RenderBundleInstance const & previousBundle
, pul::core::RenderBundleInstance const & currentBundle
, pul::core::RenderBundleInstance & outputBundle
) {
  // -- get previous/current bundle
  auto & bundleDataPreviousAny = previousBundle.pluginBundleData.at("base");
  auto & bundleDataCurrentAny  = currentBundle.pluginBundleData.at("base");

  // -- construct output any for this frame
  auto & bundleDataOutputAny = outputBundle.pluginBundleData["base"];
  bundleDataOutputAny = std::make_shared<pul::util::Any>();
  bundleDataOutputAny->Deallocate = ::BaseRenderBundle::Deallocate;
  bundleDataOutputAny->userdata = new ::BaseRenderBundle;

  // -- retrieve baserenderbundle for each
  auto
    & previous =
      *reinterpret_cast<::BaseRenderBundle*>(bundleDataPreviousAny->userdata)
  , & current =
      *reinterpret_cast<::BaseRenderBundle*>(bundleDataCurrentAny->userdata)
  , & output =
      *reinterpret_cast<::BaseRenderBundle*>(bundleDataOutputAny->userdata)
  ;

  // -- forward rendering information
  plugin::animation::Interpolate(
    msDeltaInterp
  , previous.animationInterpolants, current.animationInterpolants
  , output.animationInterpolantOutputs
  );
}

PUL_PLUGIN_DECL void Plugin_RenderInterpolated(
  pul::core::SceneBundle const & scene
, pul::core::RenderBundleInstance const & interpolatedBundle
) {
  // -- get current bundle
  auto & bundleDataCurrentAny = interpolatedBundle.pluginBundleData.at("base");

  // -- retrieve baserenderbundle for each
  auto
    & current =
      *reinterpret_cast<::BaseRenderBundle*>(bundleDataCurrentAny->userdata)
  ;

  // -- forward rendering information

  plugin::animation::RenderInterpolated(
    scene
  , interpolatedBundle
  , current.animationInterpolantOutputs
  );
}

} // -- extern C
