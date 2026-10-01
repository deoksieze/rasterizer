#pragma once

namespace raster {

struct Vec2 {
  double x;
  double y;
};

Vec2 operator-(const Vec2& a, const Vec2& b);
Vec2 operator+(const Vec2& a, const Vec2& b);
Vec2 operator*(const Vec2& v, double scalar);
Vec2 operator*(double scalar, const Vec2& v);
Vec2 operator/(const Vec2& v, double scalar);
Vec2 Lerp(const Vec2& from, const Vec2& to, double t);

struct Vec3 {
  double x;
  double y;
  double z;
};

Vec3 operator-(const Vec3& a, const Vec3& b);
Vec3 operator+(const Vec3& a, const Vec3& b);
Vec3 operator*(const Vec3& v, double scalar);
Vec3 operator*(double scalar, const Vec3& v);
double Dot(const Vec3& a, const Vec3& b);
Vec3 Cross(const Vec3& a, const Vec3& b);
Vec3 Normalize(const Vec3& v);

struct Vec4 {
  double x;
  double y;
  double z;
  double w;
};

Vec4 operator-(const Vec4& a, const Vec4& b);
Vec4 operator+(const Vec4& a, const Vec4& b);
Vec4 operator*(const Vec4& v, double scalar);
Vec4 operator*(double scalar, const Vec4& v);
Vec4 Lerp(const Vec4& from, const Vec4& to, double t);

}  // namespace raster
