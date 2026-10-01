#include "raster/Projector.h"

namespace raster {
namespace {

ClipVertex TransformVertex(const MeshVertex& vertex, const Mat4& model,
                           const Mat4& view, const Mat4& projection) {
  ClipVertex result{};

  const Vec4 cLocalPos{vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0};

  result.clip_pos = projection * view * model * cLocalPos;
  result.uv = vertex.uv;
  result.color = vertex.color;

  return result;
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

}  // namespace

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

}  // namespace raster
