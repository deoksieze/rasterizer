#include "raster/Rasterizer.h"

#include <algorithm>
#include <cmath>

namespace raster {
namespace {

const double cPixCentOffset = 0.5;

}  // namespace

double Orientation(const Vec2& a, const Vec2& b, const Vec2& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool IsFrontFacing(const ScreenTriangle& tr) {
  return Orientation(tr.a.pos, tr.b.pos, tr.c.pos) < 0;
}

bool IsInside(const BarycentricCoordinates& bc) {
  return bc.l1 >= 0.0 && bc.l2 >= 0.0 && bc.l3 >= 0.0;
}

BarycentricCoordinates GetBarycentricCoordinates(const ScreenTriangle& tr,
                                                 const Vec2& p) {
  const double cArea2 = Orientation(tr.a.pos, tr.b.pos, tr.c.pos);

  return {
      Orientation(tr.b.pos, tr.c.pos, p) / cArea2,
      Orientation(tr.c.pos, tr.a.pos, p) / cArea2,
      Orientation(tr.a.pos, tr.b.pos, p) / cArea2,
  };
}

BoundingBox FindBoundingBox(const ScreenTriangle& tr,
                            const Framebuffer& buffer) {
  Vec2 a = tr.a.pos;
  Vec2 b = tr.b.pos;
  Vec2 c = tr.c.pos;

  const int cMinX =
      std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));

  const int cMaxX = std::min(
      buffer.Width(), static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));

  const int cMinY =
      std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));

  const int cMaxY = std::min(
      buffer.Height(), static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

  return BoundingBox{cMinX, cMaxX, cMinY, cMaxY};
}

void RasterizeTriangle(Framebuffer& buffer, const ScreenTriangle& tr) {
  const BoundingBox cBox = FindBoundingBox(tr, buffer);

  if (cBox.max_x <= cBox.min_x || cBox.max_y <= cBox.min_y) {
    return;
  }

  for (int y = cBox.min_y; y < cBox.max_y; y++) {
    for (int x = cBox.min_x; x < cBox.max_x; x++) {
      Vec2 p = {static_cast<double>(x) + cPixCentOffset,
                static_cast<double>(y) + cPixCentOffset};

      const BarycentricCoordinates cBc = GetBarycentricCoordinates(tr, p);

      if (IsInside(cBc)) {
        const double cDepth =
            cBc.l1 * tr.a.depth + cBc.l2 * tr.b.depth + cBc.l3 * tr.c.depth;

        if (buffer.PassDepthTest(cDepth, x, y)) {
          buffer.DepthAt(x, y) = cDepth;

          // double q = cBc.l1 * tr.a.inv_w + cBc.l2 * tr.b.inv_w +
          //            cBc.l3 * tr.c.inv_w;
          // Vec2 uv_over_w = cBc.l1 * tr.a.uv + cBc.l2 * tr.b.uv +
          //                  cBc.l3 * tr.c.uv;
          //
          // Vec2 uv = uv_over_w / q;
          //
          // const Color color = texture.SampleNearest(uv);
          const Color color = tr.color;  // NOLINT
          buffer.At(x, y) = color;
        }
      }
    }
  }
}

void RasterizeTriangles(Framebuffer& buffer,
                        const std::vector<ScreenTriangle>& triangles,
                        bool cull_back_faces) {
  for (const auto& tr : triangles) {
    if (cull_back_faces && !IsFrontFacing(tr)) {
      continue;
    }

    RasterizeTriangle(buffer, tr);
  }
}

}  // namespace raster
