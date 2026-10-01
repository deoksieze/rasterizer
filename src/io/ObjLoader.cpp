#include "io/ObjLoader.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

int ResolveObjIndex(std::string_view text, std::size_t element_count,
                    std::string_view field, std::size_t line_number) {
  if (text.empty()) {
    return -1;
  }

  std::size_t parsed_characters = 0;
  const int cIndex = std::stoi(std::string(text), &parsed_characters);

  if (parsed_characters != text.size()) {
    throw std::runtime_error("Invalid OBJ " + std::string(field) + " at line " +
                             std::to_string(line_number) + ": '" +
                             std::string(text) + "'");
  }

  if (cIndex == 0) {
    throw std::runtime_error("OBJ indices are 1-based, got 0 at line " +
                             std::to_string(line_number));
  }

  const int cResolved =
      (cIndex > 0) ? cIndex - 1 : static_cast<int>(element_count) + cIndex;

  if (cResolved < 0 || static_cast<std::size_t>(cResolved) >= element_count) {
    throw std::runtime_error("OBJ " + std::string(field) +
                             " index out of range at line " +
                             std::to_string(line_number));
  }

  return cResolved;
}

struct ObjFaceVertex {
  int position = -1;
  int texcoord = -1;
  int normal = -1;
};

std::array<std::string_view, 3> SplitObjFaceToken(std::string_view token) {
  std::array<std::string_view, 3> parts{};

  std::size_t start = 0;
  for (std::size_t part = 0; part < parts.size(); ++part) {
    const std::size_t cSlash = token.find('/', start);
    parts[part] = token.substr(start, cSlash - start);

    if (cSlash == std::string_view::npos) {
      break;
    }

    start = cSlash + 1;
  }

  return parts;
}

ObjFaceVertex ParseObjFaceVertex(std::string_view token,
                                 std::size_t position_count,
                                 std::size_t texcoord_count,
                                 std::size_t normal_count,
                                 std::size_t line_number) {
  const std::array<std::string_view, 3> cParts = SplitObjFaceToken(token);

  ObjFaceVertex vertex;
  vertex.position =
      ResolveObjIndex(cParts[0], position_count, "position", line_number);
  vertex.texcoord =
      ResolveObjIndex(cParts[1], texcoord_count, "texcoord", line_number);
  vertex.normal =
      ResolveObjIndex(cParts[2], normal_count, "normal", line_number);

  return vertex;
}

using VertexCache = std::map<std::array<int, 2>, int>;

int GetOrCreateVertex(Mesh& mesh, VertexCache& cache,
                      const std::vector<Vec3>& positions,
                      const std::vector<Vec2>& texcoords,
                      const ObjFaceVertex& face_vertex) {
  const std::array<int, 2> cKey{face_vertex.position, face_vertex.texcoord};

  const auto cIt = cache.find(cKey);

  if (cIt != cache.end()) {
    return cIt->second;
  }

  const int cIndex = static_cast<int>(mesh.vertices.size());

  MeshVertex vertex;
  vertex.pos = positions[static_cast<std::size_t>(face_vertex.position)];
  vertex.color = Colors::White;
  vertex.uv = (face_vertex.texcoord >= 0)
                  ? texcoords[static_cast<std::size_t>(face_vertex.texcoord)]
                  : Vec2{0.0, 0.0};

  mesh.vertices.push_back(vertex);
  cache.emplace(cKey, cIndex);

  return cIndex;
}

}  // namespace

Mesh LoadObj(std::istream& input) {
  Mesh mesh;

  std::vector<Vec3> positions;
  std::vector<Vec2> texcoords;
  std::size_t normal_count = 0;
  VertexCache cache;

  std::string line;
  std::size_t line_number = 0;

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

      positions.push_back(position);
    } else if (tag == "vt") {
      Vec2 texcoord{};
      double w = 1.0;

      if (!(line_stream >> texcoord.x >> texcoord.y)) {
        throw std::runtime_error("Invalid OBJ texcoord at line " +
                                 std::to_string(line_number));
      }

      line_stream >> w;  // Третья координата vt опциональна.

      texcoords.push_back(texcoord);
    } else if (tag == "vn") {
      ++normal_count;
    } else if (tag == "f") {
      std::vector<ObjFaceVertex> face_vertices;

      std::string token;
      while (line_stream >> token) {
        face_vertices.push_back(ParseObjFaceVertex(token, positions.size(),
                                                   texcoords.size(),
                                                   normal_count, line_number));
      }

      if (face_vertices.size() < 3) {
        throw std::runtime_error("OBJ face needs at least 3 vertices at line " +
                                 std::to_string(line_number));
      }

      std::vector<int> indices;
      indices.reserve(face_vertices.size());
      for (const ObjFaceVertex& face_vertex : face_vertices) {
        indices.push_back(
            GetOrCreateVertex(mesh, cache, positions, texcoords, face_vertex));
      }

      // Выпуклый n-gon разбиваем в triangle fan, сохраняя порядок вершин.
      for (std::size_t i = 1; i + 1 < indices.size(); ++i) {
        mesh.triangles.push_back(
            Triangle{.i0 = indices[0],
                     .i1 = indices[i],
                     .i2 = indices[i + 1],
                     .color = DebugTriangleColor(mesh.triangles.size())});
      }
    }

    // Пока пропускаем:
    // o, g, s, usemtl, mtllib, ...
  }

  mesh.has_texcoords = !texcoords.empty();

  return mesh;
}

Mesh LoadObj(const std::string& filename) {
  std::ifstream input(filename);

  if (!input) {
    throw std::runtime_error("Failed to open OBJ file: " + filename);
  }

  return LoadObj(input);
}

}  // namespace raster