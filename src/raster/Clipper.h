#pragma once

#include <vector>

#include "core/Color.h"
#include "core/Vec.h"

namespace raster {

struct ClipVertex {
  Vec4 clip_pos;
  Color color;
  Vec2 uv;
};

struct ClipTriangle {
  ClipVertex a;
  ClipVertex b;
  ClipVertex c;
  Color color;
};

struct ClipPlane {
  Vec4 coeff;

  double operator()(const ClipVertex& vertex) const;
};

void ClipTriangles(const std::vector<ClipTriangle>& clip_triangles,
                   std::vector<ClipTriangle>& clipped_clip_triangles);

}  // namespace raster
