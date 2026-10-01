#pragma once

#include <iosfwd>

#include "raster/Framebuffer.h"

namespace raster {

void SavePpmP3(std::ostream& stream, const Framebuffer& buff);

}  // namespace raster
