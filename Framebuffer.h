#ifndef FRAMEBUFFER
#define FRAMEBUFFER

#include <vector>
struct Color {
  double r;
  double g;
  double b;

  Color& operator+=(const Color& other) {
    r += other.r;
    g += other.g;
    b += other.b;
    return *this;
  }

  Color& operator*=(double x) {
    r *= x;
    g *= x;
    b *= x;
    return *this;
  }
};

Color operator*(Color c, double x) { return Color{c.r * x, c.g * x, c.b * x}; }

Color operator*(double x, Color color) { return color * x; }

Color operator+(Color c1, Color c2) {
  return Color{c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}

class Framebuffer {
 public:
  Framebuffer(int width, int height, Color clear_color)
      : width_(width), height_(height), pixels_(width * height, clear_color) {}

  const Color& At(int x, int y) const { return pixels_[y * width_ + x]; }

  Color& At(int x, int y) { return pixels_[y * width_ + x]; }

  int Width() const { return width_; }

  int Height() const { return height_; }

 private:
  int width_;
  int height_;
  std::vector<Color> pixels_;
};

#endif