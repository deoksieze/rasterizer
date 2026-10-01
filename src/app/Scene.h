#pragma once

#include <string>
#include <vector>

#include "core/Color.h"
#include "core/Matrix.h"

namespace raster {

struct Camera {
  Mat4 view = Mat4::MakeUnitMatrix();
  double fov_y_degrees = 60.0;
  double near_plane = 0.1;
  double far_plane = 100.0;
};

struct Drawable {
  std::string mesh_path;
  std::string texture_path;
  Mat4 model = Mat4::MakeUnitMatrix();
};

struct Scene {
  int width = 512;
  int height = 512;
  Color background = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};
  Camera camera;
  bool cull_back_faces = false;
  std::vector<Drawable> objects;
  std::string output_path = "out/render.png";
};

}  // namespace raster
