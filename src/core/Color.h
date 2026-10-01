#pragma once

#include <cstdint>

namespace raster {

struct Color {
  double r;
  double g;
  double b;

  Color& operator+=(const Color& other);
  Color& operator*=(double x);
  constexpr bool operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b;
  }
};

Color operator*(Color c, double x);
Color operator*(double x, Color color);
Color operator+(Color c1, Color c2);
Color operator-(const Color& lhs, const Color& rhs);
Color operator/(const Color& color, double x);
Color Lerp(const Color& from, const Color& to, double t);

std::uint8_t ToByte(double value);

// NOLINTBEGIN(readability-identifier-naming)

namespace Colors {
inline constexpr Color Black{0.0, 0.0, 0.0};
inline constexpr Color White{1.0, 1.0, 1.0};
inline constexpr Color Red{1.0, 0.0, 0.0};
inline constexpr Color Green{0.0, 1.0, 0.0};
inline constexpr Color Blue{0.0, 0.0, 1.0};
inline constexpr Color Gray{0.5, 0.5, 0.5};
inline constexpr Color Magenta{1.0, 0.0, 1.0};
inline constexpr Color Cyan{0.0, 1.0, 1.0};
inline constexpr Color Yellow{1.0, 1.0, 0.0};
}  // namespace Colors

// NOLINTEND(readability-identifier-naming)

}  // namespace raster
