#include <pulcher-physics/intersections.hpp>

#include <pulcher-core/log.hpp>
#include <pulcher-util/enum.hpp>

size_t pulcher::physics::Queries::AddQuery(
  pulcher::physics::IntersectorPoint const & intersector
) {
  intersectorPoints.emplace_back(intersector);
  return
    intersectorPoints.size() | Idx(pulcher::physics::IntersectorType::Point);
}

size_t pulcher::physics::Queries::AddQuery(
  pulcher::physics::IntersectorRay const & intersector
) {
  intersectorRays.emplace_back(intersector);
  return
    intersectorRays.size() | Idx(pulcher::physics::IntersectorType::Ray);
}

pulcher::physics::IntersectionResults
pulcher::physics::Queries::RetrieveQuery(size_t handle) {

  size_t idx;
  if ((idx = handle & Idx(pulcher::physics::IntersectorType::Point))) {
    if (idx >= intersectorResultsPoints.size()) {
      spdlog::critical("query idx OOB");
      return {};
    }
    return intersectorResultsPoints[idx];
  } else if ((idx = handle & Idx(pulcher::physics::IntersectorType::Ray))) {
    if (idx >= intersectorResultsRays.size()) {
      spdlog::critical("query idx OOB");
      return {};
    }
    return intersectorResultsRays[idx];
  } else {
    spdlog::critical("could not retrieve query of unknown handle '{}'", handle);
    return {};
  }
}

void pulcher::physics::Queries::Submit() {
  // clear results to allow new results to be stored to
  intersectorResultsPoints.clear();
  intersectorResultsRays.clear();

  // if the current buffer is much smaller than necessary, perform a resize
  if (
      intersectorResultsPoints.size() > 50ul
   && intersectorPoints.size() < intersectorResultsPoints.size()*2ul
  ) {
    intersectorResultsPoints.resize(intersectorPoints.size());
  }

  // reserve either way
  intersectorResultsPoints.reserve(std::max(25ul, intersectorPoints.size()));
  for (auto & result : intersectorPoints)
    { intersectorResultsPoints.emplace_back(result.outputCollision); }

  // -- now same for rays

  // if the current buffer is much smaller than necessary, perform a resize
  if (
      intersectorResultsRays.size() > 50ul
   && intersectorRays.size() < intersectorResultsRays.size()*2
  ) {
    intersectorResultsRays.resize(intersectorRays.size());
  }

  // reserve either way
  intersectorResultsRays.reserve(std::max(25ul, intersectorRays.size()));
  for (auto & result : intersectorRays) {
    intersectorResultsRays
      .emplace_back(result.outputCollision, result.origin);
  }

  // clear out current array to be written to using AddQuery
  intersectorPoints.clear();
  intersectorRays.clear();

  // TODO this is definitely not the right way to handle queries, consider if
  // every frame has >500 points/rays as example.
  if (intersectorPoints.size() > 500ul) { intersectorPoints.resize(0ul); }
  if (intersectorRays.size() > 500ul) { intersectorRays.resize(0ul); }
}


pulcher::physics::Queries & pulcher::physics::BufferedQueries::Get() {
  return queries[queryIdx];
}

pulcher::physics::Queries & pulcher::physics::BufferedQueries::GetComputing() {
  return queries[(queryIdx+1) % queries.size()];
}

void pulcher::physics::BufferedQueries::Swap() {
  queryIdx = (queryIdx+1) % queries.size();
}
