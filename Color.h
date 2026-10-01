#pragma once

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

  constexpr bool operator==(const Color& other) const {
    return r == other.r && g == other.g && b == other.b;
  }
};

Color operator*(Color c, double x) { return Color{c.r * x, c.g * x, c.b * x}; }

Color operator*(double x, Color color) { return color * x; }

Color operator+(Color c1, Color c2) {
  return Color{c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}

Color operator-(Color lhs, const Color& rhs) {
  return {
      lhs.r - rhs.r,
      lhs.g - rhs.g,
      lhs.b - rhs.b,
  };
}

Color operator/(Color color, double x) {
  return Color{color.r / x, color.g / x, color.b / x};
}

Color Lerp(const Color& from, const Color& to, double t) {
  return from + t * (to - from);
}

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

//NOLINTEND(readability-identifier-naming)