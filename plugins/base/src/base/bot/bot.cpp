#include <plugin-base/bot/bot.hpp>

#include <micropather/stlastar.hpp>

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/map.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/log.hpp>

namespace {

struct NavMapSearchNode {
  glm::i32vec2 origin = glm::i32vec2(0);

  float GoalDistanceEstimate(NavMapSearchNode & nodeGoal);
  bool IsGoal(NavMapSearchNode & nodeGoal);
  bool GetSuccessors(
    AStarSearch<NavMapSearchNode> * astar,
    NavMapSearchNode * parent
  );
  float GetCost(NavMapSearchNode & successor);
  bool IsSameState(NavMapSearchNode & rhs);

  void PrintNodeInfo();
};

bool NavMapSearchNode::IsSameState(NavMapSearchNode & rhs) {
  return origin == rhs.origin;
}

float NavMapSearchNode::GoalDistanceEstimate(NavMapSearchNode & nodeGoal) {
  return glm::abs(glm::length(glm::vec2(origin - nodeGoal.origin)));
}

bool NavMapSearchNode::IsGoal(NavMapSearchNode & /*nodeGoal*/) {
  return true;
}

bool NavMapSearchNode::GetSuccessors(
  AStarSearch<NavMapSearchNode> * /*astar*/,
  NavMapSearchNode * /*parent*/
) {
  return true;
}

float NavMapSearchNode::GetCost(NavMapSearchNode & /*successor*/) {
  return 1.0f;
}

void NavMapSearchNode::PrintNodeInfo() {
  spdlog::info("Node: {}", origin);
}


struct NavMesh {
};

NavMesh navMesh;

}

void plugin::bot::BuildNavigationMap(
  std::vector<pul::physics::Tileset const *> const & tilesets
, std::vector<std::span<size_t>>             const & mapTileIndices
, std::vector<std::span<glm::u32vec2>>       const & mapTileOrigins
, std::vector<
    std::span<pul::core::TileOrientation>
  > const & mapTileOrientations
) {
  (void) tilesets;
  (void) mapTileIndices;
  (void) mapTileOrigins;
  (void) mapTileOrientations;

  /* navMesh.pather = */
  /*   std::make_unique<micropather::Micropather>( */

}

void plugin::bot::ApplyInput(
  pul::plugin::Info const &, pul::core::SceneBundle &
, pul::controls::Controller & controls
, pul::core::ComponentPlayer const &
, glm::vec2 const &
) {
  controls.current.jump = true;
}
