#include "app/RenderScene.h"

#include <numbers>
#include <vector>

#include "core/Matrix.h"
#include "geometry/Mesh.h"
#include "io/ObjLoader.h"
#include "raster/Clipper.h"
#include "raster/Projector.h"
#include "raster/Rasterizer.h"

namespace raster {

Framebuffer RenderScene(const Scene& scene) {
  Framebuffer buffer(scene.width, scene.height, scene.background);

  const double cAspect =
      static_cast<double>(scene.width) / static_cast<double>(scene.height);

  const Mat4 cProjection = MakePerspectiveMatrix(
      scene.camera.fov_y_degrees * std::numbers::pi / 180.0, cAspect,
      scene.camera.near_plane, scene.camera.far_plane);

  std::vector<ClipTriangle> clip_triangles;
  std::vector<ClipTriangle> clipped_triangles;
  std::vector<ScreenTriangle> screen_triangles;

  for (const Drawable& object : scene.objects) {
    const Mesh cMesh = LoadObj(object.mesh_path);

    TransformMeshToClipTriangles(cMesh, object.model, scene.camera.view,
                                 cProjection, clip_triangles);
    ClipTriangles(clip_triangles, clipped_triangles);

    screen_triangles.clear();
    ProjectClippedTrianglesToScreen(clipped_triangles, screen_triangles,
                                    buffer);

    RasterizeTriangles(buffer, screen_triangles, scene.cull_back_faces);
  }

  return buffer;
}

}  // namespace raster
