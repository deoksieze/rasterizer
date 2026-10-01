#pragma once

#include <iosfwd>
#include <string>

#include "geometry/Mesh.h"

namespace raster {

Mesh LoadObj(std::istream& input);
Mesh LoadObj(const std::string& filename);

}  // namespace raster
