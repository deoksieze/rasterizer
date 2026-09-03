#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Framebuffer.h"
#include "math.h"

namespace fs = std::filesystem;

struct Vec2 {
  double x;
  double y;
};

Vec2 operator-(Vec2 a, Vec2 b) { return Vec2{a.x - b.x, a.y - b.y}; }

struct BarycentricCoordinates {
  double l1;
  double l2;
  double l3;
};

struct Triangle {
  Vec2 a;
  Vec2 b;
  Vec2 c;
  Color color;
};

struct BoundingBox {
  int min_x;
  int max_x;
  int min_y;
  int max_y;
};

BoundingBox FindBoundingBox(const Triangle& tr, const Framebuffer& buffer) {
  const int cMinX = std::max(
      0, static_cast<int>(std::floor(std::min({tr.a.x, tr.b.x, tr.c.x}))));

  const int cMaxX =
      std::min(buffer.Width(),
               static_cast<int>(std::ceil(std::max({tr.a.x, tr.b.x, tr.c.x}))));

  const int cMinY = std::max(
      0, static_cast<int>(std::floor(std::min({tr.a.y, tr.b.y, tr.c.y}))));

  const int cMaxY =
      std::min(buffer.Height(),
               static_cast<int>(std::ceil(std::max({tr.a.y, tr.b.y, tr.c.y}))));

  return BoundingBox{cMinX, cMaxX, cMinY, cMaxY};
}

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;

const Color cColorA = {1.0, 0.0, 0.0};
const Color cColorB = {0.0, 1.0, 0.0};
const Color cColorC = {0.0, 0.0, 1.0};
const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

const Mesh cCube{
    .vertices =
        {
            // Back / far layer: z = -3.45.
            // Все вершины гарантированно перед камерой,
            // которая смотрит вдоль -Z.
            {{-0.65, -0.35, -3.45, 1.0}, {1.0, 0.0, 0.0}},  // 0
            {{+0.25, -0.35, -3.45, 1.0}, {0.0, 1.0, 0.0}},  // 1
            {{+0.25, +0.55, -3.45, 1.0}, {0.0, 0.0, 1.0}},  // 2
            {{-0.65, +0.55, -3.45, 1.0}, {1.0, 1.0, 0.0}},  // 3

            // Front / near layer: z = -2.55.
            // Вершины расположены ближе к камере, чем back layer.
            {{-0.25, -0.65, -2.55, 1.0}, {1.0, 0.0, 1.0}},  // 4
            {{+0.65, -0.65, -2.55, 1.0}, {0.0, 1.0, 1.0}},  // 5
            {{+0.65, +0.25, -2.55, 1.0}, {1.0, 1.0, 1.0}},  // 6
            {{-0.25, +0.25, -2.55, 1.0}, {0.3, 0.3, 0.3}},  // 7
        },

    .triangles =
        {
            // 1. Самая дальняя грань. Рисуем первой.
            {0, 2, 1},
            {0, 3, 2},

            // 2. Стороны, соединяющие дальний и ближний слои.
            // Пока z-buffer отсутствует, они должны быть до front face.
            {0, 4, 7},
            {0, 7, 3},

            {1, 2, 6},
            {1, 6, 5},

            {0, 1, 5},
            {0, 5, 4},

            // 3. Ближняя грань: рисуется после дальних граней.
            {4, 5, 6},
            {4, 6, 7},

            // 4. Верхняя грань остаётся последней,
            // как в твоём текущем порядке.
            {3, 7, 6},
            {3, 6, 2},
        },
};

// Методы для математики
double Orientation(const Vec2& a, const Vec2& b, const Vec2& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool IsInside(const BarycentricCoordinates& bc) {
  return bc.l1 >= 0.0 && bc.l2 >= 0.0 && bc.l3 >= 0.0;
}

BarycentricCoordinates GetBarycentricCoordinates(const Triangle& tr,
                                                 const Vec2& p) {
  const double cArea2 = Orientation(tr.a, tr.b, tr.c);

  return {
      Orientation(tr.b, tr.c, p) / cArea2,
      Orientation(tr.c, tr.a, p) / cArea2,
      Orientation(tr.a, tr.b, p) / cArea2,
  };
}

void Project(const Mesh& mesh, std::vector<Triangle>& triangles,
             const Framebuffer& buff) {
  Triangle triangle;

  for (const auto& tr : mesh.triangles) {
    triangle.a = {(mesh.vertices[tr.i0].pos.x + 1) * buff.Width() / 2,
                  (1 - mesh.vertices[tr.i0].pos.y) * buff.Height() / 2};

    triangle.b = {(mesh.vertices[tr.i1].pos.x + 1) * buff.Width() / 2,
                  (1 - mesh.vertices[tr.i1].pos.y) * buff.Height() / 2};

    triangle.c = {(mesh.vertices[tr.i2].pos.x + 1) * buff.Width() / 2,
                  (1 - mesh.vertices[tr.i2].pos.y) * buff.Height() / 2};

    triangle.color = mesh.vertices[tr.i0]
                         .color;  // Вот это очень тупо, но почему бы и нет пока

    triangles.push_back(triangle);
  }
}

Vec2 ProjectVertex(const Mat4& mat, const Vec4& vec, const Framebuffer& buff) {
  Vec4 v = mat * vec;
  Vec3 ncd_position{v.x / v.w, v.y / v.w, v.z / v.w};
  return Vec2{(ncd_position.x + 1.0) / 2 * buff.Width(),
              (1.0 - ncd_position.y) / 2 * buff.Height()};
}

void PerspectiveProjection(const Mesh& mesh, std::vector<Triangle>& triangles,
                           const Framebuffer& buff) {
  Triangle triangle;

  const double cAspect = static_cast<double>(cImageWidth) / cImageHeight;

  const double cFovY = 60.0 * std::numbers::pi / 180.0;

  Mat4 mat = MakePerspectiveMatrix(cFovY, cAspect, 0.1, 100.0);

  for (const auto& tr : mesh.triangles) {
    triangle.a = ProjectVertex(mat, mesh.vertices[tr.i0].pos, buff);
    triangle.b = ProjectVertex(mat, mesh.vertices[tr.i1].pos, buff);
    triangle.c = ProjectVertex(mat, mesh.vertices[tr.i2].pos, buff);
    triangle.color = mesh.vertices[tr.i0].color;

    triangles.push_back(triangle);
  }
}

// Методы для работы с Системой

std::uint8_t ToByte(double value) {
  value = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(std::lround(value * cMaxColor));
}

std::ostream& OpenNextPpm(std::ofstream& output) {
  const fs::path cDirectory = "examples";

  std::size_t file_count = 0;
  for (const auto& entry : fs::directory_iterator(cDirectory)) {
    if (entry.is_regular_file()) {
      ++file_count;
    }
  }

  const fs::path cFilename =
      cDirectory / ("image_" + std::to_string(file_count + 1) + ".ppm");

  output.open(cFilename, std::ios::out | std::ios::binary);

  if (!output) {
    throw std::runtime_error("Не удалось открыть файл: " + cFilename.string());
  }

  return output;
}

void SaveImage(std::ostream& stream, const Framebuffer& buff) {
  stream << "P3\n";
  stream << buff.Width() << " " << buff.Height() << "\n";
  stream << "255\n";

  for (int y = 0; y < buff.Height(); y++) {
    for (int x = 0; x < buff.Width(); x++) {
      Color color = buff.At(x, y);
      stream << static_cast<int>(ToByte(color.r)) << ' '
             << static_cast<int>(ToByte(color.g)) << ' '
             << static_cast<int>(ToByte(color.b)) << '\n';
    }
  }
}

int main() {
  std::vector<Triangle> triangles;
  Framebuffer buffer = Framebuffer(cImageWidth, cImageHeight, cBackGroundColor);

  PerspectiveProjection(cCube, triangles, buffer);

  // Ищем bounding box
  for (const auto& tr : triangles) {
    BoundingBox box = FindBoundingBox(tr, buffer);

    if (box.max_x <= box.min_x || box.max_y <= box.min_y) {
      continue;
    }

    for (int y = box.min_y; y < box.max_y; y++) {
      for (int x = box.min_x; x < box.max_x; x++) {
        Vec2 p = {static_cast<double>(x) + cPixCentOffset,
                  static_cast<double>(y) + cPixCentOffset};

        BarycentricCoordinates bc = GetBarycentricCoordinates(tr, p);

        if (IsInside(bc)) {
          // buffer.At(x, y) = cColorA * bc.l1 + cColorB * bc.l2 + cColorC *
          // bc.l3;
          buffer.At(x, y) = tr.color;
        }
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, buffer);
}