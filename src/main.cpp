#include <exception>
#include <iostream>

#include "app/DemoScene.h"
#include "app/RenderScene.h"
#include "app/Scene.h"
#include "io/PpmWriter.h"

int main() {
  using namespace raster;  // NOLINT

  try {
    const Scene cScene = MakeDemoScene();

    const Framebuffer cBuffer = RenderScene(cScene);

    SavePpmP3(cScene.output_path, cBuffer);
  } catch (const std::exception& error) {
    std::cerr << "render failed: " << error.what() << '\n';
    return 1;
  }
}
