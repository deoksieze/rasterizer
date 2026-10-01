#pragma once

#include <vector>

#include "core/Color.h"
#include "core/Vec.h"

namespace raster {

struct MeshVertex {
  Vec3 pos;
  Color color;
  Vec2 uv;
};

struct Triangle {
  int i0;
  int i1;
  int i2;
  Color color;
};

struct Mesh {
  std::vector<MeshVertex> vertices;
  std::vector<Triangle> triangles;
};

}  // namespace raster
