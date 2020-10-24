#include <pulcher-physics/intersections.hpp>

#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

pul::physics::IntersectorRay pul::physics::IntersectorRay::Construct(
  glm::vec2 const beginOrigin, glm::vec2 const endOrigin
) {
  pul::physics::IntersectorRay ray;
  if (beginOrigin.x < endOrigin.x) {
    ray.beginOrigin.x = glm::floor(beginOrigin.x);
    ray.endOrigin.x = glm::ceil(endOrigin.x);
  } else {
    ray.beginOrigin.x = glm::ceil(beginOrigin.x);
    ray.endOrigin.x = glm::floor(endOrigin.x);
  }

  if (beginOrigin.y < endOrigin.y) {
    ray.beginOrigin.y = glm::floor(beginOrigin.y);
    ray.endOrigin.y = glm::ceil(endOrigin.y);
  } else {
    ray.beginOrigin.y = glm::ceil(beginOrigin.y);
    ray.endOrigin.y = glm::floor(endOrigin.y);
  }

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
