
#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "Color.h"

template <int rows, int col>
class Matrix;
using Mat4 = Matrix<4, 4>;

struct Vec4 {
  double x;
  double y;
  double z;
  double w;
};

Vec4 operator+(Vec4 a, const Vec4& b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  a.w += b.w;
  return a;
}

Vec4 operator-(Vec4 a, const Vec4& b) {
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;
  a.w -= b.w;
  return a;
}

Vec4 operator*(Vec4 v, double scalar) {
  v.x *= scalar;
  v.y *= scalar;
  v.z *= scalar;
  v.w *= scalar;
  return v;
}

Vec4 operator*(double scalar, Vec4 v) { return v * scalar; }

Vec4 Lerp(const Vec4& from, const Vec4& to, double t) {
  return from + t * (to - from);
}

struct Vec3 {
  double x;
  double y;
  double z;
};

struct Vec2 {
  double x;
  double y;
};

Vec2 operator-(Vec2 a, Vec2 b) { return Vec2{a.x - b.x, a.y - b.y}; }

Vec2 operator+(Vec2 a, const Vec2& b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

Vec2 operator*(Vec2 v, double scalar) {
  v.x *= scalar;
  v.y *= scalar;
  return v;
}

Vec2 operator/(Vec2 v, double scalar) { return v * (1 / scalar); }

Vec2 operator*(double scalar, Vec2 v) { return v * scalar; }

Vec2 Lerp(const Vec2& from, const Vec2& to, double t) {
  return from + t * (to - from);
}

template <int Rows, int Colls>  // NOLINT
class Matrix {
 public:
  Matrix() : data_{} {}

  double& At(int row, int column) { return data_[Colls * row + column]; }

  const double& At(int row, int column) const {
    return data_[Colls * row + column];
  }

  static Mat4 MakeUnitMatrix() {
    Mat4 mat = Mat4();
    for (int i = 0; i < 4; i++) {
      mat.At(i, i) = 1;
    }

    return mat;
  }

 private:
  std::array<double, Rows * Colls> data_;
};

Vec4 operator*(const Mat4& mat, const Vec4& vec) {
  Vec4 ans;

  std::array<double, 4> temp;

  for (int row = 0; row < 4; row++) {
    temp[row] = mat.At(row, 0) * vec.x + mat.At(row, 1) * vec.y +
                mat.At(row, 2) * vec.z + mat.At(row, 3) * vec.w;
  }

  ans.x = temp[0];
  ans.y = temp[1];
  ans.z = temp[2];
  ans.w = temp[3];

  return ans;
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
  Mat4 c;

  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      double sum = 0;

      for (int i = 0; i < 4; i++) {
        sum += a.At(row, i) * b.At(i, col);
      }

      c.At(row, col) = sum;
    }
  }

  return c;
}

Mat4 MakePerspectiveMatrix(double vertical_fov_radians, double aspect_ratio,
                           double near_plane, double far_plane) {
  if (vertical_fov_radians <= 0.0 || vertical_fov_radians >= std::numbers::pi) {
    throw std::invalid_argument("FOV must be in (0, pi)");
  }

  if (aspect_ratio <= 0.0) {
    throw std::invalid_argument("Aspect ratio must be positive");
  }

  if (near_plane <= 0.0 || far_plane <= near_plane) {
    throw std::invalid_argument("Expected 0 < nearPlane < farPlane");
  }

  const double cF = 1.0 / std::tan(vertical_fov_radians / 2.0);

  Mat4 projection;  // ВАЖНО: Matrix() должен создавать нулевую матрицу.

  projection.At(0, 0) = cF / aspect_ratio;
  projection.At(1, 1) = cF;

  projection.At(2, 2) = -(far_plane + near_plane) / (far_plane - near_plane);
  projection.At(2, 3) =
      -(2.0 * far_plane * near_plane) /  // NOLINT(readability-magic-numbers)
      (far_plane - near_plane);

  projection.At(3, 2) = -1.0;

  return projection;
}

Mat4 MakeTranslateMatrix(double tx, double ty, double tz) {
  Mat4 mat = Mat4::MakeUnitMatrix();
  mat.At(0, 3) = tx;
  mat.At(1, 3) = ty;
  mat.At(2, 3) = tz;
  return mat;
}

Mat4 MakeScaleMatrix(double sx, double sy, double sz) {
  Mat4 mat = Mat4::MakeUnitMatrix();
  mat.At(0, 0) = sx;
  mat.At(1, 1) = sy;
  mat.At(2, 2) = sz;
  return mat;
}

Mat4 MakeRotateXMatrix(double angle_radians) {
  const double c = std::cos(angle_radians);  // NOLINT
  const double s = std::sin(angle_radians);  // NOLINT

  Mat4 mat = Mat4::MakeUnitMatrix();
  mat.At(1, 1) = c;
  mat.At(1, 2) = -s;
  mat.At(2, 1) = s;
  mat.At(2, 2) = c;
  return mat;
}

Mat4 MakeRotateYMatrix(double angle_radians) {
  const double c = std::cos(angle_radians);  // NOLINT
  const double s = std::sin(angle_radians);  // NOLINT

  Mat4 mat = Mat4::MakeUnitMatrix();
  mat.At(0, 0) = c;
  mat.At(0, 2) = s;
  mat.At(2, 0) = -s;
  mat.At(2, 2) = c;
  return mat;
}

Mat4 MakeRotateZMatrix(double angle_radians) {
  const double c = std::cos(angle_radians);  // NOLINT
  const double s = std::sin(angle_radians);  // NOLINT

  Mat4 mat = Mat4::MakeUnitMatrix();
  mat.At(0, 0) = c;
  mat.At(0, 1) = -s;
  mat.At(1, 0) = s;
  mat.At(1, 1) = c;
  return mat;
}
