#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

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

struct Triangle {
  Vec2D a;
  Vec2D b;
  Vec2D c;
};

struct BoundingBox {
  int min_x;
  int max_x;
  int min_y;
  int max_y;
};

BoundingBox FindBoundingBox(const Triangle& tr, const Framebuffer& buffer) {
  const int cMinX = std::max(
      0, static_cast<int>(std::floor(std::min({tr.a.x, tr.b.x, tr.c.x}))));

  const int cMaxX =
      std::min(buffer.Width(),
               static_cast<int>(std::ceil(std::max({tr.a.x, tr.b.x, tr.c.x}))));

  const int cMinY = std::max(
      0, static_cast<int>(std::floor(std::min({tr.a.y, tr.b.y, tr.c.y}))));

  const int cMaxY =
      std::min(buffer.Height(),
               static_cast<int>(std::ceil(std::max({tr.a.y, tr.b.y, tr.c.y}))));

  return BoundingBox{cMinX, cMaxX, cMinY, cMaxY};
}

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;

const Vec2D cA1 = {80.0, 80.0};
const Vec2D cB1 = {250.0, 120.0};
const Vec2D cC1 = {150.0, 300.0};

const Triangle cTriangle1 = {cA1, cB1, cC1};

const Vec2D cA2 = {380.0, 350.0};
const Vec2D cB2 = {620.0, 430.0};
const Vec2D cC2 = {450.0, 650.0};

const Triangle cTriangle2 = {cA2, cB2, cC2};

const Vec2D cA3 = {-300.0, 100.0};
const Vec2D cB3 = {-80.0, 180.0};
const Vec2D cC3 = {-120.0, 400.0};

const Triangle cTriangle3 = {cA3, cB3, cC3};

const Vec2D cA4 = {350.2, 120.4};
const Vec2D cB4 = {400.8, 180.6};
const Vec2D cC4 = {370.5, 300.3};

const Triangle cTriangle4 = {cA4, cB4, cC4};

const Color cColorA = {1.0, 0.0, 0.0};
const Color cColorB = {0.0, 1.0, 0.0};
const Color cColorC = {0.0, 0.0, 1.0};
const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

// Методы для математики
double Orientation(const Vec2D& a, const Vec2D& b, const Vec2D& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

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
  std::vector<Triangle> triangles{cTriangle1, cTriangle2, cTriangle3,
                                  cTriangle4};
  Framebuffer buffer = Framebuffer(cImageWidth, cImageHeight, cBackGroundColor);

  // Ищем bounding box
  for (const auto& tr : triangles) {
    BoundingBox box = FindBoundingBox(tr, buffer);

    if (box.max_x <= box.min_x || box.max_y <= box.min_y) {
      continue;
    }

    for (int y = box.min_y; y < box.max_y; y++) {
      for (int x = box.min_x; x < box.max_x; x++) {
        Vec2D p = {static_cast<double>(x) + cPixCentOffset,
                   static_cast<double>(y) + cPixCentOffset};

        BarycentricCoordinates bc = GetBarycentricCoordinates(tr, p);

        if (IsInside(bc)) {
          buffer.At(x, y) = cColorA * bc.l1 + cColorB * bc.l2 + cColorC * bc.l3;
        }
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, buffer);
}