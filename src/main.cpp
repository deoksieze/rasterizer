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
#include "geometry/Mesh.h"
#include "io/ObjLoader.h"
#include "io/PpmLoader.h"
#include "io/PpmWriter.h"
#include "raster/Clipper.h"
#include "raster/Framebuffer.h"
#include "raster/Projector.h"
#include "raster/Rasterizer.h"

namespace fs = std::filesystem;

namespace raster {

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const bool cCullBackFaces = false;

const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

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

  RasterizeTriangles(buffer, triangles, cCullBackFaces);

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SavePpmP3(out, buffer);
}
