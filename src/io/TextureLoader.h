#pragma once

#include <string>

#include "core/Texture.h"

namespace raster {

// Dispatches on the file extension: .png or .ppm
Texture LoadTexture(const std::string& path);

}  // namespace raster
