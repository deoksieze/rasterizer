#pragma once

#include <cmath>
#include <vector>

#include "Color.h"
class Framebuffer {
 public:
  Framebuffer(int width, int height, Color clear_color)
      : width_(width),
        height_(height),
        pixels_(width * height, clear_color),
        depth_(width * height, +INFINITY) {}

  const Color& At(int x, int y) const { return pixels_[y * width_ + x]; }

  Color& At(int x, int y) { return pixels_[y * width_ + x]; }

  double DepthAt(int x, int y) const { return depth_[y * width_ + x]; }

  double& DepthAt(int x, int y) { return depth_[y * width_ + x]; }

  bool PassDepthTest(double depth, int x, int y) {
    return depth < this->DepthAt(x, y);
  }

  int Width() const { return width_; }

  int Height() const { return height_; }

 private:
  int width_;
  int height_;
  std::vector<Color> pixels_;
  std::vector<double> depth_;
};
