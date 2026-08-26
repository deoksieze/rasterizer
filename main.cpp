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

namespace fs = std::filesystem;
struct Color {
  double r;
  double g;
  double b;

  Color& operator+=(const Color& other) {
    r += other.r;
    g += other.g;
    b += other.b;
    return *this;
  }

  Color& operator*=(double x) {
    r *= x;
    g *= x;
    b *= x;
    return *this;
  }
};

Color operator*(Color c, double x) { return Color{c.r * x, c.g * x, c.b * x}; }

Color operator*(double x, Color color) { return color * x; }

Color operator+(Color c1, Color c2) {
  return Color{c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}
struct Point2D {
  double x;
  double y;
};

Point2D operator-(Point2D a, Point2D b) {
  return Point2D{a.x - b.x, a.y - b.y};
}

struct BarycentricCoordinates {
  double l1;
  double l2;
  double l3;
};

BarycentricCoordinates operator/(BarycentricCoordinates bc, double x) {
  return BarycentricCoordinates{bc.l1 / x, bc.l2 / x, bc.l3 / x};
}

struct Triangle {
  Point2D a;
  Point2D b;
  Point2D c;
};

// Описание констант
const int cImageWidth = 512;
const int cImageHeight = 512;
const double cPixCentOffset = 0.5;
const double cMaxColor = 255.0;

const Point2D cA = {10.0, 500.0};
const Point2D cB = {220.0, 460.0};
const Point2D cC = {400.0, 250.0};

const Color cColorA = {1.0, 0.0, 0.0};
const Color cColorB = {0.0, 1.0, 0.0};
const Color cColorC = {0.0, 0.0, 1.0};
const Color cBackGroundColor = {64.0 / 255.0, 64.0 / 255.0, 64.0 / 255.0};

const Triangle cTriangle = {cA, cB, cC};

// Методы для математики
double Orientation(const Point2D& a, const Point2D& b, const Point2D& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double SignedDoubleArea(const Triangle& tr) {
  return Orientation(tr.a, tr.b, tr.c);
}

double CrossProduct(Point2D a, Point2D b) { return a.x * b.y - a.y * b.x; }

bool IsInside(const BarycentricCoordinates& bc) {
  return bc.l1 >= 0.0 && bc.l2 >= 0.0 && bc.l3 >= 0.0;
}

BarycentricCoordinates GetBarycentricCoordinates(const Triangle& tr,
                                                 const Point2D& p) {
  BarycentricCoordinates coords{};

  double d = CrossProduct(tr.a - tr.b, tr.b - tr.c);
  coords.l1 = CrossProduct(tr.b - p, tr.c - p);
  coords.l2 = CrossProduct(tr.c - p, tr.a - p);
  coords.l3 = CrossProduct(tr.a - p, tr.b - p);

  return (coords / d);
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

void SaveImage(std::ostream& stream, std::vector<std::vector<Color>>& pixels) {
  stream << "P3\n";
  stream << cImageWidth << " " << cImageHeight << "\n";
  stream << "255\n";

  for (int i = 0; i < cImageHeight; i++) {
    for (int j = 0; j < cImageWidth; j++) {
      Color color = pixels[i][j];
      stream << static_cast<int>(ToByte(color.r)) << ' '
             << static_cast<int>(ToByte(color.g)) << ' '
             << static_cast<int>(ToByte(color.b)) << '\n';
    }
  }
}

int main() {
  std::vector<std::vector<Color>> pixels(cImageHeight,
                                         std::vector<Color>(cImageWidth));

  for (int i = 0; i < cImageHeight; i++) {
    for (int j = 0; j < cImageWidth; j++) {
      pixels[i][j] = cBackGroundColor;

      Point2D p = {static_cast<double>(j) + cPixCentOffset,
                   static_cast<double>(i) + cPixCentOffset};

      BarycentricCoordinates bc = GetBarycentricCoordinates(cTriangle, p);

      if (IsInside(bc)) {
        pixels[i][j] = cColorA * bc.l1 + cColorB * bc.l2 + cColorC * bc.l3;
      }
    }
  }

  std::ofstream file;
  std::ostream& out = OpenNextPpm(file);

  SaveImage(out, pixels);
}