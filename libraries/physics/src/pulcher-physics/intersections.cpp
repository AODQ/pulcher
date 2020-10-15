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
