#pragma once

#include <vector>

#include "Color.h"
#include "Vec.h"

namespace raster {

class Texture {
 public:
  Texture(int width, int height, Color fill_color = {0.0, 0.0, 0.0});

  [[nodiscard]] int Width() const;
  [[nodiscard]] int Height() const;

  Color& TexelAt(int x, int y);
  const Color& TexelAt(int x, int y) const;

  [[nodiscard]] Color SampleNearest(Vec2 uv) const;
  [[nodiscard]] Color SampleBilinear(Vec2 uv) const;

 private:
  int width_;
  int height_;
  std::vector<Color> texels_;
};

}  // namespace raster
