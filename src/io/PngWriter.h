#pragma once

#include <string>

#include "raster/Framebuffer.h"

namespace raster {

void SavePng(const std::string& path, const Framebuffer& buff);

}  // namespace raster