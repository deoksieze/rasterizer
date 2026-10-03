#pragma once

#include <string>

#include "core/Texture.h"

namespace raster {

// Loads any PNG flavour (palette, grayscale, with or without alpha, 1-16 bit,
// interlaced) and normalizes it into an opaque 8-bit RGB Texture.
Texture LoadTextureFromPng(const std::string& filename);

}  // namespace raster
