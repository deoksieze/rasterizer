#pragma once

#include <vector>

#include "core/Color.h"
#include "core/Matrix.h"
#include "core/Vec.h"
#include "geometry/Mesh.h"
#include "raster/Clipper.h"
#include "raster/Framebuffer.h"

namespace raster {

struct ScreenVertex {
  Vec2 pos;
  double depth;
  Color color;
  Vec2 uv;
  double inv_w;
};

struct ScreenTriangle {
  ScreenVertex a;
  ScreenVertex b;
  ScreenVertex c;
  Color color;
};

void TransformMeshToClipTriangles(const Mesh& mesh, const Mat4& model,
                                  const Mat4& view, const Mat4& projection,
                                  std::vector<ClipTriangle>& clip_triangles);

void ProjectClippedTrianglesToScreen(
    const std::vector<ClipTriangle>& clipped_triangles,
    std::vector<ScreenTriangle>& triangles, const Framebuffer& buff);

}  // namespace raster
