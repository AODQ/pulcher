#include <plugin-base/bot/bot.hpp>

#include <plugin-base/debug/renderer.hpp>

#include <pulcher-controls/controls.hpp>
#include <pulcher-core/map.hpp>
#include <pulcher-core/player.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <micropather/micropather.hpp>

#include <fstream>

namespace {

float CostEstimate(glm::i32vec2 const start, glm::i32vec2 const end) {
  return std::abs(end.x - start.x) + std::abs(end.y - start.y)*5.0f;
}

class GeneralAiPathFinder : public micropather::Graph {
public:

  struct NodeNeighbor {
    int32_t index;
    float cost;
  };

  struct Node {
    glm::i32vec2 origin;
    std::vector<NodeNeighbor> neighbors;
  };

  std::vector<Node> nodes;

  float LeastCostEstimate(void * stateStart, void * stateEnd) {
    auto const & start = reinterpret_cast<Node *>(stateStart)->origin;
    auto const & end   = reinterpret_cast<Node *>(stateEnd)->origin;

    // Y is much more difficult to navigate
    return CostEstimate(start, end);
  }

  void AdjacentCost(void * state, MP_VECTOR<micropather::StateCost> * adjacent)
  {
    auto const & self = *this;
    auto const & node = *reinterpret_cast<Node *>(state);

    for (auto & neighbor : node.neighbors) {
      adjacent->push_back(
        micropather::StateCost {
          .state =
            const_cast<void *>(
              reinterpret_cast<void const *>(&self.nodes[neighbor.index])
            ),
          .cost  = neighbor.cost,
        }
      );
    }
  }

  void PrintStateInfo(void * /*state*/)
  {
  }
};

GeneralAiPathFinder generalAiPathFinder;
// TODO look into using unique ptr
micropather::MicroPather * generalAiPather = nullptr;

}

void plugin::bot::BuildNavigationMap(char const * filename) {
  // reset pather information
  if (::generalAiPather) {
    delete ::generalAiPather;
  }

  ::generalAiPathFinder = {};

  ::generalAiPather =
    new micropather::MicroPather(
      micropather::MicroPather(&::generalAiPathFinder)
    );

  // build up the ai path finder
  spdlog::info("Loading ai navmap for '{}.ainav'", filename);

  cJSON * navmap;
  { // load navmap file
    auto file = std::ifstream{std::string{filename}+".ainav"};
    if (file.eof() || !file.good()) {
      spdlog::error("could not load ai navmap");
      return;
    }

    auto str =
      std::string {
        std::istreambuf_iterator<char>(file)
      , std::istreambuf_iterator<char>()
      };

    navmap = cJSON_Parse(str.c_str());

    if (navmap == nullptr) {
      spdlog::critical(
        " -- failed to parse json for ai navmap; '{}'", cJSON_GetErrorPtr()
      );
      return;
    }
  }

  cJSON * node;
  cJSON_ArrayForEach(
    node, cJSON_GetObjectItemCaseSensitive(navmap, "nodes")
  ) {
    auto const id = cJSON_GetObjectItemCaseSensitive(node, "id")->valueint;
    if (::generalAiPathFinder.nodes.size() <= static_cast<size_t>(id)) {
      ::generalAiPathFinder.nodes.resize(id+1);
    }

    auto & pathNode = ::generalAiPathFinder.nodes[id];

    pathNode.origin =
      glm::i32vec2(
        cJSON_GetObjectItemCaseSensitive(node, "origin-x")->valueint,
        cJSON_GetObjectItemCaseSensitive(node, "origin-y")->valueint
      );

    cJSON * neighbor;
    cJSON_ArrayForEach(
      neighbor, cJSON_GetObjectItemCaseSensitive(node, "neighbors")
    ) {
      pathNode.neighbors.emplace_back(
        GeneralAiPathFinder::NodeNeighbor {
          .index =
            static_cast<int32_t>(
              cJSON_GetObjectItemCaseSensitive(neighbor, "id")->valueint
            ),
          .cost = 0,
        }
      );
    }
  }

  // calculate movement costs and verify all neighbors exist
  for (auto & pathNode : ::generalAiPathFinder.nodes)
  for (auto & neighbor : pathNode.neighbors) {
    if (
        static_cast<size_t>(neighbor.index)
      >= ::generalAiPathFinder.nodes.size()
    ) {
      spdlog::error(
        "neighbor {} doesn't exist in AI navigation map", neighbor.index
      );
      continue;
    }
    auto const & start = pathNode.origin;
    auto const & end   = ::generalAiPathFinder.nodes[neighbor.index].origin;
    neighbor.cost = ::CostEstimate(start, end);
  }

  cJSON_Delete(navmap);
}

void plugin::bot::Shutdown() {
  if (::generalAiPather) {
    delete ::generalAiPather;
  }
}

void plugin::bot::DebugRender() {
  for (size_t nIt = 0ul; nIt < ::generalAiPathFinder.nodes.size(); ++ nIt) {
    auto const & node = ::generalAiPathFinder.nodes[nIt];
    plugin::debug::RenderCircle(node.origin, 4.0, glm::vec3(0.5, 0.5, 0.8));
    for (auto const & neighbor : node.neighbors) {
      // TODO lines are drawn on top of each other for bidirectional
      //      edges, which while is fine, it should be indicated as such
      //      by using an arrow or some other convention
      plugin::debug::RenderLine(
        node.origin,
        ::generalAiPathFinder.nodes[neighbor.index].origin,
        glm::vec3(0.5, 0.5, 0.9)
      );
    }
  }
}

void plugin::bot::ApplyInput(
  pul::core::SceneBundle &
, pul::controls::Controller & controls
, pul::core::ComponentPlayer const &
, glm::vec2 const &
) {
  controls.current.jump = true;
}
