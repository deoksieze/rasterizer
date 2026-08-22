#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Color {
  int r;
  int g;
  int b;
};

struct Point2D {
  double x;
  double y;
};

struct Triangle {
  Point2D a;
  Point2D b;
  Point2D c;
};

const int cImageWidth = 512;
const int cImageHeight = 512;

const Point2D cA = {10.0, 500.0};
const Point2D cB = {220.0, 460.0};
const Point2D cC = {400.0, 250.0};
const Triangle cTriangle = {cA, cB, cC};

double Orientation(const Point2D& a, const Point2D& b, const Point2D& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool IsInsideTriangle(
    const Triangle& triangle,
    const Point2D& p) {  // Вот не совсем понятно в каком порядке это делать

  return (Orientation(triangle.a, triangle.b, p) <= 0) &&
         (Orientation(triangle.b, triangle.c, p) <= 0) &&
         (Orientation(triangle.c, triangle.a, p) <= 0);
}

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

      Point2D p = {(double)j, (double)i};
      if (IsInsideTriangle(cTriangle, p)) {
        pixels[i][j].b = 255;
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, pixels);
}