#include "io/TextureLoader.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#include "io/PngLoader.h"
#include "io/PpmLoader.h"

namespace raster {

Texture LoadTexture(const std::string& path) {
  const std::string cExtension =
      std::filesystem::path(path).extension().string();

  if (cExtension == ".png") {
    return LoadTextureFromPng(path);
  }

  if (cExtension == ".ppm") {
    return LoadPpmP6(path);
  }

  throw std::invalid_argument("Unsupported texture extension '" + cExtension +
                              "', expected .png or .ppm");
}

}  // namespace raster
