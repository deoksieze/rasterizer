#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "Check.h"
#include "app/RenderScene.h"
#include "app/SceneLoader.h"
#include "io/ImageWriter.h"

using namespace raster;  // NOLINT

namespace {

const char* const cScenePath = "tests/fixtures/golden.json";
const char* const cGoldenPath = "tests/golden/scene_128.ppm";

std::string ReadFile(const std::string& path) {
  std::ifstream input(path, std::ios::binary);

  if (!input) {
    return {};
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

}  // namespace

int main(int argc, char** argv) {
  const Scene cScene = LoadSceneFromJson(cScenePath);
  const Framebuffer cBuffer = RenderScene(cScene);

  SaveImage(cScene.output_path, cBuffer);

  if (argc > 1 && std::string{argv[1]} == "--update") {
    const std::filesystem::path cGolden(cGoldenPath);
    std::filesystem::create_directories(cGolden.parent_path());
    std::filesystem::copy_file(
        cScene.output_path, cGolden,
        std::filesystem::copy_options::overwrite_existing);
    std::cout << "updated " << cGoldenPath << '\n';
    return EXIT_SUCCESS;
  }

  const std::string cExpected = ReadFile(cGoldenPath);

  if (cExpected.empty()) {
    std::cerr << "missing golden image " << cGoldenPath
              << " (regenerate with --update)\n";
    return EXIT_FAILURE;
  }

  CHECK(ReadFile(cScene.output_path) == cExpected);

  return Summary("golden_image");
}
