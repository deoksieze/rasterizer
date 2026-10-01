#pragma once

#include <iosfwd>

#include "geometry/Mesh.h"

namespace raster {

Mesh LoadObj(std::istream& input);

}  // namespace raster
