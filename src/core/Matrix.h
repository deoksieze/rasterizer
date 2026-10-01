#pragma once

#include <array>

#include "Vec.h"

namespace raster {

template <int Rows, int Colls>  // NOLINT
class Matrix {
 public:
  Matrix() : data_{} {}

  double& At(int row, int column) { return data_[Colls * row + column]; }

  const double& At(int row, int column) const {
    return data_[Colls * row + column];
  }

  static Matrix MakeUnitMatrix() {
    static_assert(Rows == Colls, "MakeUnitMatrix requires a square matrix");

    Matrix mat;
    for (int i = 0; i < Rows; ++i) {
      mat.At(i, i) = 1;
    }

    return mat;
  }

 private:
  std::array<double, Rows * Colls> data_;
};

using Mat4 = Matrix<4, 4>;

Vec4 operator*(const Mat4& mat, const Vec4& vec);
Mat4 operator*(const Mat4& a, const Mat4& b);

Mat4 MakePerspectiveMatrix(double vertical_fov_radians, double aspect_ratio,
                           double near_plane, double far_plane);
Mat4 MakeTranslateMatrix(double tx, double ty, double tz);
Mat4 MakeScaleMatrix(double sx, double sy, double sz);
Mat4 MakeRotateXMatrix(double angle_radians);
Mat4 MakeRotateYMatrix(double angle_radians);
Mat4 MakeRotateZMatrix(double angle_radians);

}  // namespace raster
