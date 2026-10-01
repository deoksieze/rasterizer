#include "Texture.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace raster {

Texture::Texture(int width, int height, Color fill_color)
    : width_(width), height_(height), texels_(width * height, fill_color) {
  assert(width > 0);
  assert(height > 0);
}

int Texture::Width() const { return width_; }

int Texture::Height() const { return height_; }

Color& Texture::TexelAt(int x, int y) {
  assert(x >= 0 && x < width_);
  assert(y >= 0 && y < height_);
  return texels_[y * width_ + x];
}

const Color& Texture::TexelAt(int x, int y) const {
  assert(x >= 0 && x < width_);
  assert(y >= 0 && y < height_);
  return texels_[y * width_ + x];
}

Color Texture::SampleNearest(Vec2 uv) const {
  double u = std::clamp(uv.x, 0.0, 1.0);
  double v = std::clamp(uv.y, 0.0, 1.0);

  int x = static_cast<int>(std::round(u * static_cast<double>(Width() - 1)));

  int y = static_cast<int>(std::round(v * static_cast<double>(Height() - 1)));

  return TexelAt(x, y);
}

Color Texture::SampleBilinear(Vec2 uv) const {
  double u = std::clamp(uv.x, 0.0, 1.0);
  double v = std::clamp(uv.y, 0.0, 1.0);

  const double cX = u * static_cast<double>(Width() - 1);
  const double cY = v * static_cast<double>(Height() - 1);

  const int cX0 = static_cast<int>(std::floor(cX));
  const int cY0 = static_cast<int>(std::floor(cY));

  const int cX1 = std::min(cX0 + 1, Width() - 1);
  const int cY1 = std::min(cY0 + 1, Height() - 1);

  const double cWebX = cX - cX0;
  const double cWebY = cY - cY0;

  const Color cBottom = Lerp(TexelAt(cX0, cY0), TexelAt(cX1, cY0), cWebX);
  const Color cTop = Lerp(TexelAt(cX0, cY1), TexelAt(cX1, cY1), cWebX);

  return Lerp(cBottom, cTop, cWebY);
}

}  // namespace raster
