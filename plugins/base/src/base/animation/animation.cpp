#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <entt/entt.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/transform2.hpp>
#include <imgui/imgui.hpp>
#include <sokol/gfx.hpp>

#include <fstream>

// animation could always use cleaning / optimizing as a lot of it isn't based
// on anything concrete, it's all developed pretty rapidly using whatever just
// worked at the time

// this implements the logic for the 'pixel-perfect' animation skeletal system

namespace {

static size_t animationBufferMaxSize = 4096*4096*5; // ~50MB

static std::vector<pul::animation::Instance const *> debugRenderingInstances;

static size_t animMsTimer = 0ul;
static bool animLoop = true;
static float animShowPreviousSprite = 0.0f;
static bool animShowZoom = false;
static bool animPlaying = true;
static bool animEmptyOnLoopEnd = false;
static size_t animMaxTime = 100'000ul;

void JsonParseRecursiveSkeleton(
  cJSON * skeletalParentJson
, std::vector<pul::animation::Animator::SkeletalPiece> & skeletals
) {
  if (!skeletalParentJson) { return; }
  cJSON * skeletalChildJson;
  cJSON_ArrayForEach(
    skeletalChildJson
  , cJSON_GetObjectItemCaseSensitive(skeletalParentJson, "skeleton")
  ) {
    pul::animation::Animator::SkeletalPiece skeletal;
    skeletal.label =
      cJSON_GetObjectItemCaseSensitive(skeletalChildJson, "label")->valuestring;
    skeletal.origin.x =
      cJSON_GetObjectItemCaseSensitive(skeletalChildJson, "origin-x")->valueint;
    skeletal.origin.y =
      cJSON_GetObjectItemCaseSensitive(skeletalChildJson, "origin-y")->valueint;
    JsonParseRecursiveSkeleton(skeletalChildJson, skeletal.children);
    skeletals.emplace_back(std::move(skeletal));
  }
}

std::vector<pul::animation::Component> JsonLoadComponents(
  cJSON * componentsJson
) {
  std::vector<pul::animation::Component> components;

  cJSON * componentJson;
  cJSON_ArrayForEach(componentJson, componentsJson) {
    pul::animation::Component component;
    component.tile.x =
      cJSON_GetObjectItemCaseSensitive(componentJson, "x")->valueint;

    component.tile.y =
      cJSON_GetObjectItemCaseSensitive(componentJson, "y")->valueint;

    component.msDeltaTimeOverride =
      cJSON_GetObjectItemCaseSensitive(componentJson, "ms-delta-time-override")
        ->valueint;

    if (
      auto offX =
        cJSON_GetObjectItemCaseSensitive(componentJson, "origin-offset-x")
    ) {
      component.originOffset.x = offX->valueint;
    }

    if (
      auto offY =
        cJSON_GetObjectItemCaseSensitive(componentJson, "origin-offset-y")
    ) {
      component.originOffset.y = offY->valueint;
    }

    components.emplace_back(component);
  }

  return components;
}

cJSON * JsonWriteRecursiveSkeleton(
  std::vector<pul::animation::Animator::SkeletalPiece> & skeletals
) {
  if (skeletals.size() == 0ul) { return nullptr; }

  cJSON * skeletalsJson = cJSON_CreateArray();
  for (auto & skeletal : skeletals) {
    cJSON * skeletalJson = cJSON_CreateObject();
    cJSON_AddItemToArray(skeletalsJson, skeletalJson);

    cJSON_AddItemToObject(
      skeletalJson, "label", cJSON_CreateString(skeletal.label.c_str())
    );

    cJSON_AddItemToObject(
      skeletalJson, "origin-x", cJSON_CreateInt(skeletal.origin.x)
    );
    cJSON_AddItemToObject(
      skeletalJson, "origin-y", cJSON_CreateInt(skeletal.origin.y)
    );

    if (auto childrenJson = JsonWriteRecursiveSkeleton(skeletal.children)) {
      cJSON_AddItemToObject(skeletalJson, "skeleton", childrenJson);
    }
  }

  return skeletalsJson;
}

size_t ComputeVertexBufferSize(
  std::vector<pul::animation::Animator::SkeletalPiece> const & skeletals
) {
  size_t vertices = 0ul;
  for (const auto & skeletal : skeletals) {
    vertices += 6 + ComputeVertexBufferSize(skeletal.children);
  }
  return vertices;
}

// used to compute generic animation info necessary for computing vertices and
// caching animation state. Returns a pointer to list of components, nullptr if
// none were found
std::tuple<
  pul::animation::Animator::Piece &
, pul::animation::Instance::StateInfo &
, pul::animation::Animator::State &
, std::vector<pul::animation::Component> *
> ComputeAnimationInfo(
  pul::animation::Instance & instance
, pul::animation::Animator::SkeletalPiece const & skeletal
, bool & skeletalFlip
, float & skeletalRotation
) {
  // okay the below is crazy with the map lookups, I need to cache the
  // state somehow
  auto & piece     = instance.animator->pieces[skeletal.label];
  auto & stateInfo = instance.pieceToState[skeletal.label];
  auto & state     = piece.states[stateInfo.label];

  // update skeletal information (origins, flip, rotation, etc)
  skeletalFlip ^= stateInfo.flip;

  skeletalRotation += stateInfo.angle;

  if (state.rotationMirrored && ((skeletalRotation > 0.0f) ^ skeletalFlip)) {
    skeletalFlip ^= 1;
  }

  // modify component runtime info based off skeletal info
  pul::animation::VariationRuntimeInfo variationRti = stateInfo.variationRti;
  switch (state.variationType) {
    case pul::animation::VariationType::Range:
      variationRti.range.flip = skeletalFlip;
      variationRti.range.angle = skeletalRotation;
    break;
    default: break;
  }

  // grab component from flip/rotation
  auto * componentsPtr = state.ComponentLookup(variationRti);
  return { piece, stateInfo, state, componentsPtr };
}

void ComputeVertices(
  pul::core::SceneBundle &
, pul::animation::Instance & instance
, pul::animation::Animator::SkeletalPiece const & skeletal
, size_t & indexOffset
, bool & skeletalFlip
, float & skeletalRotation
, bool & forceUpdate
) {
  auto const & [piece, stateInfo, state, componentsPtr] =
    ComputeAnimationInfo(instance, skeletal, skeletalFlip, skeletalRotation);

  // if there are no components to render, output a degenerate tile
  if (!componentsPtr || componentsPtr->size() == 0ul) {
    for (size_t it = 0ul; it < 6ul; ++ it) {
      instance.uvCoordBufferData[indexOffset + it] = glm::vec2(-1);
      instance.originBufferData[indexOffset + it] = glm::vec3(-1);
    }

    indexOffset += 6ul;
    return;
  }

  auto & components = *componentsPtr;

  PUL_ASSERT_CMP(components.size(), >, 0ul, return;);

  PUL_ASSERT_CMP(
    stateInfo.componentIt, <, components.size()
  , stateInfo.componentIt = components.size()-1;
  );

  auto & component = components[stateInfo.componentIt];

  // update delta time and if animation update is necessary, apply uv coord
  // updates
  bool hasUpdate = forceUpdate;
  float const msDeltaTime = state.MsDeltaTime(component);

  if (msDeltaTime > 0.0f && !stateInfo.animationFinished) {
    stateInfo.deltaTime += pul::util::MsPerFrame;
    if (stateInfo.deltaTime > msDeltaTime) {
      if (state.loops) {
        stateInfo.deltaTime = stateInfo.deltaTime - msDeltaTime;
        stateInfo.componentIt = (stateInfo.componentIt + 1) % components.size();
        hasUpdate = true;
      } else {
        if (stateInfo.componentIt < components.size()-1) {
          stateInfo.deltaTime = stateInfo.deltaTime - msDeltaTime;
          stateInfo.componentIt = stateInfo.componentIt + 1;
          hasUpdate = true;
        } else {
          stateInfo.deltaTime = 0.0f;
          stateInfo.animationFinished = true;
        }
      }
    }
  }

  if (!hasUpdate) {
    indexOffset += 6ul;
    return;
  }

  auto pieceDimensions = glm::vec2(piece.dimensions);

  // update origins & UV coords
  for (size_t it = 0ul; it < 6; ++ it, ++ indexOffset) {
    auto v = pul::util::TriangleVertexArray()[it];
    auto uv = v;

    auto uvCoordWrap = stateInfo.uvCoordWrap;
    /* if (state.flipXAxis) { uvCoordWrap = 1.0f - uvCoordWrap; } */
    /* if (skeletalFlip)    { uvCoordWrap = 1.0f - uvCoordWrap; } */

    // apply uv clipping/wrapping if requested (non 1.0 value)
    uv *= uvCoordWrap;
    v  *= stateInfo.vertWrap;

    if (stateInfo.flipVertWrap) {
      v = glm::vec2(1.0f) - v;
    }

    // flip uv coords if requested
    if (skeletalFlip) { uv.x = 1.0f - uv.x; }

    if (state.flipXAxis) { uv.x = 1.0f - uv.x; }

    instance.uvCoordBufferData[indexOffset] =
      (
          (uv*pieceDimensions + glm::vec2(component.tile)*pieceDimensions)
        + glm::vec2(instance.animator->uvCoordOffset)
      )
      * instance.animator->spritesheet.InvResolution()
    ;
    auto origin = glm::vec3(v*pieceDimensions, 1.0f);

    origin = stateInfo.cachedLocalSkeletalMatrix * origin;

    // produce degenerate triangle if not visible
    if (!stateInfo.visible)
      { origin = glm::vec3(-99999.0f); }

    instance.originBufferData[indexOffset] =
        glm::vec3(origin.x, origin.y, static_cast<float>(piece.renderDepth));
  }
}

void ComputeVertices(
  pul::core::SceneBundle & scene
, pul::animation::Instance & instance
, std::vector<pul::animation::Animator::SkeletalPiece> const & skeletals
, size_t & indexOffset
, bool const skeletalFlip
, float const skeletalRotation
, bool const forceUpdate
) {
  for (auto const & skeletal : skeletals) {
    // compute origin / uv coords
    bool newSkeletalFlip = skeletalFlip;
    float newSkeletalRotation = skeletalRotation;
    bool newForceUpdate = forceUpdate;
    ComputeVertices(
      scene, instance, skeletal, indexOffset
    , newSkeletalFlip, newSkeletalRotation, newForceUpdate
    );

    // continue to children
    ComputeVertices(
      scene, instance, skeletal.children, indexOffset
    , newSkeletalFlip, newSkeletalRotation, newForceUpdate
    );
  }
}

void ComputeVertices(
  pul::core::SceneBundle & scene
, pul::animation::Instance & instance
, bool forceUpdate = false
) {
  size_t indexOffset = 0ul;
  ComputeVertices(
    scene, instance, instance.animator->skeleton
  , indexOffset, false, 0.0f, forceUpdate
  );
}

void ComputeCache(
  pul::animation::Instance & instance
, pul::animation::Animator::SkeletalPiece const & skeletal
, glm::mat3 & skeletalMatrix
, bool & skeletalFlip
, float & skeletalRotation
) {
  if (instance.hasCalculatedCachedInfo) { return; }


  auto const & [piece, stateInfo, state, componentsPtr] =
    ComputeAnimationInfo(instance, skeletal, skeletalFlip, skeletalRotation);

  if (!componentsPtr || componentsPtr->size() == 0ul) { return; }

  auto & components = *componentsPtr;

  PUL_ASSERT_CMP(
    stateInfo.componentIt, <, components.size()
  , stateInfo.componentIt = components.size()-1;
  );

  auto & component = components[stateInfo.componentIt];

  // -- compose skeletal matrix
  glm::mat3 localMatrix = glm::mat3(1);
  glm::vec2 localOrigin = piece.origin;

  // interpolate origin if requested
  if (state.originInterpolates) {
    glm::vec2 previousOrigin = glm::vec2(0.0f);

    // if it loops then wrap around, otherwise can only grab origin offset if
    // not the first component
    if (state.loops || stateInfo.componentIt != 0ul) {
      previousOrigin =
        components[
          (stateInfo.componentIt+components.size()-1)%components.size()
        ].originOffset
      ;
    }

    localOrigin +=
      glm::mix(
        glm::vec2(previousOrigin),
        glm::vec2(component.originOffset),
        stateInfo.deltaTime/state.MsDeltaTime(component)
      );
  } else {
    localOrigin += component.originOffset;
  }

  // first rotate locally by offset into the pixel it rotates around
  if (skeletalFlip) {
    localOrigin.x = piece.dimensions.x - localOrigin.x;
  }

  localMatrix =
    glm::translate(
      localMatrix, glm::vec2(localOrigin)
    );

  // flip origin

  float const theta = -skeletalRotation - pul::Pi*0.5f + skeletalFlip*pul::Pi;
  if (state.rotatePixels)
    { localMatrix = glm::rotate(localMatrix, theta); }

  localMatrix =
    glm::translate(
      localMatrix, glm::vec2(-localOrigin)
    );

  // then apply its offset into the skeletal structure
  auto skeletalOrigin = skeletal.origin;
  if (skeletalFlip) { skeletalOrigin.x *= -1; }
  skeletalMatrix =
    glm::translate(
      skeletalMatrix
    , glm::vec2(skeletalOrigin) - localOrigin
    );

  skeletalMatrix = skeletalMatrix * localMatrix;

  // cache matrix
  stateInfo.cachedLocalSkeletalMatrix = skeletalMatrix;

  // the piece origin is only part of the local matrix, so it must be
  // recomposed, but apply the flipping
  {
    localOrigin = piece.origin;
    if (skeletalFlip)
      { localOrigin.x = piece.dimensions.x - localOrigin.x; }
    skeletalMatrix =
      glm::translate(skeletalMatrix, glm::vec2(localOrigin));
   }
}

void ComputeCache(
  pul::animation::Instance & instance
, std::vector<pul::animation::Animator::SkeletalPiece> const & skeletals
, glm::mat3 const skeletalMatrix
, bool const skeletalFlip
, float const skeletalRotation
) {
  for (auto const & skeletal : skeletals) {
    auto newSkeletalMatrix = skeletalMatrix;
    auto newSkeletalFlip = skeletalFlip;
    auto newSkeletalRotation = skeletalRotation;
    ComputeCache(
      instance, skeletal
    , newSkeletalMatrix, newSkeletalFlip, newSkeletalRotation
    );

    ComputeCache(
      instance, skeletal.children
    , newSkeletalMatrix, newSkeletalFlip, newSkeletalRotation
    );
  }
}

} // -- namespace

extern "C" {
PUL_PLUGIN_DECL void Animation_ConstructInstance(
  pul::core::SceneBundle & scene
, pul::animation::Instance & animationInstance
, pul::animation::System & animationSystem
, char const * label
) {
  if (
    auto instance = animationSystem.animators.find(label);
    instance != animationSystem.animators.end()
  ) {
    animationInstance.animator = instance->second;
  } else {
    spdlog::error("Could not find animation of type '{}'", label);
    return;
  }

  // set default values for pieces
  for (auto & piecePair : animationInstance.animator->pieces) {
    if (piecePair.second.states.begin() == piecePair.second.states.end()) {
      spdlog::error(
        "need at least one state for piece '{}' of '{}'"
      , piecePair.first, animationInstance.animator->label
      );
      continue;
    }
    auto & pieceToState = animationInstance.pieceToState[piecePair.first];
    pieceToState.label = piecePair.second.states.begin()->first;
    pieceToState.animator = animationInstance.animator;
    pieceToState.pieceLabel = piecePair.first;
  }

  { // -- compute initial sokol buffers

    // precompute size
    size_t const vertexBufferSize =
      ::ComputeVertexBufferSize(animationInstance.animator->skeleton);

    animationInstance.uvCoordBufferData.resize(vertexBufferSize);
    animationInstance.originBufferData.resize(vertexBufferSize);

    ::ComputeVertices(scene, animationInstance, true);

    /* { // -- origin */
    /*   sg_buffer_desc desc = {}; */
    /*   desc.size = vertexBufferSize * sizeof(glm::vec3); */
    /*   desc.usage = SG_USAGE_STREAM; */
    /*   desc.content = nullptr; */
    /*   desc.label = "origin buffer"; */
    /*   animationInstance.sgBufferOrigin = std::make_unique<pul::gfx::SgBuffer>(); */
    /*   animationInstance.sgBufferOrigin->buffer = sg_make_buffer(&desc); */
    /* } */

    /* { // -- uv coord */
    /*   sg_buffer_desc desc = {}; */
    /*   desc.size = vertexBufferSize * sizeof(glm::vec2); */
    /*   desc.usage = SG_USAGE_STREAM; */
    /*   desc.content = nullptr; */
    /*   desc.label = "uv coord buffer"; */
    /*   animationInstance.sgBufferUvCoord = std::make_unique<pul::gfx::SgBuffer>(); */
    /*   animationInstance.sgBufferUvCoord->buffer = sg_make_buffer(&desc); */
    /* } */

    // bindings
    /* animationInstance.sgBindings.vertex_buffers[0] = */
    /*   *animationSystem.sgBuffer; */
    /* animationInstance.sgBindings.vertex_buffers[1] = */
    /*   *animationInstance.sgBufferUvCoord; */
    /* animationInstance.sgBindings.fs_images[0] = */
    /*   animationInstance.animator->spritesheet.Image(); */

    // get draw call count
    animationInstance.drawCallCount = vertexBufferSize;
  }
}

} // -- extern C

namespace {

void ReconstructInstances(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();
  auto & system = scene.AnimationSystem();
  auto view = registry.view<pul::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pul::animation::ComponentInstance>(entity);
    auto label = self.instance.animator->label;
    self.instance = {};
    Animation_ConstructInstance(scene, self.instance, system, label.c_str());
  }
}

void ImGuiRenderSpritesheetTile(
  pul::animation::Animator & animator
, pul::animation::Animator::Piece & piece
, pul::animation::Component & component
, float alpha = 1.0f
) {
  auto pieceDimensions = glm::vec2(piece.dimensions);
  auto const imgUl =
    (
      glm::vec2(animator.uvCoordOffset)
    + glm::vec2(component.tile)*pieceDimensions
    )
  * animator.spritesheet.InvResolution()
  ;

  auto const imgLr =
    imgUl + pieceDimensions * animator.spritesheet.InvResolution()
  ;

  ImVec2 dimensions = ImVec2(piece.dimensions.x, piece.dimensions.y);
  if (dimensions.x > 100)
    dimensions.x = 100;

  if (animShowZoom) {
    dimensions.x *= 5.0f;
    dimensions.y *= 5.0f;
  }

  ImGui::Image(
    reinterpret_cast<void *>(animator.spritesheet.Image().id)
  , dimensions
  , ImVec2(imgUl.x, 1.0f-imgUl.y)
  , ImVec2(imgLr.x, 1.0f-imgLr.y)
  , (
      animMsTimer == animMaxTime && animEmptyOnLoopEnd
    ? ImVec4(0, 0, 0, 0) : ImVec4(1, 1, 1, alpha)
    )
  , ImVec4(1, 1, 1, 1)
  );

  // must be at least 64 pixels  for alignment, so if it is not we need to
  // introduce empty space
  if (dimensions.y < 64) {
    ImGui::Image(
      0, ImVec2(dimensions.x, 64 - dimensions.y),
      ImVec2(0, 0), ImVec2(0, 0), ImVec4(0, 0, 0, 0)
    );
  }
}

void DisplayImGuiComponent(
  pul::animation::Animator & animator
, pul::animation::Animator::Piece & piece
, std::vector<pul::animation::Component> & components
, size_t componentIt
) {
  auto & component = components[componentIt];

  ImGui::PushID(componentIt);

  // do not show columns or editor properties on sprite zoom
  if (!::animShowZoom) {
    ImGui::Columns(2, nullptr, false);

    ImGui::SetColumnWidth(0, glm::min(100u, piece.dimensions.x + 16u));
  }

  // display current image on left side of column, use transparency
  // w/ previous frame if requested
  ImVec2 pos = ImGui::GetCursorScreenPos();
  if (animShowPreviousSprite > 0.0f) {
    ::ImGuiRenderSpritesheetTile(
      animator, piece
    , components[(components.size() + componentIt - 1) % components.size()]
    , animShowPreviousSprite
    );
  }
  ImGui::SetCursorScreenPos(pos);
  ::ImGuiRenderSpritesheetTile(animator, piece, component);

  if (!::animShowZoom) {
    ImGui::NextColumn();
    pul::imgui::DragInt2("tile", &component.tile.x, 0.025f);

    pul::imgui::DragInt2("origin-offset", &component.originOffset.x, 0.025f);
    pul::imgui::DragInt("ms-time", &component.msDeltaTimeOverride, 0.25f);

    if (ImGui::Button("-")) {
      components.erase(components.begin() + componentIt);
      -- componentIt;
    }

    ImGui::SameLine();
    if (ImGui::Button("+")) {
      components
        .emplace(components.begin() + componentIt + 1, component);
    }

    ImGui::SameLine();
    if (ImGui::Button("+x")) {
      components
        .emplace(components.begin() + componentIt + 1, component);
      components[componentIt+1].tile.x += 1;
    }

    ImGui::SameLine();
    if (ImGui::Button("+y")) {
      components
        .emplace(components.begin() + componentIt + 1, component);
      components[componentIt+1].tile.y += 1;
    }

    if (componentIt > 0ul) {
      ImGui::SameLine();
      if(ImGui::Button("^"))
        { std::swap(components[componentIt], components[componentIt-1]); }
    }

    if (componentIt < components.size()-1) {
      ImGui::SameLine();
      if (ImGui::Button("V"))
        { std::swap(components[componentIt], components[componentIt+1]); }
    }

    ImGui::NextColumn();
  }

  ImGui::PopID(); // componentIt
  ImGui::Separator();

  ImGui::Columns(1);
}

void DisplayImGuiComponents(
  pul::animation::Animator & animator
, pul::animation::Animator::State & state
, pul::animation::Animator::Piece & piece
, std::vector<pul::animation::Component> & components
) {
  if (components.size() > 0ul)
  { // display image
    float tempMsDeltaTime = animMsTimer;
    { // wrap tempMsDeltaTime first
      float totalComponentTime = 0.0f;
      for (auto & component : components)
        { totalComponentTime += state.MsDeltaTime(component); }
      tempMsDeltaTime = glm::mod(tempMsDeltaTime, totalComponentTime);
    }

    // then subtract until we reach the component
    size_t componentIt = 0ul;
    for (componentIt = 0ul; componentIt < components.size(); ++ componentIt) {
      auto & component = components[componentIt];
      if (tempMsDeltaTime - state.MsDeltaTime(component) < 0.0f) { break; }
      tempMsDeltaTime -= state.MsDeltaTime(component);
    }

    // render w/ previous sprite alpha if requested

    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (animShowPreviousSprite > 0.0f) {
      ::ImGuiRenderSpritesheetTile(
        animator, piece
      , components[(components.size() + componentIt - 1) % components.size()]
      );
    }
    ImGui::SetCursorScreenPos(pos);
    ::ImGuiRenderSpritesheetTile(animator, piece, components[componentIt]);

    ImGui::Separator();
  }

  for (size_t it = 0ul; it < components.size(); ++ it) {
    DisplayImGuiComponent(animator, piece, components, it);
  }
}

void DisplayImGuiSkeleton(
  pul::core::SceneBundle & scene
, pul::animation::Animator & animator
, pul::animation::System & system
, std::vector<pul::animation::Animator::SkeletalPiece> & skeletals
) {

  ImGui::SameLine();
  if (ImGui::Button("add"))
    { ImGui::OpenPopup("add skeletal"); }

  if (ImGui::BeginPopup("add skeletal")) {
    for (auto & piecePair : animator.pieces) {
      auto & label = std::get<0>(piecePair);
      if (ImGui::Selectable(label.c_str())) {
        skeletals.emplace_back(
          pul::animation::Animator::SkeletalPiece{label}
        );
        ReconstructInstances(scene);
      }
    }

    ImGui::EndPopup();
  }

  size_t skeletalIdx = 0ul;
  for (auto & skeletal : skeletals) {
    ImGui::PushID(skeletalIdx);
    if (
      !ImGui::TreeNode(fmt::format("skeletal '{}'", skeletal.label).c_str())
    ) {
      ++ skeletalIdx;
      ImGui::PopID();
      continue;
    }

    pul::imgui::DragInt2("origin", &skeletal.origin.x, 0.025f);

    if (ImGui::Button("remove")) {
      skeletals.erase(skeletals.begin() + skeletalIdx);
      ReconstructInstances(scene);

      ImGui::TreePop();
      ImGui::PopID();
      continue;
    }

    DisplayImGuiSkeleton(scene, animator, system, skeletal.children);

    ImGui::TreePop();
    ImGui::PopID();

    ++ skeletalIdx;
  }
}

cJSON * SaveAnimationComponent(
  std::vector<pul::animation::Component> const & components
) {
  cJSON * componentsJson = cJSON_CreateArray();

  for (auto const & component : components) {
    cJSON * componentJson = cJSON_CreateObject();
    cJSON_AddItemToArray(componentsJson, componentJson);

    cJSON_AddItemToObject(
      componentJson, "x", cJSON_CreateInt(component.tile.x)
    );
    cJSON_AddItemToObject(
      componentJson, "y", cJSON_CreateInt(component.tile.y)
    );
    cJSON_AddItemToObject(
      componentJson, "origin-offset-x"
    , cJSON_CreateInt(component.originOffset.x)
    );
    cJSON_AddItemToObject(
      componentJson, "origin-offset-y"
    , cJSON_CreateInt(component.originOffset.y)
    );
    cJSON_AddItemToObject(
      componentJson, "ms-delta-time-override"
    , cJSON_CreateInt(component.msDeltaTimeOverride)
    );
  }

  return componentsJson;
}

cJSON * SaveAnimation(pul::animation::Animator& animator) {
  cJSON * spritesheetJson = cJSON_CreateObject();

  cJSON_AddItemToObject(
    spritesheetJson, "label", cJSON_CreateString(animator.label.c_str())
  );

  cJSON_AddItemToObject(
    spritesheetJson, "uv-offset-x", cJSON_CreateInt(animator.uvCoordOffset.x)
  );

  cJSON_AddItemToObject(
    spritesheetJson, "uv-offset-y", cJSON_CreateInt(animator.uvCoordOffset.y)
  );

  cJSON_AddItemToObject(
    spritesheetJson
  , "filename", cJSON_CreateString(animator.spritesheet.filename.c_str())
  );

  // TODO bug if skeleton is empty i think
  cJSON_AddItemToObject(
    spritesheetJson
  , "skeleton"
  , JsonWriteRecursiveSkeleton(animator.skeleton)
  );

  cJSON * animationPieceJson = cJSON_CreateArray();
  cJSON_AddItemToObject(
    spritesheetJson, "animation-piece", animationPieceJson
  );

  for (auto const & piecePair : animator.pieces) {
    auto const & pieceLabel = std::get<0>(piecePair);
    auto const & piece = std::get<1>(piecePair);

    cJSON * pieceJson = cJSON_CreateObject();
    cJSON_AddItemToArray(animationPieceJson, pieceJson);

    cJSON_AddItemToObject(
      pieceJson, "label", cJSON_CreateString(pieceLabel.c_str())
    );
    cJSON_AddItemToObject(
      pieceJson, "dimension-x", cJSON_CreateInt(piece.dimensions.x)
    );
    cJSON_AddItemToObject(
      pieceJson, "dimension-y", cJSON_CreateInt(piece.dimensions.y)
    );
    cJSON_AddItemToObject(
      pieceJson, "origin-x", cJSON_CreateInt(piece.origin.x)
    );
    cJSON_AddItemToObject(
      pieceJson, "origin-y", cJSON_CreateInt(piece.origin.y)
    );
    cJSON_AddItemToObject(
      pieceJson, "render-order", cJSON_CreateInt(piece.renderDepth)
    );

    cJSON * statesJson = cJSON_CreateArray();
    cJSON_AddItemToObject(pieceJson, "states", statesJson);

    for (auto const & statePair : piece.states) {
      auto const & stateLabel = std::get<0>(statePair);
      auto const & state = std::get<1>(statePair);

      cJSON * stateJson = cJSON_CreateObject();
      cJSON_AddItemToArray(statesJson, stateJson);

      cJSON_AddItemToObject(
        stateJson, "label", cJSON_CreateString(stateLabel.c_str())
      );
      cJSON_AddItemToObject(
        stateJson, "ms-delta-time", cJSON_CreateInt(state.msDeltaTime)
      );
      cJSON_AddItemToObject(
        stateJson, "rotation-mirrored"
      , cJSON_CreateInt(state.rotationMirrored)
      );
      cJSON_AddItemToObject(
        stateJson, "origin-interpolates"
      , cJSON_CreateInt(state.originInterpolates)
      );
      cJSON_AddItemToObject(
        stateJson, "rotate-pixels"
      , cJSON_CreateInt(state.rotatePixels)
      );
      cJSON_AddItemToObject(
        stateJson, "flip-x-axis"
      , cJSON_CreateInt(state.flipXAxis)
      );
      cJSON_AddItemToObject(
        stateJson, "loops"
      , cJSON_CreateInt(state.loops)
      );

      cJSON * variationsJson = cJSON_CreateArray();
      cJSON_AddItemToObject(stateJson, "variations", variationsJson);
      cJSON_AddItemToObject(
        stateJson, "variationType"
      , cJSON_CreateString(ToStr(state.variationType))
      );

      for (auto const & variation : state.variations) {
        cJSON * variationJson = cJSON_CreateObject();
        cJSON_AddItemToArray(variationsJson, variationJson);

        switch (state.variationType) {
          default:
            spdlog::critical("unknown variation type {}", state.variationType);
          break;
          case pul::animation::VariationType::Normal:
            cJSON_AddItemToObject(
              variationJson, "default"
            , SaveAnimationComponent(variation.normal.data)
            );
          break;
          case pul::animation::VariationType::Random:
            cJSON_AddItemToObject(
              variationJson, "default"
            , SaveAnimationComponent(variation.random.data)
            );
          break;
          case pul::animation::VariationType::Range:
            cJSON_AddItemToObject(
              variationJson, "angle-range-max"
            , cJSON_CreateNumber(
                static_cast<double>(variation.range.rangeMax)
              )
            );

            cJSON_AddItemToObject(
              variationJson, "default"
            , SaveAnimationComponent(variation.range.data[0])
            );

            cJSON_AddItemToObject(
              variationJson, "flipped"
            , SaveAnimationComponent(variation.range.data[1])
            );
          break;
        }
      }
    }
  }

  return spritesheetJson;
}

void SaveAnimations(pul::animation::System & system) {
  // save each animation into its own map to seperate them out by file
  std::map<std::string, std::vector<cJSON *>> filenameToCJson;
  for (auto & animatorPair : system.animators) {
    auto & animator = *animatorPair.second;
    filenameToCJson[animator.filename].emplace_back(SaveAnimation(animator));
  }

  // accumulate each cJSON into an array of spritesheets & save the file
  for (auto & filenameCJsonPair : filenameToCJson) {
    auto & filename = filenameCJsonPair.first;

    cJSON * fileDataJson = cJSON_CreateObject();
    cJSON * spritesheetsJson = cJSON_CreateArray();
    cJSON_AddItemToObject(fileDataJson, "spritesheets", spritesheetsJson);

    for (auto & spritesheetJson : filenameCJsonPair.second)
      { cJSON_AddItemToArray(spritesheetsJson, spritesheetJson); }

    { // -- save file
      auto jsonStr = cJSON_Print(fileDataJson);

      spdlog::info("saving json: '{}'", filename);

      auto file = std::ofstream{filename};
      if (file.good()) {
        file << jsonStr;
      } else {
        spdlog::error("could not save file");
      }
    }

    cJSON_Delete(fileDataJson);
  }
}

cJSON * LoadJsonFile(std::string const & filename) {
  // load file
  auto file = std::ifstream{filename};
  if (file.eof() || !file.good()) {
    spdlog::error("could not load spritesheet '{}'", filename);
    return nullptr;
  }

  auto str =
    std::string {
      std::istreambuf_iterator<char>(file)
    , std::istreambuf_iterator<char>()
    };

  auto fileDataJson = cJSON_Parse(str.c_str());

  if (fileDataJson == nullptr) {
    spdlog::critical(
      " -- failed to parse json for '{}'; '{}'"
    , filename
    , cJSON_GetErrorPtr()
    );
  }

  return fileDataJson;
}

void LoadAnimation(
  std::string const & filename
, std::map<
    std::string
  , std::shared_ptr<pul::animation::Animator>
  > & animators
) {

  cJSON * fileDataJson = ::LoadJsonFile(filename);
  if (!fileDataJson) { return; }

  // iterate thru each spritesheet contained in file
  cJSON * sheetJson;
  cJSON_ArrayForEach(
    sheetJson
  , cJSON_GetObjectItemCaseSensitive(fileDataJson, "spritesheets")
  ) {
    auto animator = std::make_shared<pul::animation::Animator>();
    animator->filename = filename;
    animator->label =
      std::string{
        cJSON_GetObjectItemCaseSensitive(sheetJson, "label")->valuestring
      };

    animator->uvCoordOffset = {
      cJSON_GetObjectItemCaseSensitive(sheetJson, "uv-offset-x")->valueint
    , cJSON_GetObjectItemCaseSensitive(sheetJson, "uv-offset-y")->valueint
    };

    spdlog::debug("loading animation spritesheet '{}'", animator->label);
    // store animator
    animators[animator->label] = animator;

    animator->spritesheet =
      pul::gfx::Spritesheet::Construct(
        pul::gfx::Image::Construct(
          cJSON_GetObjectItemCaseSensitive(sheetJson, "filename")->valuestring
        )
      );

    cJSON * pieceJson;
    cJSON_ArrayForEach(
      pieceJson, cJSON_GetObjectItemCaseSensitive(sheetJson, "animation-piece")
    ) {
      std::string pieceLabel =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "label")->valuestring;

      pul::animation::Animator::Piece piece;

      piece.dimensions.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-x")->valueint;
      piece.dimensions.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-y")->valueint;

      piece.origin.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-x")->valueint;
      piece.origin.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-y")->valueint;

      {
        auto depth =
          cJSON_GetObjectItemCaseSensitive(pieceJson, "render-order")->valueint;
        if (depth < -127 || depth >= 127) {
          spdlog::error(
            "render-depth for '{}' of '{}' is OOB ({}); "
            "range must be from -127 to 127"
          , pieceLabel, animator->label, depth
          );
          depth = 0;
        }
        piece.renderDepth = depth;
      }

      cJSON * stateJson;
      cJSON_ArrayForEach(
        stateJson, cJSON_GetObjectItemCaseSensitive(pieceJson, "states")
      ) {
        std::string stateLabel =
          cJSON_GetObjectItemCaseSensitive(stateJson, "label")->valuestring;

        pul::animation::Animator::State state;

        state.msDeltaTime =
          cJSON_GetObjectItemCaseSensitive(
            stateJson, "ms-delta-time"
          )->valueint;

        state.rotationMirrored =
          cJSON_GetObjectItemCaseSensitive(stateJson, "rotation-mirrored")
            ->valueint
        ;

        state.originInterpolates =
          cJSON_GetObjectItemCaseSensitive(stateJson, "origin-interpolates")
            ->valueint
        ;

        state.loops =
          cJSON_GetObjectItemCaseSensitive(stateJson, "loops")
            ->valueint
        ;

        state.rotatePixels =
          cJSON_GetObjectItemCaseSensitive(stateJson, "rotate-pixels")
            ->valueint
        ;

        state.flipXAxis =
          cJSON_GetObjectItemCaseSensitive(stateJson, "flip-x-axis")
            ->valueint
        ;

        cJSON * variationJson;
        cJSON_ArrayForEach(
          variationJson
        , cJSON_GetObjectItemCaseSensitive(stateJson, "variations")
        ) {
          pul::animation::Variation variation;

          state.variationType =
            pul::animation::ToVariationType(
              cJSON_GetObjectItemCaseSensitive(
                stateJson, "variationType"
              )->valuestring
            );

          switch (state.variationType) {
            default: break;
            case pul::animation::VariationType::Normal:
              variation.normal.data =
                ::JsonLoadComponents(
                  cJSON_GetObjectItemCaseSensitive(variationJson, "default")
                );
            break;
            case pul::animation::VariationType::Random:
              variation.random.data =
                ::JsonLoadComponents(
                  cJSON_GetObjectItemCaseSensitive(variationJson, "default")
                );
            break;
            case pul::animation::VariationType::Range:
              variation.range.rangeMax =
                cJSON_GetObjectItemCaseSensitive(
                  variationJson, "angle-range-max"
                )->valuedouble
              ;

              variation.range.data[0] =
                ::JsonLoadComponents(
                  cJSON_GetObjectItemCaseSensitive(variationJson, "default")
                );

              variation.range.data[1] =
                ::JsonLoadComponents(
                  cJSON_GetObjectItemCaseSensitive(variationJson, "flipped")
                );
            break;
          }

          state.variations.emplace_back(std::move(variation));
        }

        piece.states[stateLabel] = std::move(state);
      }

      animator->pieces[pieceLabel] = std::move(piece);
    }

    // load skeleton
    ::JsonParseRecursiveSkeleton(sheetJson, animator->skeleton);
  }

  cJSON_Delete(fileDataJson);
}

} // -- namespace

extern "C" {
PUL_PLUGIN_DECL void Animation_LoadAnimations(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {

  auto & animationSystem = scene.AnimationSystem();

  { // -- create buffer
    sg_buffer_desc desc = {};
    desc.size = ::animationBufferMaxSize;
    desc.usage = SG_USAGE_STREAM;
    desc.content = nullptr;
    desc.label = "animation buffer";

    animationSystem.sgBuffer = std::make_unique<pul::gfx::SgBuffer>();
    animationSystem.sgBuffer->buffer = sg_make_buffer(&desc);
    animationSystem.sgBindings.vertex_buffers[0] =
      *animationSystem.sgBuffer;
    /* animationInstance.sgBindings.vertex_buffers[1] = */
    /*   *animationInstance.sgBufferUvCoord; */
    /* animationInstance.sgBindings.fs_images[0] = */
    /*   animationInstance.animator->spritesheet.Image(); */
  }

  { // load animations
    cJSON * spritesheetDataJson =
      ::LoadJsonFile("assets/base/spritesheets/data.json");

    cJSON * filenameJson;
    cJSON_ArrayForEach(
      filenameJson
    , cJSON_GetObjectItemCaseSensitive(spritesheetDataJson, "files")
    ) {
      spdlog::debug("loading json file '{}'", filenameJson->valuestring);
      ::LoadAnimation(
        std::string{filenameJson->valuestring}
      , animationSystem.animators
      );
    }

    cJSON_Delete(spritesheetDataJson);
  }

  { // -- sokol animation program
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[2].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[2].uniforms[0].name = "cameraOrigin";
    desc.vs.uniform_blocks[2].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[3].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[3].uniforms[0].name = "textureResolution";
    desc.vs.uniform_blocks[3].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    /* desc.fs.images[0].name = "baseSampler"; */
    /* desc.fs.images[0].type = SG_IMAGETYPE_2D; */

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec3 inOrigin;
      layout(location = 1) in vec2 inUvCoord;

      out vec2 uvCoord;
      out vec2 vertexCoord;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;
      uniform vec2 cameraOrigin;

      const vec2 vertexArray[6] = vec2[](
        vec2(0.0f,  0.0f)
      , vec2(1.0f,  1.0f)
      , vec2(1.0f,  0.0f)

      , vec2(0.0f,  0.0f)
      , vec2(0.0f,  1.0f)
      , vec2(1.0f,  1.0f)
      );

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin.xy)*vec2(1,-1) * framebufferScale;
        vertexOrigin +=
          (originOffset-cameraOrigin)*vec2(1, -1) * framebufferScale
        ;
        gl_Position = vec4(vertexOrigin, 0.5001f + inOrigin.z/100000.0f, 1.0f);
        uvCoord = vec2(inUvCoord.x, 1.0f-inUvCoord.y);
        vertexCoord = vertexArray[gl_VertexID%6];
      }
    );

    desc.fs.source = PUL_SHADER(
      //uniform sampler2D baseSampler;

      in vec2 uvCoord;
      in vec2 vertexCoord;

      out vec4 outColor;

      void main() {
        /* outColor = texture(baseSampler, uvCoord); */
        outColor = vec4(uvCoord, 0.5f, 1.0f);
        if (outColor.a < 0.1f)
          { discard; }
        if (
            vertexCoord.y < 0.0001f || vertexCoord.x < 0.0001f
         || vertexCoord.y > 0.9999f || vertexCoord.x > 0.9999f
        ) { discard; }
      }
    );

    animationSystem.sgProgram = sg_make_shader(&desc);
  }

  { // -- sokol pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = sizeof(glm::vec4)*2;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[0].step_rate = 1u;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;

    desc.layout.buffers[1].stride = sizeof(glm::vec4)*2;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[1].step_rate = 1u;
    desc.layout.attrs[1].buffer_index = 0;
    desc.layout.attrs[1].offset = sizeof(glm::vec4);
    desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    desc.index_type = SG_INDEXTYPE_NONE;

    desc.shader = animationSystem.sgProgram;
    desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
    desc.depth_stencil.depth_write_enabled = true;

    desc.blend.enabled = false;

    // TODO possibly make the below SG_CULLMODE_BACK if i get flipping worked
    // out
    desc.rasterizer.cull_mode = SG_CULLMODE_NONE;
    desc.rasterizer.alpha_to_coverage_enabled = false;
    desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
    desc.rasterizer.sample_count = 1;

    desc.label = "animation pipeline";

    animationSystem.sgPipeline = sg_make_pipeline(&desc);
  }
}

PUL_PLUGIN_DECL void Animation_Shutdown(pul::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  ::SaveAnimations(scene.AnimationSystem());

  { // -- delete sokol animation information
    auto view = registry.view<pul::animation::ComponentInstance>();

    for (auto entity : view) {
      auto & self = view.get<pul::animation::ComponentInstance>(entity);

      self.instance = {};
    }
  }

  sg_destroy_shader(scene.AnimationSystem().sgProgram);
  sg_destroy_pipeline(scene.AnimationSystem().sgPipeline);

  scene.AnimationSystem() = {};
}

PUL_PLUGIN_DECL void Animation_UpdateFrame(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  // update each component
  auto view = registry.view<pul::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pul::animation::ComponentInstance>(entity);

    ::ComputeCache(
      self.instance, self.instance.animator->skeleton
    , glm::mat3(1.0f), false, 0.0f
    );

    ::ComputeVertices(scene, self.instance, true);
  }
}

PUL_PLUGIN_DECL void Animation_RenderAnimations(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  // -- render sokol animations

  auto & animationSystem = scene.AnimationSystem();

  // bind pipeline & global uniforms
  sg_apply_pipeline(animationSystem.sgPipeline);

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 1
  , &scene.config.framebufferDimFloat.x
  , sizeof(float) * 2ul
  );

  auto cameraOrigin = glm::vec2(scene.cameraOrigin);

  sg_apply_uniforms(
    SG_SHADERSTAGE_VS
  , 2
  , &cameraOrigin.x
  , sizeof(float) * 2ul
  );

  static std::vector<glm::vec4> bufferData;
  static std::vector<size_t> imageData;

  // set capacity and set size to 0
  if (bufferData.capacity() == 0ul)
    { bufferData.reserve(animationBufferMaxSize / sizeof(glm::vec4)); }
  bufferData.resize(0); // doesn't affect capacity

  // DEBUG
  debugRenderingInstances.resize(0);

  auto const
    cullBoundLeft  = cameraOrigin.x - scene.config.framebufferDimFloat.x/2.0f
  , cullBoundRight = cameraOrigin.x + scene.config.framebufferDimFloat.x/2.0f
  , cullBoundUp    = cameraOrigin.y - scene.config.framebufferDimFloat.y/2.0f
  , cullBoundDown  = cameraOrigin.y + scene.config.framebufferDimFloat.y/2.0f
  ;

  // record each component to a buffer
  auto view = registry.view<pul::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pul::animation::ComponentInstance>(entity);

    if (self.instance.automaticCachedMatrixCalculation)
      { self.instance.hasCalculatedCachedInfo = false; }

    // check if visible / cull / ready to render
    if (
        !self.instance.visible
     || self.instance.drawCallCount == 0ul
     || cullBoundLeft > self.instance.origin.x
     || cullBoundRight < self.instance.origin.x
     || cullBoundUp > self.instance.origin.y
     || cullBoundDown < self.instance.origin.y
    ) { continue; }

    // DEBUG
    debugRenderingInstances.emplace_back(&self.instance);

    for (size_t it = 0; it < self.instance.originBufferData.size(); ++ it) {
      auto const origin =
        self.instance.originBufferData[it]
      + glm::vec3(self.instance.origin, 0.0f)
      ;

      bufferData.emplace_back(glm::vec4(origin, 0.0f));
      bufferData.emplace_back(
        glm::vec4(self.instance.uvCoordBufferData[it], 0.0f, 0.0f)
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

PUL_PLUGIN_DECL void Animation_UpdateCache(
  pul::animation::Instance & instance
) {
  ::ComputeCache(
    instance, instance.animator->skeleton, glm::mat3(1.0f), false, 0.0f
  );
  instance.hasCalculatedCachedInfo = true;
}

PUL_PLUGIN_DECL void Animation_UpdateCacheWithPrecalculatedMatrix(
  pul::animation::Instance & instance
, glm::mat3 const & skeletalMatrix
) {
  ::ComputeCache(
    instance, instance.animator->skeleton, skeletalMatrix, false, 0.0f
  );
  instance.hasCalculatedCachedInfo = true;
}

PUL_PLUGIN_DECL void Animation_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  ImGui::Begin("Animation");

  auto & registry = scene.EnttRegistry();

  static pul::animation::Animator * editAnimator = nullptr;

  { // -- display spritesheet info
    ImGui::Text("spritesheets");

    if (ImGui::Button("save animations"))
      { ::SaveAnimations(scene.AnimationSystem()); }

    ImGui::Separator();
    ImGui::Separator();

    for (auto & animatorPair : scene.AnimationSystem().animators) {
      auto & animator = *animatorPair.second;

      ImGui::PushID(&animator);

      pul::imgui::Text("{}", animator.label);
      if (ImGui::Button("edit")) { editAnimator = &animator; }

      ImGui::Separator();

      ImGui::PopID();
    }
  }

  ImGui::NewLine();

  { // -- display entity info
    auto view = registry.view<pul::animation::ComponentInstance>();

    ImGui::Text("instances");

    ImGui::Separator();
    ImGui::Separator();

    // TODO this should be moved to the entity viewer

    for (auto entity : view) {
      auto & self = view.get<pul::animation::ComponentInstance>(entity);

      ImGui::PushID(&self);

      PUL_ASSERT(self.instance.animator, continue;);

      if (ImGui::TreeNode(self.instance.animator->label.c_str())) {
        for (auto const & stateInfoPair : self.instance.pieceToState) {
          auto & stateInfo = stateInfoPair.second;

          pul::imgui::Text("part - '{}'", stateInfoPair.first);
          pul::imgui::Text("\torigin '{}'", self.instance.origin);
          pul::imgui::Text("\tlabel '{}'", stateInfo.label);
          pul::imgui::Text("\tdelta-time {}", stateInfo.deltaTime);
          pul::imgui::Text(
            "\tanimation-finished {}", stateInfo.animationFinished
          );
          pul::imgui::Text("\tcomponent-it {}", stateInfo.componentIt);
          pul::imgui::Text("\tflip {}", stateInfo.flip);
          pul::imgui::Text("\tangle {}", stateInfo.angle);
          pul::imgui::Text("\tvisible {}", stateInfo.visible);
          pul::imgui::Text("\tuvCoordWrap '{}'", stateInfo.uvCoordWrap);

          auto & mat = stateInfo.cachedLocalSkeletalMatrix;
          pul::imgui::Text(
            "\tcached matrix\n"
            "\t\t{} {} {}\n\t\t{} {} {}\n\t\t{} {} {}"
          , mat[0][0], mat[0][1], mat[0][2]
          , mat[1][0], mat[1][1], mat[1][2]
          , mat[2][0], mat[2][1], mat[2][2]
          );

          auto variationType =
            self.instance.animator
              ->pieces[stateInfoPair.first]
              .states[stateInfoPair.second.label]
              .variationType
          ;

          pul::imgui::Text("\tvariation type '{}'", ToStr(variationType));

          switch (variationType) {
            default: break;
            case pul::animation::VariationType::Normal:
            break;
            case pul::animation::VariationType::Range:
            break;
            case pul::animation::VariationType::Random:
              pul::imgui::Text(
                "random it {}", stateInfo.variationRti.random.idx
              );
            break;
          }
        }

        ImGui::TreePop();
      }

      ImGui::PopID();
    }
  }
  ImGui::End();

  if (editAnimator) {

    ImGui::Begin("Animation Timeline");
      ImGui::Checkbox("loop", &animLoop);
      ImGui::Checkbox("playing", &animPlaying);
      ImGui::Checkbox("empty on loop end", &animEmptyOnLoopEnd);

      ImGui::Separator();

      pul::imgui::InputInt("max loop time", &animMaxTime);
      pul::imgui::SliderInt("timer", &animMsTimer, 0, animMaxTime);

      ImGui::Separator();

      ImGui::Checkbox("sprite zoom", &animShowZoom);

      ImGui::SliderFloat(
        "previous sprite alpha", &animShowPreviousSprite, 0.0f, 1.0f
      );

      if (animPlaying) {
        animMsTimer += pul::util::MsPerFrame * scene.numCpuFrames;

        animMsTimer =
            animLoop
          ? animMsTimer % animMaxTime : glm::min(animMsTimer, animMaxTime)
        ;
      }
    ImGui::End();

    auto & animator = *editAnimator;

    ImGui::Begin("Animation Skeleton");
      ImGui::Separator();
      ImGui::Text("skeleton");
      ImGui::Separator();
      ImGui::Text(" ");
      DisplayImGuiSkeleton(
        scene, animator, scene.AnimationSystem(), animator.skeleton
      );

      ImGui::Separator();
      ImGui::Separator();

    ImGui::End();

    ImGui::Begin("Animation Editor");

    ImGui::Separator();

    pul::imgui::Text("animator '{}'", animator.label);

    ImGui::Separator();

    pul::imgui::InputInt2("uv offset", &animator.uvCoordOffset.x);

    ImGui::Text("pieces");
    ImGui::Separator();
    ImGui::Separator();

    { // piece add
      if (ImGui::Button("add piece")) {
        ImGui::OpenPopup("adding piece");
      }

      if (ImGui::BeginPopup("adding piece")) {
        static std::string pieceLabel = "";
        if (
          pul::imgui::InputText(
            "label", &pieceLabel, ImGuiInputTextFlags_EnterReturnsTrue
          )
        ) {
          if (pieceLabel != "") {
            animator.pieces[pieceLabel] = {};
          }
          pieceLabel = "";
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }

    size_t pieceIt = 0ul;
    for (auto & piecePair : animator.pieces) {
      ImGui::Separator();

      auto & piece = piecePair.second;

      ImGui::PushID(++ pieceIt);

      if (ImGui::TreeNode(piecePair.first.c_str())) {

      { // delete piece
        if (ImGui::Button("x"))
          { ImGui::OpenPopup("piece delete confirm"); }

        if (ImGui::BeginPopup("piece delete confirm")) {
          if (ImGui::Button("confirm deletion")) {
            animator.pieces.erase(animator.pieces.find(piecePair.first));
            ImGui::EndPopup();
            ImGui::TreePop();
            break;
          }
          ImGui::EndPopup();
        }
      }


        pul::imgui::Text(
          "img dimensions {}x{}"
        , animator.spritesheet.width, animator.spritesheet.height
        );

        pul::imgui::InputInt2("dimensions", &piece.dimensions.x);
        pul::imgui::DragInt2("origin", &piece.origin.x, 0.01f);
        pul::imgui::DragInt("render depth", &piece.renderDepth, 0.01f);

        { // state adding
          if (ImGui::Button("add state")) {
            ImGui::OpenPopup("state add confirm");
          }

          if (ImGui::BeginPopup("state add confirm")) {
            static std::string newStateLabel = "";
            if (
              pul::imgui::InputText(
                "label", &newStateLabel, ImGuiInputTextFlags_EnterReturnsTrue
              )
            ) {
              if (newStateLabel != "") {
                piece.states[newStateLabel] = {};
              }
              newStateLabel = "";
              ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
          }
        }

        for (auto & statePair : piece.states) {
          ImGui::Separator();

          if (!ImGui::TreeNode(statePair.first.c_str())) {
            continue;
          }

          { // delete state
            if (ImGui::Button("x"))
              { ImGui::OpenPopup("state delete confirm"); }

            if (ImGui::BeginPopup("state delete confirm")) {
              if (ImGui::Button("confirm deletion")) {
                piece.states.erase(piece.states.find(statePair.first));
                ImGui::EndPopup();
                ImGui::TreePop();
                break;
              }
              ImGui::EndPopup();
            }
          }

          auto & state = statePair.second;
          (void)state;

          pul::imgui::DragInt(
            "delta time", &statePair.second.msDeltaTime, 1.0f
          );

          auto & variations = statePair.second.variations;

          bool variationChanged = false;

          // select variation, but can only be done if no variations exist
          if (
            ImGui::BeginCombo("variation", ToStr(state.variationType))
          ) {
            if (variations.empty()) {
              for (
                size_t i = 0;
                i < Idx(pul::animation::VariationType::Size);
                ++ i
              ) {
                auto v = static_cast<pul::animation::VariationType>(i);
                if (ImGui::Selectable(ToStr(v), v == state.variationType)) {
                  state.variationType = v;
                }
              }
            }

            ImGui::EndCombo();
          }

          if (ImGui::TreeNode("metadata")) {
            ImGui::Checkbox("rotation mirrored", &state.rotationMirrored);
            ImGui::SameLine();
            ImGui::Checkbox("rotate pixels", &state.rotatePixels);
            ImGui::Checkbox("loops", &state.loops);
            ImGui::Checkbox("origin interpolates", &state.originInterpolates);
            ImGui::Checkbox("flip X axis", &state.flipXAxis);

            ImGui::TreePop();
          }

          // add part only works under certain conditions
          bool hasAddPartOption = false;
          switch (state.variationType) {
            default: break;
            case pul::animation::VariationType::Normal:
              hasAddPartOption = variations.empty();
            break;
            case pul::animation::VariationType::Range:
              hasAddPartOption = true;
            break;
            case pul::animation::VariationType::Random:
              hasAddPartOption = true;
            break;
          }

          if (hasAddPartOption && ImGui::Button("add part")) {
            variations.emplace_back();
            variationChanged = true;
          }

          for (
            size_t variationIt = 0ul;
            variationIt < variations.size();
            ++ variationIt
          ) {

            ImGui::PushID(&variations[variationIt]);

            auto & variation = variations[variationIt];

            ImGui::Separator();

            // -- render tree / split
            bool componentTreeNode = false;

            switch (state.variationType) {
              default: spdlog::critical("unknown variation type"); break;
              case pul::animation::VariationType::Range:
              componentTreeNode =
                ImGui::TreeNode(
                  fmt::format("{}", variationIt).c_str()
                , "%s"
                , fmt::format(
                    "[{:.2f}]"
                  , variation.range.rangeMax * 360.0f / pul::Tau
                  ).c_str()
                );
              break;
              case pul::animation::VariationType::Random:
                componentTreeNode =
                  ImGui::TreeNode(
                    fmt::format("{}", variationIt).c_str()
                  , "%s"
                  , fmt::format("{}", variationIt).c_str()
                  );
              break;
              case pul::animation::VariationType::Normal:
                componentTreeNode =
                  ImGui::TreeNode(
                    fmt::format("{}", variationIt).c_str()
                  , "%s"
                  , fmt::format("{}", variationIt).c_str()
                  );
              break;
            }


            if (!componentTreeNode) {
              ImGui::PopID();
              continue;
            }

            if (state.variationType == pul::animation::VariationType::Range)
            { // draw angle
              ImDrawList * drawList = ImGui::GetWindowDrawList();

              ImVec2 pos = ImGui::GetCursorScreenPos();
              pos.x += 96;

              ImVec4 col = ImVec4(1, 1, 1, 1);

              drawList->AddCircle(pos, 16.0f, ImColor(col), 20, 1.0f);

              float const theta = variation.range.rangeMax + pul::Pi*0.5f;

              // render angles for both sides
              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x + 16.0f * glm::cos(theta)
                , pos.y + 16.0f - 32.0f * (variation.range.rangeMax/pul::Pi)
                )
              , ImColor(ImVec4(1.0f, 0.7f, 0.7f, 1.0f))
              , 2.0f
              );

              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x - 16.0f * glm::cos(theta)
                , pos.y + 16.0f - 32.0f * (variation.range.rangeMax/pul::Pi)
                )
              , ImColor(ImVec4(1.0f, 0.7f, 0.7f, 1.0f))
              , 2.0f
              );

              // render angles for both sides for previous
              float const prevRange =
                  variationIt == 0ul
                ? 0.0f : variations[variationIt-1].range.rangeMax
              ;
              float const prevTheta = prevRange + pul::Pi*0.5f;

              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x + 16.0f * glm::cos(prevTheta)
                , pos.y + 16.0f - 32.0f * (prevRange / pul::Pi)
                )
              , ImColor(ImVec4(0.7f, 0.7f, 1.0f, 1.0f))
              , 1.0f
              );

              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x - 16.0f * glm::cos(prevTheta)
                , pos.y + 16.0f - 32.0f * (prevRange / pul::Pi)
                )
              , ImColor(ImVec4(0.7f, 0.7f, 1.0f, 1.0f))
              , 1.0f
              );

            }

            if (ImGui::Button("remove")) {
              variations.erase(variations.begin()+variationIt);
              -- variationIt;
              ImGui::PopID();
            }

            switch (state.variationType) {
              default: break;
              case pul::animation::VariationType::Normal: {
                ImGui::Separator();

                auto & components = variation.normal.data;

                // add components by default TODO this should prolly happen
                // automatically
                if (components.size() == 0ul) { components.emplace_back(); }

                DisplayImGuiComponents(
                  animator, state, piece, components
                );
              }
              break;
              case pul::animation::VariationType::Random: {
                ImGui::Separator();

                auto & components = variation.random.data;

                // add components by default TODO this should prolly happen
                // automatically
                if (components.size() == 0ul) { components.emplace_back(); }

                DisplayImGuiComponents(
                  animator, state, piece, components
                );
              }
              break;
              case pul::animation::VariationType::Range: {
                ImGui::DragFloat(
                  "angle range", &variation.range.rangeMax
                , 0.0025f, 0.0f, pul::Pi, "%.2f"
                );

                variationChanged |= ImGui::IsItemActive();

                ImGui::Separator();

                auto & componentsDefault = variation.range.data[0];
                auto & componentsFlipped = variation.range.data[1];

                if (
                    componentsDefault.size() == 0ul
                 && ImGui::Button("add default")
                ) {
                  componentsDefault.emplace_back();
                }

                DisplayImGuiComponents(
                  animator, state, piece, componentsDefault
                );

                if (componentsFlipped.size() > 0ul) {

                  ImGui::Text("flipped state");
                  ImGui::Separator();

                  ImGui::PushID(1);

                  DisplayImGuiComponents(
                    animator, state, piece, componentsFlipped
                  );

                  ImGui::PopID();
                } else {
                  if (ImGui::Button("add flipped")) {
                    componentsFlipped.emplace_back();
                  }
                }
              } break;
            }

            ImGui::PopID();
            ImGui::TreePop();
          }

          // post modification (ei sorting)
          switch (state.variationType) {
            default: break;
            case pul::animation::VariationType::Normal: break;
            case pul::animation::VariationType::Random: break;
            case pul::animation::VariationType::Range:
              // can only sort when user is not modifying anymore
              if (!variationChanged) {
                std::sort(
                  variations.begin(), variations.end()
                , [](auto & partA, auto& partB) {
                    return partA.range.rangeMax < partB.range.rangeMax;
                  }
                );
              }
            break;
          }

          ImGui::TreePop(); // state
        }

        ImGui::TreePop(); // piece
      }

      ImGui::PopID(); // pieceIt
    }

    ImGui::End();
  }

  ImGui::Begin("animation debug");
  ImGui::Text("total rendering %lu", debugRenderingInstances.size());
  for (auto & render : debugRenderingInstances) {
    ImGui::Text("'%s'", render->animator->label.c_str());
  }
  ImGui::End();
}

} // -- extern C
