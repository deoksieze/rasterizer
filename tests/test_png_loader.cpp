#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "Check.h"
#include "core/Color.h"
#include "io/PngLoader.h"

using namespace raster;  // NOLINT

namespace {

const double cEpsilon = 1e-12;

// The interlaced fixture is 8x8 with pixel (x, y) = (x * step, y * step, blue).
const int cGrid = 8;
const double cStep = 32.0;
const double cBlue = 128.0;
const double cByteScale = 255.0;
const double cGridEpsilon = 1e-6;

const Color cRed{1.0, 0.0, 0.0};
const Color cGreen{0.0, 1.0, 0.0};
const Color cBlueColor{0.0, 0.0, 1.0};
const Color cWhite{1.0, 1.0, 1.0};
const Color cBlack{0.0, 0.0, 0.0};

const std::array<Color, 4> cColourPattern{cRed, cGreen, cBlueColor, cWhite};
const std::array<Color, 4> cGrayPattern{cWhite, cBlack, cBlack, cWhite};

// Every PNG flavour below holds the same 2x2 image: red green / blue white.
void CheckColourPattern(const Texture& texture,
                        const std::array<Color, 4>& expected) {
  CHECK(texture.Width() == 2);
  CHECK(texture.Height() == 2);

  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      const Color cActual = texture.TexelAt(x, y);
      const Color cExpected = expected[static_cast<std::size_t>(y * 2 + x)];

      CHECK_NEAR(cActual.r, cExpected.r, cEpsilon);
      CHECK_NEAR(cActual.g, cExpected.g, cEpsilon);
      CHECK_NEAR(cActual.b, cExpected.b, cEpsilon);
    }
  }
}

void CheckInterlacedGrid(const Texture& texture) {
  CHECK(texture.Width() == cGrid);
  CHECK(texture.Height() == cGrid);

  for (int y = 0; y < cGrid; ++y) {
    for (int x = 0; x < cGrid; ++x) {
      const Color cActual = texture.TexelAt(x, y);

      CHECK_NEAR(cActual.r, x * cStep / cByteScale, cGridEpsilon);
      CHECK_NEAR(cActual.g, y * cStep / cByteScale, cGridEpsilon);
      CHECK_NEAR(cActual.b, cBlue / cByteScale, cGridEpsilon);
    }
  }
}

bool ThrowsRuntimeError(const std::string& path) {
  try {
    LoadTextureFromPng(path);
  } catch (const std::runtime_error&) {
    return true;
  }

  return false;
}

}  // namespace

int main() {
  {
    const Texture cTexture = LoadTextureFromPng("tests/fixtures/png_rgb8.png");

    CheckColourPattern(cTexture, cColourPattern);
  }

  {
    // Alpha is dropped, the colour underneath is kept as authored.
    const Texture cTexture = LoadTextureFromPng("tests/fixtures/png_rgba8.png");

    CheckColourPattern(cTexture, cColourPattern);
  }

  {
    // A grayscale pixel expands to r = g = b.
    const Texture cTexture = LoadTextureFromPng("tests/fixtures/png_gray8.png");

    CheckColourPattern(cTexture, cGrayPattern);
  }

  {
    // Palette + tRNS goes through palette_to_rgb and strip_alpha.
    const Texture cTexture =
        LoadTextureFromPng("tests/fixtures/png_palette.png");

    CheckColourPattern(cTexture, cColourPattern);
  }

  {
    // Indices narrower than a byte have to be unpacked first.
    const Texture cTexture =
        LoadTextureFromPng("tests/fixtures/png_palette4.png");

    CheckColourPattern(cTexture, cColourPattern);
  }

  {
    const Texture cTexture = LoadTextureFromPng("tests/fixtures/png_gray1.png");

    CheckColourPattern(cTexture, cGrayPattern);
  }

  {
    // 16 bits per channel are reduced to the most significant 8.
    const Texture cTexture = LoadTextureFromPng("tests/fixtures/png_rgb16.png");

    CheckColourPattern(cTexture, cColourPattern);
  }

  {
    CheckInterlacedGrid(
        LoadTextureFromPng("tests/fixtures/png_rgb8_interlaced.png"));
  }

  { CHECK(ThrowsRuntimeError("tests/fixtures/definitely_missing.png")); }

  {
    // A file that is not a PNG at all must not read past the header.
    CHECK(ThrowsRuntimeError("tests/fixtures/golden.json"));
  }

  return Summary("png_loader");
}
