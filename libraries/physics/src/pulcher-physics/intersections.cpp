#include <pulcher-physics/intersections.hpp>

#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

pul::physics::IntersectorRay pul::physics::IntersectorRay::Construct(
  glm::vec2 const beginOrigin, glm::vec2 const endOrigin
) {
  pul::physics::IntersectorRay ray;
  ray.beginOrigin = glm::round(beginOrigin);
  ray.endOrigin = glm::round(endOrigin);

  return ray;
}

void pul::physics::DebugQueries::Add(
  pul::physics::IntersectorPoint const & intersector
, pul::physics::IntersectionResults results
) {
  intersectorPoints.emplace_back(intersector, results);
}

void pul::physics::DebugQueries::Add(
  pul::physics::IntersectorRay const & intersector
, pul::physics::IntersectionResults results
) {
  intersectorRays.emplace_back(intersector, results);
}
