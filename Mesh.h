#pragma once

#include "Math.h"
#include "Color.h"

struct MeshVertex {
  Vec4 pos;
  Color color;
  Vec2 uv;
};

struct TriangleMesh {
  int i0;
  int i1;
  int i2;
};

struct Mesh {
  std::vector<MeshVertex> vertices;
  std::vector<TriangleMesh> triangles;
};
