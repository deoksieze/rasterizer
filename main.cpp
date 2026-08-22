#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

const int cImageWidth = 512;
const int cImageHeight = 512;

struct Color {
  int r;
  int g;
  int b;
};

namespace fs = std::filesystem;

std::ostream& OpenNextPpm(std::ofstream& output) {
  const fs::path cDirectory = "examples";

  std::size_t file_count = 0;
  for (const auto& entry : fs::directory_iterator(cDirectory)) {
    if (entry.is_regular_file()) {
      ++file_count;
    }
  }

  const fs::path cFilename =
      cDirectory / ("image_" + std::to_string(file_count + 1) + ".ppm");

  output.open(cFilename, std::ios::out | std::ios::binary);

  if (!output) {
    throw std::runtime_error("Не удалось открыть файл: " + cFilename.string());
  }

  return output;
}

void SaveImage(std::ostream& stream, std::vector<std::vector<Color>>& pixels) {
  stream << "P3\n";
  stream << cImageWidth << " " << cImageHeight << "\n";
  stream << "255\n";

  for (int i = 0; i < cImageHeight; i++) {
    for (int j = 0; j < cImageWidth; j++) {
      stream << pixels[i][j].r << " " << pixels[i][j].g << " " << pixels[i][j].b
             << "\n";
    }
  }
}

int main() {
  std::vector<std::vector<Color>> pixels(cImageHeight,
                                         std::vector<Color>(cImageWidth));

  for (int i = 0; i < cImageHeight; i++) {
    for (int j = 0; j < cImageWidth; j++) {
      pixels[i][j].r = 64;
      pixels[i][j].b = 64;
      pixels[i][j].g = 64;
      if (i == j) {
        pixels[i][j].b = 255;
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, pixels);
}