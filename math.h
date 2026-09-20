
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include "Framebuffer.h"
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

Vec2 operator/(Vec2 v, double scalar) {
  return v * (1 / scalar);
}

Vec2 operator*(double scalar, Vec2 v) { return v * scalar; }

Vec2 Lerp(const Vec2& from, const Vec2& to, double t) {
  return from + t * (to - from);
}

struct MeshVertex {
  Vec4 pos;
  Color color;
  Vec2 uv;
};

struct TriangleMesh {
  int i0;
  int i1;
  int i2;
};

struct Mesh {
  std::vector<MeshVertex> vertices;
  std::vector<TriangleMesh> triangles;
};

template <int rows, int col>
class Matrix {
 public:
  Matrix() : data_{} {}

  double& At(int row, int column) { return data_[col * row + column]; }

  const double& At(int row, int column) const {
    return data_[col * row + column];
  }

 private:
  std::array<double, rows * col> data_;
};

using Mat4 = Matrix<4, 4>;

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