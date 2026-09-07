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

struct ScreenVertex {
  Vec2 pos;
  double depth;
  // Color color;
};
struct ScreenTriangle {
  ScreenVertex a;
  ScreenVertex b;
  ScreenVertex c;
  Color color;
};

struct ClipVertex {
  Vec4 pos;
  Color color;
};

ClipVertex Lerp(const ClipVertex& from, const ClipVertex& to, double t) {
  return {
      .pos = Lerp(from.pos, to.pos, t),
      .color = Lerp(from.color, to.color, t),
  };
}

struct ClipTriangle {
  ClipVertex a;
  ClipVertex b;
  ClipVertex c;
};

struct BoundingBox {
  int min_x;
  int max_x;
  int min_y;
  int max_y;
};

BoundingBox FindBoundingBox(const ScreenTriangle& tr,
                            const Framebuffer& buffer) {
  Vec2 a = tr.a.pos;
  Vec2 b = tr.b.pos;
  Vec2 c = tr.c.pos;

  const int cMinX =
      std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));

  const int cMaxX = std::min(
      buffer.Width(), static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));

  const int cMinY =
      std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));

  const int cMaxY = std::min(
      buffer.Height(), static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

  return BoundingBox{cMinX, cMaxX, cMinY, cMaxY};
}

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const bool cCullBackFaces = false;
const double cFovY = 90.0 * std::numbers::pi / 180.0;

const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;
const double cNearPlane = 0.1;
const double cFarPlane = 100.0;

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

            // 3. Ближняя грань: рисуется после дальних граней.
            {4, 5, 6},
            {4, 6, 7},
            // 2. Стороны, соединяющие дальний и ближний слои.
            // Пока z-buffer отсутствует, они должны быть до front face.
            {0, 4, 7},
            {0, 7, 3},

            {1, 2, 6},
            {1, 6, 5},

            {0, 1, 5},
            {0, 5, 4},

            // 4. Верхняя грань остаётся последней,
            // как в твоём текущем порядке.
            {3, 7, 6},
            {3, 6, 2},

            // 1. Самая дальняя грань. Рисуем первой.
            {0, 2, 1},
            {0, 3, 2},
        },
};

const Mesh cIntersectingTriangles{
    .vertices =
        {
            // Triangle 1: красный.
            // Его глубина меняется слева направо:
            // слева он ближе к камере, справа — дальше.
            {{-1.20, -0.85, -2.00, 1.0}, {1.0, 0.0, 0.0}},  // 0
            {{+1.20, -0.85, -5.00, 1.0}, {1.0, 0.0, 0.0}},  // 1
            {{+0.00, +1.10, -3.50, 1.0}, {1.0, 0.0, 0.0}},  // 2

            // Triangle 2: зелёный.
            // Его глубина меняется в противоположную сторону:
            // справа он ближе, слева — дальше.
            {{-1.20, +0.70, -5.00, 1.0}, {0.0, 1.0, 0.0}},  // 3
            {{+1.20, +0.70, -2.00, 1.0}, {0.0, 1.0, 0.0}},  // 4
            {{+0.00, -1.10, -3.50, 1.0}, {0.0, 1.0, 0.0}},  // 5
        },

    .triangles =
        {
            // Красный triangle рисуется первым.
            {0, 1, 2},

            // Зелёный — вторым.
            // Без z-buffer он полностью перекроет красный
            // во всей общей области screen-space.
            {3, 4, 5},
        },
};

const Mesh cNearPlaneClippingTests{
    .vertices =
        {
            // ------------------------------------------------------------
            // Test 1: все 3 вершины inside near plane.
            // Ожидается: один исходный triangle проходит без изменения.
            // Красный triangle, расположен слева сверху.
            // ------------------------------------------------------------
            {{-1.40, +0.20, -2.00, 1.0}, {1.0, 0.0, 0.0}},  // 0
            {{-0.45, +0.20, -2.00, 1.0}, {1.0, 0.0, 0.0}},  // 1
            {{-0.90, +1.05, -2.00, 1.0}, {1.0, 0.0, 0.0}},  // 2

            // ------------------------------------------------------------
            // Test 2: ровно 1 вершина inside, 2 outside.
            // Ожидается: маленький clipped triangle из:
            // - единственной исходной inside-вершины;
            // - двух пересечений с near plane.
            // Зелёный triangle, расположен справа сверху.
            // ------------------------------------------------------------
            {{+0.90, +0.01, -2.00, 1.0}, {0.0, 1.0, 0.0}},  // 3: inside
            {{+0.35, +0.15, -0.05, 1.0}, {0.0, 1.0, 0.0}},  // 4: outside
            {{+1.45, +0.15, -0.05, 1.0}, {0.0, 1.0, 0.0}},  // 5: outside

            // ------------------------------------------------------------
            // Test 3: ровно 2 вершины inside, 1 outside.
            // Ожидается: clipped quadrilateral,
            // после triangulation: 2 triangles.
            // Синий triangle, расположен слева снизу.
            // ------------------------------------------------------------
            {{-1.35, 2, -4.00, 1.0}, {0.0, 0.0, 1.0}},  // 6: inside
            {{1.45, -1.50, -2.00, 1.0}, {0.0, 0.0, 1.0}},  // 7: inside
            {{-0.90, -0.20, -0.05, 1.0}, {0.0, 0.0, 1.0}},  // 8: outside

            // ------------------------------------------------------------
            // Test 4: все 3 вершины outside.
            // Ожидается: 0 output triangles, ничего не рисуется.
            // Жёлтый triangle, расположен справа снизу.
            // ------------------------------------------------------------
            {{+0.45, -1.05, -0.05, 1.0}, {1.0, 1.0, 0.0}},  // 9
            {{+1.35, -1.05, -0.05, 1.0}, {1.0, 1.0, 0.0}},  // 10
            {{+0.90, -0.20, -0.05, 1.0}, {1.0, 1.0, 0.0}},  // 11
        },

    .triangles =
        {
            {0, 1, 2},    // Test 1: 3 inside → 1 triangle
            {3, 4, 5},    // Test 2: 1 inside → 1 clipped triangle
            {6, 7, 8},    // Test 3: 2 inside → quad → 2 triangles
            {9, 10, 11},  // Test 4: 0 inside → discard
        },
};

// Методы для математики
double Orientation(const Vec2& a, const Vec2& b, const Vec2& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool IsFrontFacing(const ScreenTriangle& tr) {
  return Orientation(tr.a.pos, tr.b.pos, tr.c.pos) < 0;
}

bool IsInside(const BarycentricCoordinates& bc) {
  return bc.l1 >= 0.0 && bc.l2 >= 0.0 && bc.l3 >= 0.0;
}

BarycentricCoordinates GetBarycentricCoordinates(const ScreenTriangle& tr,
                                                 const Vec2& p) {
  const double cArea2 = Orientation(tr.a.pos, tr.b.pos, tr.c.pos);

  return {
      Orientation(tr.b.pos, tr.c.pos, p) / cArea2,
      Orientation(tr.c.pos, tr.a.pos, p) / cArea2,
      Orientation(tr.a.pos, tr.b.pos, p) / cArea2,
  };
}

void ProjectVertex(ScreenVertex& ver_to_project, const Mat4& mat,
                   const Vec4& vec, const Framebuffer& buff) {
  Vec4 v = mat * vec;
  Vec3 ndc_position{v.x / v.w, v.y / v.w, v.z / v.w};
  ver_to_project.pos = Vec2{(ndc_position.x + 1.0) / 2 * buff.Width(),
                            (1.0 - ndc_position.y) / 2 * buff.Height()};
  ver_to_project.depth = ndc_position.z;
}

void ProjectMeshToScreen(const Mesh& mesh,
                         std::vector<ScreenTriangle>& triangles,
                         const Framebuffer& buff) {
  ScreenTriangle triangle;

  const double cAspect = static_cast<double>(buff.Width()) / buff.Height();

  Mat4 mat = MakePerspectiveMatrix(cFovY, cAspect, cNearPlane, cFarPlane);

  for (const auto& tr : mesh.triangles) {
    ProjectVertex(triangle.a, mat, mesh.vertices[tr.i0].pos, buff);
    ProjectVertex(triangle.b, mat, mesh.vertices[tr.i1].pos, buff);
    ProjectVertex(triangle.c, mat, mesh.vertices[tr.i2].pos, buff);
    triangle.color =
        mesh.vertices[tr.i0].color;  // Временно работаю так с цветом

    triangles.push_back(triangle);
  }
}

void TransformMeshToClipTriangles(const Mesh& mesh, const Mat4& P,  // NOLINT
                                  std::vector<ClipTriangle>& clip_triangles) {
  clip_triangles.clear();  // ХЗ оставить это так или нет
  ClipTriangle clip_triangle;

  for (const auto& tr : mesh.triangles) {
    clip_triangle.a.pos = P * mesh.vertices[tr.i0].pos;
    clip_triangle.a.color = mesh.vertices[tr.i0].color;

    clip_triangle.b.pos = P * mesh.vertices[tr.i1].pos;
    clip_triangle.b.color = mesh.vertices[tr.i1].color;

    clip_triangle.c.pos = P * mesh.vertices[tr.i2].pos;
    clip_triangle.c.color = mesh.vertices[tr.i2].color;

    clip_triangles.push_back(clip_triangle);
  }
}

void ClipTriangles(const std::vector<ClipTriangle>& clip_triangles,
                   std::vector<ClipTriangle>& clipped_clip_triangles) {
  clipped_clip_triangles.clear();

  const auto SignedDistanceToNearPlane =  // NOLINT
      [](const ClipVertex& vertex) { return vertex.pos.z + vertex.pos.w; };

  const auto IsInsideNearPlane = [&](const ClipVertex& vertex) {  // NOLINT
    return SignedDistanceToNearPlane(vertex) >= 0.0;
  };
  std::vector<ClipVertex> clip_polygon;

  for (const ClipTriangle& tr : clip_triangles) {
    clip_polygon.clear();
    std::vector<ClipVertex> input = {tr.a, tr.b, tr.c};
    ClipVertex prev = input.back();
    for (ClipVertex curr : input) {
      const double cDPrev = SignedDistanceToNearPlane(prev);
      const double cDCurr = SignedDistanceToNearPlane(curr);
      bool prev_inside = IsInsideNearPlane(prev);
      bool curr_inside = IsInsideNearPlane(curr);

      if (prev_inside && curr_inside) {
        clip_polygon.push_back(curr);
      }

      else if (prev_inside && !curr_inside) {
        double t = cDPrev / (cDPrev - cDCurr);
        clip_polygon.push_back(Lerp(prev, curr, t));
      }

      else if (!prev_inside && curr_inside) {
        double t = cDPrev / (cDPrev - cDCurr);
        clip_polygon.push_back(Lerp(prev, curr, t));
        clip_polygon.push_back(curr);
      }

      prev = curr;
    }

    if (clip_polygon.size() < 3) {
      continue;
    }

    if (clip_polygon.size() == 3) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[1], clip_polygon[2]});
      continue;
    }

    // Для near-plane clipping исходного triangle здесь будет ровно 4 вершины.
    // Разбиваем выпуклый polygon на triangle fan, сохраняя порядок вершин.
    for (std::size_t i = 1; i + 1 < clip_polygon.size(); ++i) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[i], clip_polygon[i + 1]});
    }
  }
}

void ProjectVertexToScreen(ScreenVertex& ver_to_project, const Vec4& v,
                           const Framebuffer& buff) {
  Vec3 ndc_position{v.x / v.w, v.y / v.w, v.z / v.w};
  ver_to_project.pos = Vec2{(ndc_position.x + 1.0) / 2 * buff.Width(),
                            (1.0 - ndc_position.y) / 2 * buff.Height()};
  ver_to_project.depth = ndc_position.z;
}

void ProjectClippedTrianglesToScreen(
    const std::vector<ClipTriangle>& clipped_triangles,
    std::vector<ScreenTriangle>& triangles, const Framebuffer& buff) {
  ScreenTriangle triangle;

  for (const auto& tr : clipped_triangles) {
    ProjectVertexToScreen(triangle.a, tr.a.pos, buff);
    ProjectVertexToScreen(triangle.b, tr.b.pos, buff);
    ProjectVertexToScreen(triangle.c, tr.c.pos, buff);
    triangle.color = tr.a.color;  // Временно работаю так с цветом

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
  Framebuffer buffer = Framebuffer(cImageWidth, cImageHeight, cBackGroundColor);

  const double cAspect = static_cast<double>(buffer.Width()) / buffer.Height();
  Mat4 mat = MakePerspectiveMatrix(cFovY, cAspect, cNearPlane, cFarPlane);

  // ProjectMeshToScreen(cCube, triangles, buffer);
  std::vector<ClipTriangle> clip_triangles{};
  std::vector<ClipTriangle> clipped_triangles{};
  std::vector<ScreenTriangle> triangles;
  TransformMeshToClipTriangles(cNearPlaneClippingTests, mat, clip_triangles);
  ClipTriangles(clip_triangles, clipped_triangles);
  ProjectClippedTrianglesToScreen(clipped_triangles, triangles, buffer);

  // Ищем bounding box
  for (const auto& tr : triangles) {
    if (!IsFrontFacing(tr) && cCullBackFaces) {
      continue;
    }

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
          double depth =
              bc.l1 * tr.a.depth + bc.l2 * tr.b.depth + bc.l3 * tr.c.depth;

          if (buffer.PassDepthTest(depth, x, y)) {
            // buffer.At(x, y) = cColorA * bc.l1 + cColorB * bc.l2 + cColorC *
            // bc.l3;
            buffer.DepthAt(x, y) = depth;
            buffer.At(x, y) = tr.color;
          }
        }
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, buffer);
}