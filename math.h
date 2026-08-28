
#include <array>

#include "Framebuffer.h"
struct Vec4 {
  double x;
  double y;
  double z;
  double w;
};

struct Vertex {
  Vec4 pos;
  Color color;
};

struct TriangleMesh {
  int i0;
  int i1;
  int i2;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<TriangleMesh> triangles;
};

template <int rows, int col>
class Matrix {
 public:
  Matrix() : height_(rows), width_(col), data_{} {}

  double& At(int row, int column) { return data_[width_ * row + column]; }

  const double& At(int row, int column) const {
    return data_[width_ * row + column];
  }

 private:
  std::array<double, rows * col> data_;

  int height_;
  int width_;
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