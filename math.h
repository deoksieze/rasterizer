
#include "Framebuffer.h"
struct Vec3 {
  double x;
  double y;
  double z;
};

struct Vertex {
  Vec3 pos;
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