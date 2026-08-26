#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Framebuffer.h"

namespace fs = std::filesystem;

struct Vec2D {
  double x;
  double y;
};

Vec2D operator-(Vec2D a, Vec2D b) { return Vec2D{a.x - b.x, a.y - b.y}; }

struct BarycentricCoordinates {
  double l1;
  double l2;
  double l3;
};

BarycentricCoordinates operator/(BarycentricCoordinates bc, double x) {
  return BarycentricCoordinates{bc.l1 / x, bc.l2 / x, bc.l3 / x};
}

struct Triangle {
  Vec2D a;
  Vec2D b;
  Vec2D c;
};

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;

const Vec2D cA = {10.0, 500.0};
const Vec2D cB = {220.0, 460.0};
const Vec2D cC = {400.0, 250.0};

const Color cColorA = {1.0, 0.0, 0.0};
const Color cColorB = {0.0, 1.0, 0.0};
const Color cColorC = {0.0, 0.0, 1.0};
const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

const Triangle cTriangle = {cA, cB, cC};

// Методы для математики
double Orientation(const Vec2D& a, const Vec2D& b, const Vec2D& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double SignedDoubleArea(const Triangle& tr) {
  return Orientation(tr.a, tr.b, tr.c);
}

double CrossProduct(Vec2D a, Vec2D b) { return a.x * b.y - a.y * b.x; }

bool IsInside(const BarycentricCoordinates& bc) {
  return bc.l1 >= 0.0 && bc.l2 >= 0.0 && bc.l3 >= 0.0;
}

BarycentricCoordinates GetBarycentricCoordinates(const Triangle& tr,
                                                 const Vec2D& p) {
  const double cArea2 = Orientation(tr.a, tr.b, tr.c);

  return {
      Orientation(tr.b, tr.c, p) / cArea2,
      Orientation(tr.c, tr.a, p) / cArea2,
      Orientation(tr.a, tr.b, p) / cArea2,
  };
}

// Методы для работы с Системой

std::uint8_t ToByte(double value) {
  value = std::clamp(value, 0.0, 1.0);
  return static_cast<std::uint8_t>(std::lround(value * cMaxColor));
}

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

void SaveImage(std::ostream& stream, const Framebuffer& buff) {
  stream << "P3\n";
  stream << buff.Width() << " " << buff.Height() << "\n";
  stream << "255\n";

  for (int y = 0; y < buff.Height(); y++) {
    for (int x = 0; x < buff.Width(); x++) {
      Color color = buff.At(x, y);
      stream << static_cast<int>(ToByte(color.r)) << ' '
             << static_cast<int>(ToByte(color.g)) << ' '
             << static_cast<int>(ToByte(color.b)) << '\n';
    }
  }
}

int main() {
  Framebuffer buffer = Framebuffer(cImageWidth, cImageHeight, cBackGroundColor);
  // std::vector<std::vector<Color>> pixels(cImageHeight,
  //                                        std::vector<Color>(cImageWidth));

  for (int y = 0; y < buffer.Height(); y++) {
    for (int x = 0; x < buffer.Width(); x++) {
      buffer.At(x, y) = cBackGroundColor;

      Vec2D p = {static_cast<double>(x) + cPixCentOffset,
                 static_cast<double>(y) + cPixCentOffset};

      BarycentricCoordinates bc = GetBarycentricCoordinates(cTriangle, p);

      if (IsInside(bc)) {
        buffer.At(x, y) = cColorA * bc.l1 + cColorB * bc.l2 + cColorC * bc.l3;
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, buffer);
}