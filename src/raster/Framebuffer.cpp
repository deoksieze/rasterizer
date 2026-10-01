#include "raster/Framebuffer.h"

#include <limits>

namespace raster {

Framebuffer::Framebuffer(int width, int height, Color clear_color)
    : width_(width),
      height_(height),
      pixels_(width * height, clear_color),
      depth_(width * height, std::numeric_limits<double>::infinity()) {}

int Framebuffer::Width() const { return width_; }

int Framebuffer::Height() const { return height_; }

const Color& Framebuffer::At(int x, int y) const {
  return pixels_[y * width_ + x];
}

Color& Framebuffer::At(int x, int y) { return pixels_[y * width_ + x]; }

double Framebuffer::DepthAt(int x, int y) const {
  return depth_[y * width_ + x];
}

double& Framebuffer::DepthAt(int x, int y) { return depth_[y * width_ + x]; }

bool Framebuffer::PassDepthTest(double depth, int x, int y) {
  return depth < this->DepthAt(x, y);
}

}  // namespace raster
