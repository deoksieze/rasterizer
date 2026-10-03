#include "app/RenderScene.h"

#include <numbers>
#include <optional>
#include <vector>

#include "core/Matrix.h"
#include "core/Texture.h"
#include "geometry/Mesh.h"
#include "geometry/UvProjection.h"
#include "io/ObjLoader.h"
#include "io/TextureLoader.h"
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
    Mesh mesh = LoadObj(object.mesh_path);

    if (!mesh.has_texcoords) {
      ProjectSphericalUvs(mesh);
    }

    std::optional<Texture> texture;
    if (!object.texture_path.empty()) {
      texture = LoadTexture(object.texture_path);
    }

    TransformMeshToClipTriangles(mesh, object.model, scene.camera.view,
                                 cProjection, clip_triangles);
    ClipTriangles(clip_triangles, clipped_triangles);

    screen_triangles.clear();
    ProjectClippedTrianglesToScreen(clipped_triangles, screen_triangles,
                                    buffer);

    RasterizeTriangles(buffer, screen_triangles, scene.cull_back_faces,
                       texture ? &*texture : nullptr);
  }

  return buffer;
}

}  // namespace raster
