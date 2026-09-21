#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "Color.h"
#include "Math.h"

class Texture {
 public:
  Texture(int width, int height, Color fill_color = {0.0, 0.0, 0.0})
      : width_(width), height_(height), texels_(width * height, fill_color) {
    assert(width > 0);
    assert(height > 0);
  }

  [[nodiscard]] int Width() const { return width_; }

  [[nodiscard]] int Height() const { return height_; }

  Color& TexelAt(int x, int y) {
    assert(x >= 0 && x < width_);
    assert(y >= 0 && y < height_);
    return texels_[y * width_ + x];
  }

  const Color& TexelAt(int x, int y) const {
    assert(x >= 0 && x < width_);
    assert(y >= 0 && y < height_);
    return texels_[y * width_ + x];
  }

  [[nodiscard]] Color SampleNearest(Vec2 uv) const {
    double u = std::clamp(uv.x, 0.0, 1.0);
    double v = std::clamp(uv.y, 0.0, 1.0);

    int x = static_cast<int>(std::round(u * static_cast<double>(Width() - 1)));

    int y = static_cast<int>(std::round(v * static_cast<double>(Height() - 1)));

    return TexelAt(x, y);
  }

  [[nodiscard]] Color SampleBilinear(Vec2 uv) const {
    double u = std::clamp(uv.x, 0.0, 1.0);
    double v = std::clamp(uv.y, 0.0, 1.0);

    int x0 = static_cast<int>(std::floor(u * static_cast<double>(Width() - 1)));
    int y0 = static_cast<int>(std::floor(v * static_cast<double>(Width() - 1)));

    int x1 = x0 + 1;
    int y1 = y0 + 1;

    return (TexelAt(x0, y0) + TexelAt(x0, y1) + TexelAt(x1, y0) +
            TexelAt(x1, y1)) /
           4.0;  // NOLINT
  }

 private:
  int width_;
  int height_;
  std::vector<Color> texels_;
};