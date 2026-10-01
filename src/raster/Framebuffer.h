#pragma once

#include <vector>

#include "core/Color.h"

namespace raster {

class Framebuffer {
 public:
  Framebuffer(int width, int height, Color clear_color);

  [[nodiscard]] int Width() const;
  [[nodiscard]] int Height() const;

  const Color& At(int x, int y) const;
  Color& At(int x, int y);

  double DepthAt(int x, int y) const;
  double& DepthAt(int x, int y);

  bool PassDepthTest(double depth, int x, int y);

 private:
  int width_;
  int height_;
  std::vector<Color> pixels_;
  std::vector<double> depth_;
};

}  // namespace raster
