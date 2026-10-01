#include <exception>
#include <iostream>
#include <string>

#include "app/RenderScene.h"
#include "app/Scene.h"
#include "app/SceneLoader.h"
#include "io/ImageWriter.h"

int main(int argc, char** argv) {
  using namespace raster;  // NOLINT

  try {
    const std::string cScenePath =
        (argc > 1) ? std::string{argv[1]} : std::string{"assets/scene.json"};

    const Scene cScene = LoadSceneFromJson(cScenePath);

    const Framebuffer cBuffer = RenderScene(cScene);

    SaveImage(cScene.output_path, cBuffer);

    std::cout << "rendered " << cScenePath << " -> " << cScene.output_path
              << '\n';
  } catch (const std::exception& error) {
    std::cerr << "render failed: " << error.what() << '\n';
    return 1;
  }
}
