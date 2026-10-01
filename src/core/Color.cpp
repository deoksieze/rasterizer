#include "Color.h"

#include <algorithm>
#include <cmath>

namespace raster {
namespace {

const double cMaxColor = 255.0;

}  // namespace

Color& Color::operator+=(const Color& other) {
  r += other.r;
  g += other.g;
  b += other.b;
  return *this;
}

Color& Color::operator*=(double x) {
  r *= x;
  g *= x;
  b *= x;
  return *this;
}

Color operator*(Color c, double x) { return Color{c.r * x, c.g * x, c.b * x}; }

Color operator*(double x, Color color) { return color * x; }

Color operator+(Color c1, Color c2) {
  return Color{c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}

Color operator-(const Color& lhs, const Color& rhs) {
  return {
      lhs.r - rhs.r,
      lhs.g - rhs.g,
      lhs.b - rhs.b,
  };
}

Color operator/(const Color& color, double x) {
  return Color{color.r / x, color.g / x, color.b / x};
}

Color Lerp(const Color& from, const Color& to, double t) {
  return from + t * (to - from);
}

std::uint8_t ToByte(double value) {
  value = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(std::lround(value * cMaxColor));
}

}  // namespace raster
