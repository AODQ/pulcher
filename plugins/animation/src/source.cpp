#include <pulcher-animation/animation.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-gfx/context.hpp>
#include <pulcher-gfx/image.hpp>
#include <pulcher-gfx/imgui.hpp>
#include <pulcher-gfx/spritesheet.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <entt/entt.hpp>

#include <cjson/cJSON.h>
#include <imgui/imgui.hpp>
#include <sokol/gfx.hpp>

#include <glm/gtx/transform2.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

#include <fstream>

// animation could always use cleaning / optimizing as a lot of it isn't based
// on anything concrete, it's all developed pretty rapidly using whatever just
// worked at the time

// this implements the logic for the 'pixel-perfect' animation skeletal system

namespace {

static size_t animMsTimer = 0ul;
static bool animLoop = true;
static float animShowPreviousSprite = 0.0f;
static bool animShowZoom = false;
static bool animPlaying = true;
static bool animEmptyOnLoopEnd = false;
static size_t animMaxTime = 100'000ul;

void JsonParseRecursiveSkeleton(
  cJSON * skeletalParentJson
, std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
) {
  if (!skeletalParentJson) { return; }
  cJSON * skeletalChildJson;
  cJSON_ArrayForEach(
    skeletalChildJson
  , cJSON_GetObjectItemCaseSensitive(skeletalParentJson, "skeleton")
  ) {
    pulcher::animation::Animator::SkeletalPiece skeletal;
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

std::vector<pulcher::animation::Animator::Component> JsonLoadComponents(
  cJSON * componentsJson
) {
  std::vector<pulcher::animation::Animator::Component> components;

  cJSON * componentJson;
  cJSON_ArrayForEach(componentJson, componentsJson) {
    pulcher::animation::Animator::Component component;
    component.tile.x =
      cJSON_GetObjectItemCaseSensitive(componentJson, "x")->valueint;

    component.tile.y =
      cJSON_GetObjectItemCaseSensitive(componentJson, "y")->valueint;

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
  std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
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
  std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
) {
  size_t vertices = 0ul;
  for (const auto & skeletal : skeletals) {
    vertices += 6 + ComputeVertexBufferSize(skeletal.children);
  }
  return vertices;
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, pulcher::animation::Animator::SkeletalPiece const & skeletal
, size_t & indexOffset
, bool & skeletalFlip
, float & skeletalRotation
, bool & forceUpdate
) {
  // okay the below is crazy with the map lookups, I need to cache the
  // state somehow
  auto & piece = instance.animator->pieces[skeletal.label];
  auto & stateInfo = instance.pieceToState[skeletal.label];
  auto & state = piece.states[stateInfo.label];

  // update skeletal information (origins, flip, rotation, etc)
  skeletalFlip ^= stateInfo.flip;

  skeletalRotation += stateInfo.angle;

  if (state.rotationMirrored && ((skeletalRotation > 0.0f) ^ skeletalFlip)) {
    skeletalFlip ^= 1;
  }

  // grab component from flip/rotation
  auto * componentsPtr =
    state.ComponentLookup(
      skeletalFlip , skeletalFlip ? skeletalRotation : -skeletalRotation
    );

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

  PUL_ASSERT_CMP(
    stateInfo.componentIt, <, components.size()
  , stateInfo.componentIt = components.size()-1;
  );

  auto & component = components[stateInfo.componentIt];

  // update delta time and if animation update is necessary, apply uv coord
  // updates
  bool hasUpdate = forceUpdate;
  if (state.msDeltaTime > 0.0f) {
    stateInfo.deltaTime += pulcher::util::msPerFrame;
    if (stateInfo.deltaTime > state.msDeltaTime) {
      stateInfo.deltaTime = stateInfo.deltaTime - state.msDeltaTime;
      stateInfo.componentIt = (stateInfo.componentIt + 1) % components.size();
      hasUpdate = true;
    }
  }

  if (!hasUpdate) {
    indexOffset += 6ul;
    return;
  }

  auto pieceDimensions = glm::vec2(piece.dimensions);

  // update origins & UV coords
  for (size_t it = 0ul; it < 6; ++ it, ++ indexOffset) {
    auto v = pulcher::util::TriangleVertexArray()[it];

    // flip uv coords if requested
    auto uv = v;
    if (skeletalFlip) { uv.x = 1.0f - uv.x; }

    instance.uvCoordBufferData[indexOffset] =
        (uv*pieceDimensions + glm::vec2(component.tile)*pieceDimensions)
      * instance.animator->spritesheet.InvResolution()
    ;
    auto origin = glm::vec3(v*pieceDimensions, 1.0f);

    origin = stateInfo.cachedLocalSkeletalMatrix * origin;

    instance.originBufferData[indexOffset] =
        glm::vec3(origin.x, origin.y, static_cast<float>(piece.renderOrder));
  }
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
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
      instance, skeletal, indexOffset
    , newSkeletalFlip, newSkeletalRotation, newForceUpdate
    );

    // continue to children
    ComputeVertices(
      instance, skeletal.children, indexOffset
    , newSkeletalFlip, newSkeletalRotation, newForceUpdate
    );
  }
}

void ComputeVertices(
  pulcher::animation::Instance & instance
, bool forceUpdate = false
) {
  size_t indexOffset = 0ul;
  ComputeVertices(
    instance, instance.animator->skeleton, indexOffset, false, 0.0f, forceUpdate
  );
}

void ComputeCache(
  pulcher::animation::Instance & instance
, pulcher::animation::Animator::SkeletalPiece const & skeletal
, glm::mat3 & skeletalMatrix
, bool & skeletalFlip
, float & skeletalRotation
) {
  auto & piece = instance.animator->pieces[skeletal.label];
  auto & stateInfo = instance.pieceToState[skeletal.label];
  auto & state = piece.states[stateInfo.label];

  // update skeletal information (origins, flip, rotation, etc)
  skeletalFlip ^= stateInfo.flip;

  skeletalRotation += stateInfo.angle;

  if (state.rotationMirrored && ((skeletalRotation > 0.0f) ^ skeletalFlip)) {
    skeletalFlip ^= 1;
  }

  float const theta = -skeletalRotation - pul::Pi*0.5f + skeletalFlip*pul::Pi;

  // grab component from flip/rotation
  auto * componentsPtr =
    state.ComponentLookup(
      skeletalFlip , skeletalFlip ? skeletalRotation : -skeletalRotation
    );

  PUL_ASSERT(
    componentsPtr && componentsPtr->size() > 0ul
  , return;
  );

  auto & components = *componentsPtr;

  PUL_ASSERT_CMP(
    stateInfo.componentIt, <, components.size()
  , stateInfo.componentIt = components.size()-1;
  );

  auto & component = components[stateInfo.componentIt];

  // -- compose skeletal matrix
  // first rotate locally by offset into the pixel it rotates around
  glm::mat3 localMatrix = glm::mat3(1);
  glm::vec2 localOrigin = piece.origin + component.originOffset;

  if (skeletalFlip) {
    localOrigin.x = piece.dimensions.x - localOrigin.x;
  }

  localMatrix =
    glm::translate(
      localMatrix, glm::vec2(localOrigin)
    );

  // flip origin

  if (state.rotatePixels) {
    localMatrix = glm::rotate(localMatrix, theta);
  }

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
  pulcher::animation::Instance & instance
, std::vector<pulcher::animation::Animator::SkeletalPiece> const & skeletals
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
PUL_PLUGIN_DECL void Animation_DestroyInstance(
  pulcher::animation::Instance & instance
) {
  if (instance.sgBufferOrigin.id) {
    sg_destroy_buffer(instance.sgBufferOrigin);
    instance.sgBufferOrigin = {};
  }

  if (instance.sgBufferUvCoord.id) {
    sg_destroy_buffer(instance.sgBufferUvCoord);
    instance.sgBufferUvCoord = {};
  }

  instance.uvCoordBufferData = {};
  instance.originBufferData = {};
  instance.pieceToState = {};
  instance.animator = {};
}

PUL_PLUGIN_DECL void Animation_ConstructInstance(
  pulcher::animation::Instance & animationInstance
, pulcher::animation::System & animationSystem
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
    animationInstance.pieceToState[piecePair.first] =
      {piecePair.second.states.begin()->first};
  }

  { // -- compute initial sokol buffers

    // precompute size
    size_t const vertexBufferSize =
      ::ComputeVertexBufferSize(animationInstance.animator->skeleton);
    spdlog::debug("vertex buffer size {}", vertexBufferSize);

    animationInstance.uvCoordBufferData.resize(vertexBufferSize);
    animationInstance.originBufferData.resize(vertexBufferSize);

    ::ComputeVertices(animationInstance, true);

    { // -- origin
      sg_buffer_desc desc = {};
      desc.size = vertexBufferSize * sizeof(glm::vec3);
      desc.usage = SG_USAGE_STREAM;
      desc.content = animationInstance.originBufferData.data();
      desc.label = "origin buffer";
      animationInstance.sgBufferOrigin = sg_make_buffer(&desc);
    }

    { // -- uv coord
      sg_buffer_desc desc = {};
      desc.size = vertexBufferSize * sizeof(glm::vec2);
      desc.usage = SG_USAGE_STREAM;
      desc.content = animationInstance.uvCoordBufferData.data();
      desc.label = "uv coord buffer";
      animationInstance.sgBufferUvCoord = sg_make_buffer(&desc);
    }

    // bindings
    animationInstance.sgBindings.vertex_buffers[0] =
      animationInstance.sgBufferOrigin;
    animationInstance.sgBindings.vertex_buffers[1] =
      animationInstance.sgBufferUvCoord;
    animationInstance.sgBindings.fs_images[0] =
      animationInstance.animator->spritesheet.Image();

    // get draw call count
    animationInstance.drawCallCount = vertexBufferSize;
  }
}

} // -- extern C

namespace {

void ReconstructInstances(pulcher::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();
  auto & system = scene.AnimationSystem();
  auto view = registry.view<pulcher::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pulcher::animation::ComponentInstance>(entity);
    auto label = self.instance.animator->label;
    Animation_DestroyInstance(self.instance);
    Animation_ConstructInstance(self.instance, system, label.c_str());
  }
}

void ImGuiRenderSpritesheetTile(
  pulcher::gfx::Spritesheet & spritesheet
, pulcher::animation::Animator::Piece & piece
, pulcher::animation::Animator::Component & component
, float alpha = 1.0f
) {
  auto pieceDimensions = glm::vec2(piece.dimensions);
  auto const imgUl =
    glm::vec2(component.tile)*pieceDimensions
  * spritesheet.InvResolution()
  ;

  auto const imgLr =
    imgUl + pieceDimensions * spritesheet.InvResolution()
  ;

  ImVec2 dimensions = ImVec2(piece.dimensions.x, piece.dimensions.y);

  if (animShowZoom) {
    dimensions.x *= 5.0f;
    dimensions.y *= 5.0f;
  }

  ImGui::Image(
    reinterpret_cast<void *>(spritesheet.Image().id)
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
  pulcher::animation::Animator & animator
, pulcher::animation::Animator::Piece & piece
, std::vector<pulcher::animation::Animator::Component> & components
, size_t componentIt
) {
  auto & component = components[componentIt];

  ImGui::PushID(componentIt);

  // do not show columns or editor properties on sprite zoom
  if (!::animShowZoom) {
    ImGui::Columns(2, nullptr, false);

    ImGui::SetColumnWidth(0, piece.dimensions.x + 16);
  }

  // display current image on left side of column, use transparency
  // w/ previous frame if requested
  ImVec2 pos = ImGui::GetCursorScreenPos();
  if (animShowPreviousSprite > 0.0f) {
    ::ImGuiRenderSpritesheetTile(
      animator.spritesheet, piece
    , components[
        (components.size() + componentIt - 1) % components.size()
      ]
    , animShowPreviousSprite
    );
  }
  ImGui::SetCursorScreenPos(pos);
  ::ImGuiRenderSpritesheetTile(animator.spritesheet, piece, component);

  if (!::animShowZoom) {
    ImGui::NextColumn();
    pul::imgui::DragInt2("tile", &component.tile.x, 0.025f);

    pul::imgui::DragInt2("origin-offset", &component.originOffset.x, 0.025f);

    if (ImGui::Button("-")) {
      components.erase(components.begin() + componentIt);
      -- componentIt;
    }

    ImGui::SameLine();
    ImGui::SameLine();
    if (ImGui::Button("+")) {
      components
        .emplace(components.begin() + componentIt + 1, component);
      components.back().tile.x += 1;
    }

    ImGui::SameLine();
    if (componentIt > 0ul && ImGui::Button("^")) {
      std::swap(components[componentIt], components[componentIt-1]);
    }

    ImGui::SameLine();
    if (componentIt < components.size()-1 && ImGui::Button("V")) {
      std::swap(components[componentIt], components[componentIt+1]);
    }

    ImGui::NextColumn();
  }

  ImGui::PopID(); // componentIt
  ImGui::Separator();

  ImGui::Columns(1);
}

void DisplayImGuiSkeleton(
  pulcher::core::SceneBundle & scene
, pulcher::animation::Animator & animator
, pulcher::animation::System & system
, std::vector<pulcher::animation::Animator::SkeletalPiece> & skeletals
) {

  ImGui::SameLine();
  if (ImGui::Button("add"))
    { ImGui::OpenPopup("add skeletal"); }

  if (ImGui::BeginPopup("add skeletal")) {
    for (auto & piecePair : animator.pieces) {
      auto & label = std::get<0>(piecePair);
      if (ImGui::Selectable(label.c_str())) {
        skeletals.emplace_back(
          pulcher::animation::Animator::SkeletalPiece{label}
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
  std::vector<pulcher::animation::Animator::Component> const & components
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
  }

  return componentsJson;
}

cJSON * SaveAnimation(pulcher::animation::Animator& animator) {
  cJSON * spritesheetJson = cJSON_CreateObject();

  cJSON_AddItemToObject(
    spritesheetJson, "label", cJSON_CreateString(animator.label.c_str())
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
      pieceJson, "render-order", cJSON_CreateInt(piece.renderOrder)
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
        stateJson, "rotate-pixels"
      , cJSON_CreateInt(state.rotatePixels)
      );

      cJSON * componentPartsJson = cJSON_CreateArray();
      cJSON_AddItemToObject(stateJson, "components", componentPartsJson);

      for (auto const & componentPart : state.components) {
        cJSON * componentPartJson = cJSON_CreateObject();
        cJSON_AddItemToArray(componentPartsJson, componentPartJson);

        cJSON_AddItemToObject(
          componentPartJson, "angle-range-max"
        , cJSON_CreateNumber(static_cast<double>(componentPart.rangeMax))
        );

        cJSON_AddItemToObject(
          componentPartJson, "default"
        , SaveAnimationComponent(componentPart.data[0])
        );

        cJSON_AddItemToObject(
          componentPartJson, "flipped"
        , SaveAnimationComponent(componentPart.data[1])
        );
      }
    }
  }

  return spritesheetJson;
}

void SaveAnimations(pulcher::animation::System & system) {
  // save each animation into its own map to seperate them out by file
  std::map<std::string, std::vector<cJSON *>> filenameToCJson;
  for (auto & animatorPair : system.animators) {
    auto & animator = *animatorPair.second;
    spdlog::debug(
      "attempting to save '{}' to '{}'", animator.label, animator.filename
    );
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
  , std::shared_ptr<pulcher::animation::Animator>
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
    auto animator = std::make_shared<pulcher::animation::Animator>();
    animator->filename = filename;
    animator->label =
      std::string{
        cJSON_GetObjectItemCaseSensitive(sheetJson, "label")->valuestring
      };

    spdlog::debug("loading animation spritesheet '{}'", animator->label);
    // store animator
    animators[animator->label] = animator;

    animator->spritesheet =
      pulcher::gfx::Spritesheet::Construct(
        pulcher::gfx::Image::Construct(
          cJSON_GetObjectItemCaseSensitive(sheetJson, "filename")->valuestring
        )
      );

    cJSON * pieceJson;
    cJSON_ArrayForEach(
      pieceJson, cJSON_GetObjectItemCaseSensitive(sheetJson, "animation-piece")
    ) {
      std::string pieceLabel =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "label")->valuestring;

      pulcher::animation::Animator::Piece piece;

      piece.dimensions.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-x")->valueint;
      piece.dimensions.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "dimension-y")->valueint;

      piece.origin.x =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-x")->valueint;
      piece.origin.y =
        cJSON_GetObjectItemCaseSensitive(pieceJson, "origin-y")->valueint;

      {
        auto order =
          cJSON_GetObjectItemCaseSensitive(pieceJson, "render-order")->valueint;
        if (order < 0u || order >= 100u) {
          spdlog::error(
            "render-order for '{}' of '{}' is OOB ({}); "
            "range must be from 0 to 100"
          , pieceLabel, animator->label, order
          );
          order = 99u;
        }
        piece.renderOrder = order;
      }

      cJSON * stateJson;
      cJSON_ArrayForEach(
        stateJson, cJSON_GetObjectItemCaseSensitive(pieceJson, "states")
      ) {
        std::string stateLabel =
          cJSON_GetObjectItemCaseSensitive(stateJson, "label")->valuestring;

        pulcher::animation::Animator::State state;

        state.msDeltaTime =
          cJSON_GetObjectItemCaseSensitive(
            stateJson, "ms-delta-time"
          )->valueint;

        state.rotationMirrored =
          cJSON_GetObjectItemCaseSensitive(stateJson, "rotation-mirrored")
            ->valueint
        ;

        state.rotatePixels =
          cJSON_GetObjectItemCaseSensitive(stateJson, "rotate-pixels")
            ->valueint
        ;

        cJSON * componentPartJson;
        cJSON_ArrayForEach(
          componentPartJson
        , cJSON_GetObjectItemCaseSensitive(stateJson, "components")
        ) {
          pulcher::animation::Animator::ComponentPart componentPart;
          componentPart.rangeMax =
            cJSON_GetObjectItemCaseSensitive(
              componentPartJson, "angle-range-max"
            )->valuedouble
          ;

          componentPart.data[0] =
            ::JsonLoadComponents(
              cJSON_GetObjectItemCaseSensitive(componentPartJson, "default")
            );

          componentPart.data[1] =
            ::JsonLoadComponents(
              cJSON_GetObjectItemCaseSensitive(componentPartJson, "flipped")
            );

          state.components.emplace_back(std::move(componentPart));
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
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {

  auto & animationSystem = scene.AnimationSystem();

  { // load animations
    cJSON * spritesheetDataJson =
      ::LoadJsonFile("assets/base/spritesheets/data.json");

    spdlog::debug("Loading animations");

    cJSON * filenameJson;
    cJSON_ArrayForEach(
      filenameJson
    , cJSON_GetObjectItemCaseSensitive(spritesheetDataJson, "files")
    ) {
      spdlog::debug("Loading '{}'", filenameJson->valuestring);
      ::LoadAnimation(
        std::string{filenameJson->valuestring}
      , animationSystem.animators
      );
    }

    cJSON_Delete(spritesheetDataJson);
  }

  for (auto const & anim : animationSystem.animators) {
    spdlog::debug("successfully loaded anim '{}'/'{}'", anim.first, anim.second->label);
  }

  { // -- sokol animation program
    sg_shader_desc desc = {};
    desc.vs.uniform_blocks[0].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[0].uniforms[0].name = "originOffset";
    desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.vs.uniform_blocks[1].size = sizeof(float) * 2;
    desc.vs.uniform_blocks[1].uniforms[0].name = "framebufferResolution";
    desc.vs.uniform_blocks[1].uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;

    desc.fs.images[0].name = "baseSampler";
    desc.fs.images[0].type = SG_IMAGETYPE_2D;

    desc.vs.source = PUL_SHADER(
      layout(location = 0) in vec3 inOrigin;
      layout(location = 1) in vec2 inUvCoord;

      out vec2 uvCoord;

      uniform vec2 originOffset;
      uniform vec2 framebufferResolution;

      void main() {
        vec2 framebufferScale = vec2(2.0f) / framebufferResolution;
        vec2 vertexOrigin = (inOrigin.xy)*vec2(1,-1) * framebufferScale;
        vertexOrigin += originOffset*vec2(-1, 1) * framebufferScale;
        gl_Position = vec4(vertexOrigin, 0.5001f + inOrigin.z/100000.0f, 1.0f);
        uvCoord = vec2(inUvCoord.x, 1.0f-inUvCoord.y);
      }
    );

    desc.fs.source = PUL_SHADER(
      uniform sampler2D baseSampler;
      in vec2 uvCoord;
      out vec4 outColor;
      void main() {
        outColor = texture(baseSampler, uvCoord);
        if (outColor.a < 0.1f) { discard; }
      }
    );

    animationSystem.sgProgram = sg_make_shader(&desc);
  }

  { // -- sokol pipeline
    sg_pipeline_desc desc = {};

    desc.layout.buffers[0].stride = 0u;
    desc.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[0].step_rate = 6u;
    desc.layout.attrs[0].buffer_index = 0;
    desc.layout.attrs[0].offset = 0;
    desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;

    desc.layout.buffers[1].stride = 0u;
    desc.layout.buffers[1].step_func = SG_VERTEXSTEP_PER_VERTEX;
    desc.layout.buffers[1].step_rate = 1u;
    desc.layout.attrs[1].buffer_index = 1;
    desc.layout.attrs[1].offset = 0;
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

PUL_PLUGIN_DECL void Animation_Shutdown(pulcher::core::SceneBundle & scene) {
  auto & registry = scene.EnttRegistry();

  { // -- delete sokol animation information
    auto view = registry.view<pulcher::animation::ComponentInstance>();

    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      Animation_DestroyInstance(self.instance);
    }
  }

  sg_destroy_shader(scene.AnimationSystem().sgProgram);
  sg_destroy_pipeline(scene.AnimationSystem().sgPipeline);

  scene.AnimationSystem() = {};
}

PUL_PLUGIN_DECL void Animation_UpdateFrame(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  // update each component
  auto view = registry.view<pulcher::animation::ComponentInstance>();
  for (auto entity : view) {
    auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

    if (!self.instance.hasCalculatedCachedInfo) {
      ::ComputeCache(
        self.instance, self.instance.animator->skeleton
      , glm::mat3(1.0f), false, 0.0f
      );
    }

    ::ComputeVertices(self.instance, true);
  }
}

PUL_PLUGIN_DECL void Animation_RenderAnimations(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  auto & registry = scene.EnttRegistry();
  { // -- render sokol animations

    // bind pipeline & global uniforms
    sg_apply_pipeline(scene.AnimationSystem().sgPipeline);

    std::array<float, 2> windowResolution {{
      static_cast<float>(pulcher::gfx::DisplayWidth())
    , static_cast<float>(pulcher::gfx::DisplayHeight())
    }};

    sg_apply_uniforms(
      SG_SHADERSTAGE_VS
    , 1
    , windowResolution.data()
    , sizeof(float) * 2ul
    );

    // render each component
    auto view = registry.view<pulcher::animation::ComponentInstance>();
    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      self.instance.hasCalculatedCachedInfo = false;

      if (self.instance.drawCallCount == 0ul) { continue; }

      sg_apply_bindings(self.instance.sgBindings);

      sg_apply_uniforms(
        SG_SHADERSTAGE_VS
      , 0
      , &self.instance.origin.x
      , sizeof(float) * 2ul
      );

      // must update entire animation buffer
      sg_update_buffer(
        self.instance.sgBufferUvCoord,
        self.instance.uvCoordBufferData.data(),
        self.instance.uvCoordBufferData.size() * sizeof(glm::vec2)
      );
      sg_update_buffer(
        self.instance.sgBufferOrigin,
        self.instance.originBufferData.data(),
        self.instance.originBufferData.size() * sizeof(glm::vec3)
      );

      sg_draw(0, self.instance.drawCallCount, 1);
    }
  }
}

PUL_PLUGIN_DECL void Animation_UpdateCache(
  pulcher::animation::Instance & instance
) {
  ::ComputeCache(
    instance, instance.animator->skeleton, glm::mat3(1.0f), false, 0.0f
  );
  instance.hasCalculatedCachedInfo = true;
}

PUL_PLUGIN_DECL void Animation_UpdateCacheWithPrecalculatedMatrix(
  pulcher::animation::Instance & instance
, glm::mat3 const & skeletalMatrix
) {
  ::ComputeCache(
    instance, instance.animator->skeleton, skeletalMatrix, false, 0.0f
  );
  instance.hasCalculatedCachedInfo = true;
}

PUL_PLUGIN_DECL void Animation_UiRender(
  pulcher::plugin::Info const &, pulcher::core::SceneBundle & scene
) {
  ImGui::Begin("Animation");

  auto & registry = scene.EnttRegistry();

  static pulcher::animation::Animator * editAnimator = nullptr;

  { // -- display spritesheet info
    ImGui::Text("spritesheets");

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
    auto view = registry.view<pulcher::animation::ComponentInstance>();

    ImGui::Text("instances");

    ImGui::Separator();
    ImGui::Separator();

    for (auto entity : view) {
      auto & self = view.get<pulcher::animation::ComponentInstance>(entity);

      ImGui::PushID(&self);

      PUL_ASSERT(self.instance.animator, continue;);

      if (ImGui::TreeNode(self.instance.animator->label.c_str())) {
        for (auto const & stateInfoPair : self.instance.pieceToState) {
          auto & stateInfo = stateInfoPair.second;

          pul::imgui::Text("part - '{}'", stateInfoPair.first);
          pul::imgui::Text("\tdelta-time {}", stateInfo.deltaTime);
          pul::imgui::Text("\tcomponent-it {}", stateInfo.componentIt);
          pul::imgui::Text("\tflip {}", stateInfo.flip);
          pul::imgui::Text("\tangle {}", stateInfo.angle);
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
        animMsTimer += pulcher::util::msPerFrame;

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

    if (ImGui::Button("save")) {
      ::SaveAnimations(scene.AnimationSystem());
    }

    ImGui::Separator();

    pul::imgui::Text("animator '{}'", animator.label);

    ImGui::Separator();

    ImGui::Text("pieces");
    ImGui::Separator();
    ImGui::Separator();

    size_t pieceIt = 0ul;
    for (auto & piecePair : animator.pieces) {
      ImGui::Separator();

      auto & piece = piecePair.second;

      ImGui::PushID(++ pieceIt);

      if (ImGui::TreeNode(piecePair.first.c_str())) {

        pul::imgui::InputInt2("dimensions", &piece.dimensions.x);
        pul::imgui::DragInt2("origin", &piece.origin.x, 0.01f);
        pul::imgui::DragInt("render order", &piece.renderOrder, 0.01f);

        for (auto & statePair : piece.states) {
          ImGui::Separator();

          if (!ImGui::TreeNode(statePair.first.c_str())) {
            continue;
          }

          auto & state = statePair.second;
          (void)state;

          ImGui::SliderFloat(
            "delta time", &statePair.second.msDeltaTime, 0.0f, 1000.0f
          );

          auto & componentParts = statePair.second.components;

          /* if (components.size() > 0ul) */
          /* { // display image */
          /*   size_t componentIt = */
          /*     static_cast<size_t>(animMsTimer / statePair.second.msDeltaTime); */
          /*   componentIt %= components.size(); */

          /*   // render w/ previous sprite alpha if requested */

          /*   ImVec2 pos = ImGui::GetCursorScreenPos(); */
          /*   if (animShowPreviousSprite > 0.0f) { */
          /*     ::ImGuiRenderSpritesheetTile( */
          /*       instance, piece */
          /*     , components[ */
          /*         (components.size() + componentIt - 1) % components.size() */
          /*       ] */
          /*     , animShowPreviousSprite */
          /*     ); */
          /*   } */
          /*   ImGui::SetCursorScreenPos(pos); */
          /*   ::ImGuiRenderSpritesheetTile( */
          /*     instance, piece, components[componentIt] */
          /*   ); */

          /*   ImGui::Separator(); */
          /* } */

          /* if (ImGui::Button("Set animation timer")) { */
          /*   animMaxTime = */
          /*     static_cast<size_t>( */
          /*       0.5f + components.size() * statePair.second.msDeltaTime */
          /*     ); */
          /* } */

/*           // insert at 0, important for when component is size 0 also */
/*           if (ImGui::Button("+")) { */
/*             components.emplace(components.begin()); */
/*           } */

          bool angleRangeChanged = false;

          ImGui::Checkbox("rotation mirrored", &state.rotationMirrored);
          ImGui::SameLine();
          ImGui::Checkbox("rotate pixels", &state.rotatePixels);

          if (ImGui::Button("add range part")) {
            componentParts.emplace_back();
            angleRangeChanged = true;
          }

          for (
            size_t componentPartIt = 0ul;
            componentPartIt < componentParts.size();
            ++ componentPartIt
          ) {

            ImGui::PushID(&componentParts[componentPartIt]);

            auto & componentPart = componentParts[componentPartIt];

            ImGui::Separator();

            bool const componentTreeNode = 
              ImGui::TreeNode(
                fmt::format("{}", componentPartIt).c_str()
              , "%s"
              , fmt::format(
                  "[{:.2f}]"
                , componentPart.rangeMax * 360.0f / pul::Tau
                ).c_str()
              );

            if (!componentTreeNode) {
              ImGui::PopID();
              continue;
            }

            { // draw angle
              ImDrawList * drawList = ImGui::GetWindowDrawList();

              ImVec2 pos = ImGui::GetCursorScreenPos();
              pos.x += 96;

              ImVec4 col = ImVec4(1, 1, 1, 1);

              drawList->AddCircle(pos, 16.0f, ImColor(col), 20, 1.0f);

              float const theta = componentPart.rangeMax + pul::Pi*0.5f;

              // render angles for both sides
              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x + 16.0f * glm::cos(theta)
                , pos.y + 16.0f - 32.0f * (componentPart.rangeMax / pul::Pi)
                )
              , ImColor(ImVec4(1.0f, 0.7f, 0.7f, 1.0f))
              , 2.0f
              );

              drawList->AddLine(
                pos
              , ImVec2(
                  pos.x - 16.0f * glm::cos(theta)
                , pos.y + 16.0f - 32.0f * (componentPart.rangeMax / pul::Pi)
                )
              , ImColor(ImVec4(1.0f, 0.7f, 0.7f, 1.0f))
              , 2.0f
              );

              // render angles for both sides for previous
              float const prevRange =
                  componentPartIt == 0ul
                ? 0.0f : componentParts[componentPartIt-1].rangeMax
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
              componentParts.erase(componentParts.begin()+componentPartIt);
              -- componentPartIt;
              ImGui::PopID();
            }

            ImGui::DragFloat(
              "angle range", &componentPart.rangeMax
            , 0.0025f, 0.0f, pul::Pi, "%.2f"
            );

            angleRangeChanged |= ImGui::IsItemActive();

            ImGui::Separator();

            auto & componentsDefault = componentPart.data[0];
            auto & componentsFlipped = componentPart.data[1];

            if (
                componentsDefault.size() == 0ul
             && ImGui::Button("add default")
            ) {
              componentsDefault.emplace_back();
            }

            for (
              size_t componentIt = 0ul;
              componentIt < componentsDefault.size();
              ++ componentIt
            ) {
              DisplayImGuiComponent(
                animator, piece, componentsDefault, componentIt
              );
            }

            if (componentsFlipped.size() > 0ul) {

              ImGui::Text("flipped state");
              ImGui::Separator();

              ImGui::PushID(1);

              for (
                size_t componentIt = 0ul;
                componentIt < componentsFlipped.size();
                ++ componentIt
              ) {
                DisplayImGuiComponent(
                  animator, piece, componentsFlipped, componentIt
                );
              }

              ImGui::PopID();
            } else {
              if (ImGui::Button("add flipped")) {
                componentsFlipped.emplace_back();
              }
            }

            ImGui::PopID();
            ImGui::TreePop();
          }

          // can only sort when user is not modifying anymore
          if (!angleRangeChanged) {
            std::sort(
              componentParts.begin(), componentParts.end()
            , [](auto & partA, auto& partB) {
                return partA.rangeMax < partB.rangeMax;
              }
            );
          }

          ImGui::TreePop(); // state
        }

        ImGui::TreePop(); // piece
      }

      ImGui::PopID(); // pieceIt
    }

    ImGui::End();
  }
}

} // -- extern C
