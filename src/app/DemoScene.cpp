#include "app/DemoScene.h"

#include <numbers>

#include "core/Matrix.h"

namespace raster {
namespace {

const double cTeapotDistance = -10.0;
const double cTeapotRotationY = std::numbers::pi / 5;
const double cTeapotRotationX = std::numbers::pi / 6;

}  // namespace

Scene MakeDemoScene() {
  Scene scene;

  scene.camera.view = Mat4::MakeUnitMatrix();

  Drawable teapot;
  teapot.mesh_path = "assets/teapot.obj";
  teapot.model = MakeTranslateMatrix(0, 0, cTeapotDistance) *
                 MakeRotateYMatrix(cTeapotRotationY) *
                 MakeRotateXMatrix(cTeapotRotationX);
  scene.objects.push_back(teapot);

  return scene;
}

}  // namespace raster
