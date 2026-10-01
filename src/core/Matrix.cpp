#include "Matrix.h"

#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace raster {

Vec4 operator*(const Mat4& mat, const Vec4& vec) {
  std::array<double, 4> temp{};

  for (int row = 0; row < 4; row++) {
    temp[row] = mat.At(row, 0) * vec.x + mat.At(row, 1) * vec.y +
                mat.At(row, 2) * vec.z + mat.At(row, 3) * vec.w;
  }

  return Vec4{temp[0], temp[1], temp[2], temp[3]};
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

Mat4 MakeLookAtMatrix(const Vec3& eye, const Vec3& center, const Vec3& up) {
  const Vec3 cForward = Normalize(center - eye);
  const Vec3 cRight = Normalize(Cross(cForward, up));
  const Vec3 cUp = Cross(cRight, cForward);

  Mat4 view = Mat4::MakeUnitMatrix();

  view.At(0, 0) = cRight.x;
  view.At(0, 1) = cRight.y;
  view.At(0, 2) = cRight.z;
  view.At(0, 3) = -Dot(cRight, eye);

  view.At(1, 0) = cUp.x;
  view.At(1, 1) = cUp.y;
  view.At(1, 2) = cUp.z;
  view.At(1, 3) = -Dot(cUp, eye);

  view.At(2, 0) = -cForward.x;
  view.At(2, 1) = -cForward.y;
  view.At(2, 2) = -cForward.z;
  view.At(2, 3) = Dot(cForward, eye);

  return view;
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

}  // namespace raster
