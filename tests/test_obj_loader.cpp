#include <sstream>
#include <string>

#include "Check.h"
#include "io/ObjLoader.h"

using namespace raster;  // NOLINT

namespace {

const double cEpsilon = 1e-12;

Mesh Parse(const std::string& text) {
  std::istringstream input(text);
  return LoadObj(input);
}

}  // namespace

int main() {
  {
    const Mesh cMesh = Parse("v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");

    CHECK(cMesh.vertices.size() == 4);
    CHECK(cMesh.triangles.size() == 2);
    CHECK(!cMesh.has_texcoords);
    CHECK(cMesh.triangles[0].i0 == 0);
    CHECK(cMesh.triangles[0].i1 == 1);
    CHECK(cMesh.triangles[0].i2 == 2);
    CHECK(cMesh.triangles[1].i0 == 0);
    CHECK(cMesh.triangles[1].i1 == 2);
    CHECK(cMesh.triangles[1].i2 == 3);
  }

  {
    const Mesh cMesh =
        Parse("v 0 0 0\nv 1 0 0\nv 2 0 0\nv 2 1 0\nv 0 1 0\nf 1 2 3 4 5\n");

    CHECK(cMesh.vertices.size() == 5);
    CHECK(cMesh.triangles.size() == 3);
  }

  {
    const Mesh cMesh = Parse("v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");

    CHECK(cMesh.triangles.size() == 1);
    CHECK(cMesh.triangles[0].i0 == 0);
    CHECK(cMesh.triangles[0].i1 == 1);
    CHECK(cMesh.triangles[0].i2 == 2);
  }

  {
    const Mesh cMesh = Parse(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\n"
        "f 1/1 2/2 3/3\n");

    CHECK(cMesh.has_texcoords);
    CHECK(cMesh.vertices.size() == 3);
    CHECK_NEAR(cMesh.vertices[0].uv.x, 0.0, cEpsilon);
    CHECK_NEAR(cMesh.vertices[1].uv.x, 1.0, cEpsilon);
    CHECK_NEAR(cMesh.vertices[2].uv.y, 1.0, cEpsilon);
  }

  {
    const Mesh cMesh = Parse(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vn 0 0 1\nvn 0 0 1\nvn 0 0 1\n"
        "f 1//1 2//2 3//3\n");

    CHECK(!cMesh.has_texcoords);
    CHECK(cMesh.vertices.size() == 3);
    CHECK(cMesh.triangles.size() == 1);
  }

  {
    const Mesh cMesh = Parse(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 0.5 0.5\nvt 1 1\n"
        "f 1/1 2/2 3/3\n"
        "f 1/2 2/2 3/3\n");

    CHECK(cMesh.vertices.size() == 4);
    CHECK(cMesh.triangles.size() == 2);
  }

  {
    const Mesh cMesh =
        Parse("# a comment\r\nv 0 0 0\r\nv 1 0 0\r\nv 0 1 0\r\nf 1 2 3\r\n");

    CHECK(cMesh.vertices.size() == 3);
    CHECK(cMesh.triangles.size() == 1);
  }

  return Summary("obj_loader");
}
