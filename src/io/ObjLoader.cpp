#include "io/ObjLoader.h"

#include <array>
#include <cstddef>
#include <istream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace raster {
namespace {

Color DebugTriangleColor(std::size_t triangle_index) {
  static constexpr std::array<Color, 10> cPalette = {
      Color{1.0, 0.20, 0.20},   // red
      Color{0.20, 0.85, 0.30},  // green
      Color{0.20, 0.45, 1.00},  // blue
      Color{1.00, 0.80, 0.15},  // yellow
      Color{0.95, 0.25, 0.80},  // pink
      Color{0.10, 0.85, 0.85},  // cyan
      Color{1.00, 0.50, 0.10},  // orange
      Color{0.60, 0.35, 0.95},  // purple
      Color{0.65, 0.90, 0.25},  // lime
      Color{0.95, 0.55, 0.55}   // salmon
  };

  return cPalette[triangle_index % cPalette.size()];
}

int ParseObjPositionIndex(std::string_view face_token,
                          std::size_t position_count) {
  const std::size_t cSlash = face_token.find('/');

  const std::string_view cPositionText = face_token.substr(0, cSlash);

  if (cPositionText.empty()) {
    throw std::runtime_error("Invalid OBJ face vertex: missing position index");
  }

  const int cObjIndex = std::stoi(std::string(cPositionText));

  // Пока поддерживаем только обычные положительные OBJ-indices.
  // OBJ: 1, 2, 3, ...
  // C++: 0, 1, 2, ...
  if (cObjIndex <= 0) {
    throw std::runtime_error(
        "Only positive OBJ position indices are supported");
  }

  const int cIndex = cObjIndex - 1;

  if (static_cast<std::size_t>(cIndex) >= position_count) {
    throw std::runtime_error(
        "OBJ face references a position that has not been read");
  }

  return cIndex;
}

}  // namespace

Mesh LoadObj(std::istream& input) {
  Mesh mesh;

  std::string line;
  std::size_t line_number = 0;

  std::mt19937 rng(42);  // NOLINT
  std::uniform_real_distribution<float> random_uv(0.0, 1.0);

  while (std::getline(input, line)) {
    ++line_number;

    std::istringstream line_stream(line);
    std::string tag;

    if (!(line_stream >> tag)) {
      continue;  // Пустая или состоящая из пробелов строка.
    }

    if (tag[0] == '#') {
      continue;
    }

    if (tag == "v") {
      Vec3 position{};

      if (!(line_stream >> position.x >> position.y >> position.z)) {
        throw std::runtime_error("Invalid OBJ vertex at line " +
                                 std::to_string(line_number));
      }

      mesh.vertices.push_back(
          MeshVertex{.pos = position,
                     .color = Color{1.0, 1.0, 1.0},
                     .uv = Vec2{random_uv(rng), random_uv(rng)}});
    } else if (tag == "f") {
      std::string token0;
      std::string token1;
      std::string token2;

      if (!(line_stream >> token0 >> token1 >> token2)) {
        throw std::runtime_error("Invalid triangular OBJ face at line " +
                                 std::to_string(line_number));
      }

      // Пока не принимаем quad / n-gon, чтобы не иметь
      // неявной неправильной triangulation.
      std::string extra_token;
      if (line_stream >> extra_token) {
        throw std::runtime_error(
            "Only triangular OBJ faces are supported; line " +
            std::to_string(line_number));
      }

      const std::size_t cTriangleIndex = mesh.triangles.size();

      mesh.triangles.push_back(
          Triangle{.i0 = ParseObjPositionIndex(token0, mesh.vertices.size()),
                   .i1 = ParseObjPositionIndex(token1, mesh.vertices.size()),
                   .i2 = ParseObjPositionIndex(token2, mesh.vertices.size()),
                   .color = DebugTriangleColor(cTriangleIndex)});
    }

    // Пока пропускаем:
    // vt, vn, o, g, s, usemtl, mtllib, ...
  }

  return mesh;
}

}  // namespace raster
