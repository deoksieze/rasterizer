#include "io/PpmWriter.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>

namespace raster {
namespace {

void WritePpmP3(std::ostream& stream, const Framebuffer& buff) {
  stream << "P3\n";
  stream << buff.Width() << " " << buff.Height() << "\n";
  stream << "255\n";

  for (int y = 0; y < buff.Height(); y++) {
    for (int x = 0; x < buff.Width(); x++) {
      const Color cColor = buff.At(x, y);
      stream << static_cast<int>(ToByte(cColor.r)) << ' '
             << static_cast<int>(ToByte(cColor.g)) << ' '
             << static_cast<int>(ToByte(cColor.b)) << '\n';
    }
  }
}

}  // namespace

void SavePpmP3(const std::string& path, const Framebuffer& buff) {
  const std::filesystem::path cPath(path);

  if (cPath.has_parent_path()) {
    std::filesystem::create_directories(cPath.parent_path());
  }

  std::ofstream output(cPath, std::ios::out | std::ios::binary);

  if (!output) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }

  WritePpmP3(output, buff);
}

}  // namespace raster