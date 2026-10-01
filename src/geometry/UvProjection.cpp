#include "geometry/UvProjection.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace raster {

void ProjectSphericalUvs(Mesh& mesh) {
  if (mesh.vertices.empty()) {
    return;
  }

  double min_y = mesh.vertices.front().pos.y;
  double max_y = min_y;

  for (const MeshVertex& vertex : mesh.vertices) {
    min_y = std::min(min_y, vertex.pos.y);
    max_y = std::max(max_y, vertex.pos.y);
  }

  const double cHeight = max_y - min_y;

  for (MeshVertex& vertex : mesh.vertices) {
    const double cRadius =
        std::sqrt(vertex.pos.x * vertex.pos.x + vertex.pos.z * vertex.pos.z);

    const double cU = (cRadius > 0.0) ? std::atan2(vertex.pos.z, vertex.pos.x) /
                                                (2.0 * std::numbers::pi) +
                                            0.5
                                      : 0.5;

    const double cV = (cHeight > 0.0) ? (vertex.pos.y - min_y) / cHeight : 0.5;

    vertex.uv = Vec2{cU, cV};
  }
}

}  // namespace raster
