#include "io/ImageWriter.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#include "io/PngWriter.h"
#include "io/PpmWriter.h"

namespace raster {

void SaveImage(const std::string& path, const Framebuffer& buff) {
  const std::string cExtension =
      std::filesystem::path(path).extension().string();

  if (cExtension == ".png") {
    SavePng(path, buff);
    return;
  }

  if (cExtension == ".ppm") {
    SavePpmP3(path, buff);
    return;
  }

  throw std::invalid_argument("Unsupported image extension '" + cExtension +
                              "', expected .png or .ppm");
}

}  // namespace raster