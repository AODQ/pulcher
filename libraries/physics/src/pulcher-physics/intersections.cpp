#include <pulcher-physics/intersections.hpp>

#include <pulcher-core/log.hpp>
#include <pulcher-util/enum.hpp>

size_t pulcher::physics::Queries::AddQuery(
  pulcher::physics::IntersectorPoint const & intersector
) {
  intersectorPoints.emplace_back(intersector);
  return
      (intersectorPoints.size()-1)
    | Idx(pulcher::physics::IntersectorType::Point)
  ;
}

size_t pulcher::physics::Queries::AddQuery(
  pulcher::physics::IntersectorRay const & intersector
) {
  intersectorRays.emplace_back(intersector);
  return
      (intersectorRays.size()-1)
    | Idx(pulcher::physics::IntersectorType::Ray)
  ;
}

pulcher::physics::IntersectionResults
pulcher::physics::Queries::RetrieveQuery(size_t handle) {

  if (handle & Idx(pulcher::physics::IntersectorType::Point)) {
    size_t idx = handle & ~Idx(pulcher::physics::IntersectorType::Point);
    PUL_ASSERT_CMP(idx, <, intersectorResultsPoints.size(), return {};);
    return intersectorResultsPoints[idx];
  } else if (handle & Idx(pulcher::physics::IntersectorType::Ray)) {
    size_t idx = handle & ~Idx(pulcher::physics::IntersectorType::Ray);
    PUL_ASSERT_CMP(idx, <, intersectorResultsPoints.size(), return {};);
    return intersectorResultsRays[idx];
  } else {
    spdlog::critical("could not retrieve query of unknown handle '{}'", handle);
    return {};
  }
}
