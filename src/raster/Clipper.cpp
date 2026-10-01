#include "raster/Clipper.h"

#include <array>
#include <cstddef>

namespace raster {
namespace {

const std::array<ClipPlane, 6> cClipPlanes = {{
    {{1.0, 0.0, 0.0, 1.0}},   // left   (x + w >= 0)
    {{-1.0, 0.0, 0.0, 1.0}},  // right  (w - x >= 0)
    {{0.0, 1.0, 0.0, 1.0}},   // bottom (y + w >= 0)
    {{0.0, -1.0, 0.0, 1.0}},  // top    (w - y >= 0)
    {{0.0, 0.0, 1.0, 1.0}},   // near   (z + w >= 0)
    {{0.0, 0.0, -1.0, 1.0}},  // far    (w - z >= 0)
}};

ClipVertex LerpClipVertex(const ClipVertex& from, const ClipVertex& to,
                          double t) {
  return {.clip_pos = Lerp(from.clip_pos, to.clip_pos, t),
          .color = Lerp(from.color, to.color, t),
          .uv = Lerp(from.uv, to.uv, t)};
}

template <typename Func>
void ShClip(Func signed_distance_to_plane,
            std::vector<ClipVertex>& clip_polygon) {
  const std::vector<ClipVertex> cInput = clip_polygon;
  clip_polygon.clear();

  if (cInput.empty()) {
    return;
  }

  ClipVertex prev = cInput.back();
  for (ClipVertex curr : cInput) {
    const auto IsInsidePlane = [&](const ClipVertex& vertex) {  // NOLINT
      return signed_distance_to_plane(vertex) >= 0.0;
    };

    const double cDPrev = signed_distance_to_plane(prev);
    const double cDCurr = signed_distance_to_plane(curr);
    bool prev_inside = IsInsidePlane(prev);
    bool curr_inside = IsInsidePlane(curr);

    if (prev_inside && curr_inside) {
      clip_polygon.push_back(curr);
    }

    else if (prev_inside && !curr_inside) {
      double t = cDPrev / (cDPrev - cDCurr);
      clip_polygon.push_back(LerpClipVertex(prev, curr, t));
    }

    else if (!prev_inside && curr_inside) {
      double t = cDPrev / (cDPrev - cDCurr);
      clip_polygon.push_back(LerpClipVertex(prev, curr, t));
      clip_polygon.push_back(curr);
    }

    prev = curr;
  }
}

}  // namespace

double ClipPlane::operator()(const ClipVertex& vertex) const {
  return vertex.clip_pos.x * coeff.x + vertex.clip_pos.y * coeff.y +
         vertex.clip_pos.z * coeff.z + vertex.clip_pos.w * coeff.w;
}

void ClipTriangles(const std::vector<ClipTriangle>& clip_triangles,
                   std::vector<ClipTriangle>& clipped_clip_triangles) {
  clipped_clip_triangles.clear();

  std::vector<ClipVertex> clip_polygon;

  for (const ClipTriangle& tr : clip_triangles) {
    clip_polygon = {tr.a, tr.b, tr.c};

    for (const ClipPlane& plane : cClipPlanes) {
      ShClip(plane, clip_polygon);
      if (clip_polygon.size() < 3) {
        break;
      }
    }

    if (clip_polygon.size() < 3) {
      continue;
    }

    if (clip_polygon.size() == 3) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[1], clip_polygon[2], tr.color});
      continue;
    }

    // Для near-plane clipping исходного triangle здесь будет ровно 4 вершины.
    // Разбиваем выпуклый polygon на triangle fan, сохраняя порядок вершин.
    for (std::size_t i = 1; i + 1 < clip_polygon.size(); ++i) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[i], clip_polygon[i + 1], tr.color});
    }
  }
}

}  // namespace raster
