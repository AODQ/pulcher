#include <pulcher-physics/intersections.hpp>

#include <pulcher-util/enum.hpp>
#include <pulcher-util/log.hpp>

pulcher::physics::IntersectorRay pulcher::physics::IntersectorRay::Construct(
  glm::vec2 const beginOrigin, glm::vec2 const endOrigin
) {
  pulcher::physics::IntersectorRay ray;
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

void pulcher::physics::DebugQueries::Add(
  pulcher::physics::IntersectorPoint const & intersector
, pulcher::physics::IntersectionResults results
) {
  intersectorPoints.emplace_back(intersector, results);
}

void pulcher::physics::DebugQueries::Add(
  pulcher::physics::IntersectorRay const & intersector
, pulcher::physics::IntersectionResults results
) {
  intersectorRays.emplace_back(intersector, results);
}
