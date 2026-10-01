#include <vector>

#include "Check.h"
#include "core/Color.h"
#include "raster/Clipper.h"

using namespace raster;  // NOLINT

namespace {

const double cEpsilon = 1e-9;

ClipVertex Vertex(double x, double y, double z, double w) {
  return ClipVertex{
      .clip_pos = {x, y, z, w}, .color = Colors::White, .uv = {0.0, 0.0}};
}

ClipTriangle Triangle(const ClipVertex& a, const ClipVertex& b,
                      const ClipVertex& c) {
  return ClipTriangle{.a = a, .b = b, .c = c, .color = Colors::White};
}

void CheckInFrontOfNearPlane(const std::vector<ClipTriangle>& triangles) {
  for (const ClipTriangle& triangle : triangles) {
    CHECK(triangle.a.clip_pos.z + triangle.a.clip_pos.w >= -cEpsilon);
    CHECK(triangle.b.clip_pos.z + triangle.b.clip_pos.w >= -cEpsilon);
    CHECK(triangle.c.clip_pos.z + triangle.c.clip_pos.w >= -cEpsilon);
  }
}

}  // namespace

int main() {
  {
    const std::vector<ClipTriangle> cInput = {Triangle(
        Vertex(0, 0, 0, 1), Vertex(0.5, 0, 0, 1), Vertex(0, 0.5, 0, 1))};
    std::vector<ClipTriangle> output;
    ClipTriangles(cInput, output);

    CHECK(output.size() == 1);
  }

  {
    const std::vector<ClipTriangle> cInput = {Triangle(
        Vertex(0, 0, -2, 1), Vertex(0.5, 0, 0, 1), Vertex(0, 0.5, 0, 1))};
    std::vector<ClipTriangle> output;
    ClipTriangles(cInput, output);

    CHECK(output.size() == 2);
    CheckInFrontOfNearPlane(output);
  }

  {
    const std::vector<ClipTriangle> cInput = {Triangle(
        Vertex(0, 0, -2, 1), Vertex(0.5, 0, -3, 1), Vertex(0, 0.5, -4, 1))};
    std::vector<ClipTriangle> output;
    ClipTriangles(cInput, output);

    CHECK(output.empty());
  }

  {
    const std::vector<ClipTriangle> cInput = {Triangle(
        Vertex(-2, 0, 0, 1), Vertex(-3, 0, 0, 1), Vertex(-2, 0.5, 0, 1))};
    std::vector<ClipTriangle> output;
    ClipTriangles(cInput, output);

    CHECK(output.empty());
  }

  return Summary("clipper");
}
