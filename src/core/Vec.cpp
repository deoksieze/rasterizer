#include "Vec.h"

namespace raster {

Vec2 operator-(const Vec2& a, const Vec2& b) {
  return Vec2{a.x - b.x, a.y - b.y};
}

Vec2 operator+(const Vec2& a, const Vec2& b) {
  return Vec2{a.x + b.x, a.y + b.y};
}

Vec2 operator*(const Vec2& v, double scalar) {
  return Vec2{v.x * scalar, v.y * scalar};
}

Vec2 operator*(double scalar, const Vec2& v) { return v * scalar; }

Vec2 operator/(const Vec2& v, double scalar) { return v * (1 / scalar); }

Vec2 Lerp(const Vec2& from, const Vec2& to, double t) {
  return from + t * (to - from);
}

Vec4 operator-(const Vec4& a, const Vec4& b) {
  return Vec4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

Vec4 operator+(const Vec4& a, const Vec4& b) {
  return Vec4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

Vec4 operator*(const Vec4& v, double scalar) {
  return Vec4{v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
}

Vec4 operator*(double scalar, const Vec4& v) { return v * scalar; }

Vec4 Lerp(const Vec4& from, const Vec4& to, double t) {
  return from + t * (to - from);
}

}  // namespace raster
