#include <pulcher-physics/intersections.hpp>

#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

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
