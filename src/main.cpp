#include <algorithm>
#include <array>
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

#include "core/Color.h"
#include "core/Matrix.h"
#include "core/Texture.h"
#include "core/Vec.h"
#include "geometry/Mesh.h"
#include "io/ObjLoader.h"
#include "io/PpmLoader.h"
#include "raster/Framebuffer.h"

namespace fs = std::filesystem;

namespace raster {

struct BarycentricCoordinates {
  double l1;
  double l2;
  double l3;
};

struct ScreenVertex {
  Vec2 pos;
  double depth;
  Color color;
  Vec2 uv;
  double inv_w;
};
struct ScreenTriangle {
  ScreenVertex a;
  ScreenVertex b;
  ScreenVertex c;
  Color color;
};

struct ClipVertex {
  Vec4 clip_pos;
  Color color;
  Vec2 uv;
};

ClipVertex Lerp(const ClipVertex& from, const ClipVertex& to, double t) {
  return {.clip_pos = Lerp(from.clip_pos, to.clip_pos, t),
          .color = Lerp(from.color, to.color, t),
          .uv = Lerp(from.uv, to.uv, t)};
}

struct ClipTriangle {
  ClipVertex a;
  ClipVertex b;
  ClipVertex c;
  Color color;
};

struct ClipPlane {
  Vec4 coeff;

  double operator()(const ClipVertex& vertex) const {
    return vertex.clip_pos.x * coeff.x + vertex.clip_pos.y * coeff.y +
           vertex.clip_pos.z * coeff.z + vertex.clip_pos.w * coeff.w;
  }
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

const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;

const std::array<ClipPlane, 6> cClipPlanes = {{
    {{1.0, 0.0, 0.0, 1.0}},   // left   (x + w >= 0)
    {{-1.0, 0.0, 0.0, 1.0}},  // right  (w - x >= 0)
    {{0.0, 1.0, 0.0, 1.0}},   // bottom (y + w >= 0)
    {{0.0, -1.0, 0.0, 1.0}},  // top    (w - y >= 0)
    {{0.0, 0.0, 1.0, 1.0}},   // near   (z + w >= 0)
    {{0.0, 0.0, -1.0, 1.0}},  // far    (w - z >= 0)
}};

const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

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

ClipVertex TransformVertex(const MeshVertex& vertex, const Mat4& model,
                           const Mat4& view, const Mat4& projection) {
  ClipVertex result{};

  const Vec4 cLocalPos{vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0};

  result.clip_pos = projection * view * model * cLocalPos;
  result.uv = vertex.uv;
  result.color = vertex.color;

  return result;
}

void TransformMeshToClipTriangles(const Mesh& mesh, const Mat4& model,
                                  const Mat4& view, const Mat4& projection,
                                  std::vector<ClipTriangle>& clip_triangles) {
  clip_triangles.clear();

  for (const auto& triangle : mesh.triangles) {
    ClipTriangle clip_triangle;

    clip_triangle.a =
        TransformVertex(mesh.vertices[triangle.i0], model, view, projection);

    clip_triangle.b =
        TransformVertex(mesh.vertices[triangle.i1], model, view, projection);

    clip_triangle.c =
        TransformVertex(mesh.vertices[triangle.i2], model, view, projection);

    clip_triangle.color = triangle.color;

    clip_triangles.push_back(clip_triangle);
  }
}

template <typename Func>
void ShClip(Func signed_distance_to_plane,
            std::vector<ClipVertex>& clip_polygon) {
  const std::vector<ClipVertex> cInput = clip_polygon;
  clip_polygon.clear();

  if (cInput.empty()) {
    return;
  }

  ClipVertex prev = cInput.back();
  for (ClipVertex curr : cInput) {
    const auto IsInsidePlane = [&](const ClipVertex& vertex) {  // NOLINT
      return signed_distance_to_plane(vertex) >= 0.0;
    };

    const double cDPrev = signed_distance_to_plane(prev);
    const double cDCurr = signed_distance_to_plane(curr);
    bool prev_inside = IsInsidePlane(prev);
    bool curr_inside = IsInsidePlane(curr);

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
}

void ClipTriangles(const std::vector<ClipTriangle>& clip_triangles,
                   std::vector<ClipTriangle>& clipped_clip_triangles) {
  clipped_clip_triangles.clear();

  std::vector<ClipVertex> clip_polygon;

  for (const ClipTriangle& tr : clip_triangles) {
    clip_polygon = {tr.a, tr.b, tr.c};

    for (const ClipPlane& plane : cClipPlanes) {
      ShClip(plane, clip_polygon);
      if (clip_polygon.size() < 3) {
        break;
      }
    }

    if (clip_polygon.size() < 3) {
      continue;
    }

    if (clip_polygon.size() == 3) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[1], clip_polygon[2], tr.color});
      continue;
    }

    // Для near-plane clipping исходного triangle здесь будет ровно 4 вершины.
    // Разбиваем выпуклый polygon на triangle fan, сохраняя порядок вершин.
    for (std::size_t i = 1; i + 1 < clip_polygon.size(); ++i) {
      clipped_clip_triangles.push_back(
          {clip_polygon[0], clip_polygon[i], clip_polygon[i + 1], tr.color});
    }
  }
}

void ProjectVertexToScreen(ScreenVertex& ver_to_project, const ClipVertex& ver,
                           const Framebuffer& buff) {
  const Vec4& v = ver.clip_pos;
  Vec3 ndc_position{v.x / v.w, v.y / v.w, v.z / v.w};
  ver_to_project.pos = Vec2{(ndc_position.x + 1.0) / 2 * buff.Width(),
                            (1.0 - ndc_position.y) / 2 * buff.Height()};
  ver_to_project.depth = ndc_position.z;
  ver_to_project.color = ver.color;
  ver_to_project.inv_w = 1 / ver.clip_pos.w;
  ver_to_project.uv = ver.uv * ver_to_project.inv_w;
}

void ProjectClippedTrianglesToScreen(
    const std::vector<ClipTriangle>& clipped_triangles,
    std::vector<ScreenTriangle>& triangles, const Framebuffer& buff) {
  ScreenTriangle triangle;

  for (const auto& tr : clipped_triangles) {
    ProjectVertexToScreen(triangle.a, tr.a, buff);
    ProjectVertexToScreen(triangle.b, tr.b, buff);
    ProjectVertexToScreen(triangle.c, tr.c, buff);
    triangle.color = tr.color;  // Временно работаю так с цветом

    triangles.push_back(triangle);
  }
}

// Временный метод для создания шахматно-подобной текстуры
Color SampleCheckerboard(Vec2 uv) {
  constexpr int cKCellsPerAxis = 8;

  uv.x = std::clamp(uv.x, 0.0, 1.0);
  uv.y = std::clamp(uv.y, 0.0, 1.0);

  const int cEllX =
      std::min(cKCellsPerAxis - 1, static_cast<int>(uv.x * cKCellsPerAxis));

  const int cEllY =
      std::min(cKCellsPerAxis - 1, static_cast<int>(uv.y * cKCellsPerAxis));

  const bool cIsLight = ((cEllX + cEllY) % 2) == 0;

  return cIsLight ? Color{1.0, 1.0, 1.0} : Color{0.0, 0.0, 0.0};
}

// Методы для работы с Системой

std::uint8_t ToByte(double value) {
  value = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(std::lround(value * cMaxColor));
}

std::ostream& OpenNextPpm(std::ofstream& output) {
  const fs::path cDirectory = "out";

  fs::create_directories(cDirectory);

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

}  // namespace raster

int main() {
  using namespace raster;  // NOLINT

  Framebuffer buffer = Framebuffer(cImageWidth, cImageHeight, cBackGroundColor);

  const double cAspect = static_cast<double>(buffer.Width()) / buffer.Height();

  std::ifstream mesh_file("assets/teapot.obj");

  if (!mesh_file.is_open()) {
    std::cerr << "cannot open .obj file\n";
    return 1;
  }
  const Mesh& c_mesh = LoadObj(mesh_file);
  const Texture cTexture = LoadPpmP6("assets/Ruslan_texture.ppm");

  const Mat4 cModel = MakeTranslateMatrix(0, 0.0, -10.0) *
                      MakeRotateYMatrix(std::numbers::pi / 5) *
                      MakeRotateXMatrix(std::numbers::pi / 6);
  const Mat4 cView = Mat4::MakeUnitMatrix();

  const Mat4 cProjection = MakePerspectiveMatrix(
      60.0 * std::numbers::pi / 180.0, cAspect, 0.1, 100.0);

  std::vector<ClipTriangle> clip_triangles{};
  std::vector<ClipTriangle> clipped_triangles{};
  std::vector<ScreenTriangle> triangles;
  TransformMeshToClipTriangles(c_mesh, cModel, cView, cProjection,
                               clip_triangles);
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
    // buffer.At(x, y) = tr.a.uv
    for (int y = box.min_y; y < box.max_y; y++) {
      for (int x = box.min_x; x < box.max_x; x++) {
        Vec2 p = {static_cast<double>(x) + cPixCentOffset,
                  static_cast<double>(y) + cPixCentOffset};

        BarycentricCoordinates bc = GetBarycentricCoordinates(tr, p);

        if (IsInside(bc)) {
          double depth =
              bc.l1 * tr.a.depth + bc.l2 * tr.b.depth + bc.l3 * tr.c.depth;

          if (buffer.PassDepthTest(depth, x, y)) {
            buffer.DepthAt(x, y) = depth;

            // double q =
            //     bc.l1 * tr.a.inv_w + bc.l2 * tr.b.inv_w + bc.l3 * tr.c.inv_w;
            // Vec2 uv_over_w =
            //     bc.l1 * tr.a.uv + bc.l2 * tr.b.uv + bc.l3 * tr.c.uv;

            // Vec2 uv = uv_over_w / q;

            // const Color color = cTexture.SampleNearest(uv);  // NOLINT
            const Color color = tr.color;  // NOLINT
            buffer.At(x, y) = color;
          }
        }
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, buffer);
}
