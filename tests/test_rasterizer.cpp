#include "Check.h"
#include "core/Color.h"
#include "raster/Framebuffer.h"
#include "raster/Rasterizer.h"

using namespace raster;  // NOLINT

namespace {

const double cEpsilon = 1e-12;
const int cBufferSize = 16;

ScreenVertex Vertex(double x, double y) {
  return ScreenVertex{.pos = {x, y},
                      .depth = 0.0,
                      .color = Colors::White,
                      .uv = {},
                      .inv_w = 1};
}

}  // namespace

int main() {
  const ScreenTriangle cTriangle{Vertex(0, 0), Vertex(8, 0), Vertex(0, 8),
                                 Colors::White};

  const BarycentricCoordinates cAtA =
      GetBarycentricCoordinates(cTriangle, {0, 0});
  CHECK_NEAR(cAtA.l1, 1.0, cEpsilon);
  CHECK_NEAR(cAtA.l2, 0.0, cEpsilon);
  CHECK_NEAR(cAtA.l3, 0.0, cEpsilon);

  const BarycentricCoordinates cAtB =
      GetBarycentricCoordinates(cTriangle, {8, 0});
  CHECK_NEAR(cAtB.l2, 1.0, cEpsilon);

  const BarycentricCoordinates cAtC =
      GetBarycentricCoordinates(cTriangle, {0, 8});
  CHECK_NEAR(cAtC.l3, 1.0, cEpsilon);

  const double cThird = 1.0 / 3.0;
  const BarycentricCoordinates cCentroid =
      GetBarycentricCoordinates(cTriangle, {8.0 / 3.0, 8.0 / 3.0});
  CHECK_NEAR(cCentroid.l1, cThird, cEpsilon);
  CHECK_NEAR(cCentroid.l2, cThird, cEpsilon);
  CHECK_NEAR(cCentroid.l3, cThird, cEpsilon);

  CHECK(IsInside(cCentroid));
  CHECK(!IsInside(GetBarycentricCoordinates(cTriangle, {5, 5})));

  CHECK(Orientation({0, 0}, {1, 0}, {0, 1}) > 0);
  CHECK(!IsFrontFacing(cTriangle));

  Framebuffer buffer(cBufferSize, cBufferSize, Colors::Black);

  const BoundingBox cBox = FindBoundingBox(cTriangle, buffer);
  CHECK(cBox.min_x == 0);
  CHECK(cBox.max_x == 8);
  CHECK(cBox.min_y == 0);
  CHECK(cBox.max_y == 8);

  const ScreenTriangle cBig{Vertex(-3, -4), Vertex(20, 5), Vertex(4, 30),
                            Colors::White};
  const BoundingBox cClamped = FindBoundingBox(cBig, buffer);
  CHECK(cClamped.min_x == 0);
  CHECK(cClamped.min_y == 0);
  CHECK(cClamped.max_x == cBufferSize);
  CHECK(cClamped.max_y == cBufferSize);

  RasterizeTriangle(buffer, cTriangle);
  CHECK(buffer.At(2, 2) == Colors::White);
  CHECK(buffer.At(7, 7) == Colors::Black);

  return Summary("rasterizer");
}
