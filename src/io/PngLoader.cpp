#include "io/PngLoader.h"

#include <png.h>

#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <istream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace raster {
namespace {

const int cChannels = 3;
const int cBitDepth = 8;
const int cWideBitDepth = 16;
const int cMaxComponent = 255;

struct DecodedImage {
  int width;
  int height;
  std::vector<std::uint8_t> pixels;
};

void OnPngError(png_structp png_ptr, png_const_charp message) {
  auto* error_text = static_cast<std::string*>(png_get_error_ptr(png_ptr));

  if (error_text != nullptr) {
    *error_text = (message != nullptr) ? message : "unknown libpng error";
  }

  png_longjmp(png_ptr, 1);
}

void OnPngWarning(png_structp /* png_ptr */, png_const_charp /* message */) {}

void ReadFromStream(png_structp png_ptr, png_bytep data, png_size_t length) {
  auto* stream = static_cast<std::istream*>(png_get_io_ptr(png_ptr));

  if (stream == nullptr) {
    png_error(png_ptr, "PNG input stream is not set");
    return;
  }

  stream->read(reinterpret_cast<char*>(data),
               static_cast<std::streamsize>(length));

  if (!stream->good()) {
    png_error(png_ptr, "unexpected end of PNG file");
  }
}

class PngHandle {
 public:
  explicit PngHandle(std::string* error_text)
      : png_ptr_(png_create_read_struct(PNG_LIBPNG_VER_STRING, error_text,
                                        OnPngError, OnPngWarning)),
        info_ptr_(png_ptr_ != nullptr ? png_create_info_struct(png_ptr_)
                                      : nullptr) {}

  ~PngHandle() {
    if (png_ptr_ != nullptr) {
      png_destroy_read_struct(
          &png_ptr_, info_ptr_ != nullptr ? &info_ptr_ : nullptr, nullptr);
    }
  }

  PngHandle(const PngHandle&) = delete;
  PngHandle& operator=(const PngHandle&) = delete;

  [[nodiscard]] bool Valid() const {
    return png_ptr_ != nullptr && info_ptr_ != nullptr;
  }

  [[nodiscard]] png_structp Png() const { return png_ptr_; }
  [[nodiscard]] png_infop Info() const { return info_ptr_; }

 private:
  png_structp png_ptr_;
  png_infop info_ptr_;
};

std::string DescribeError(const std::string& fallback,
                          const std::string& libpng_error) {
  return libpng_error.empty() ? fallback : libpng_error;
}

// Everything libpng reports that we cannot turn into an opaque 8-bit RGB
// image becomes an alpha channel or a wide sample that has to be reduced.
void NormalizeToRgb8(png_structp png_ptr, png_infop info_ptr) {
  if (png_get_bit_depth(png_ptr, info_ptr) == cWideBitDepth) {
    png_set_strip_16(png_ptr);
  }

  png_set_expand(png_ptr);  // palette -> rgb, gray < 8 -> 8, tRNS -> alpha
  png_set_gray_to_rgb(png_ptr);
  png_set_strip_alpha(png_ptr);  // alpha is dropped, RGB stays as authored
}

std::size_t CheckedImageSize(png_uint_32 width, png_uint_32 height) {
  if (width == 0 || height == 0) {
    throw std::runtime_error("PNG width and height must be positive");
  }

  if (width > static_cast<png_uint_32>(std::numeric_limits<int>::max()) ||
      height > static_cast<png_uint_32>(std::numeric_limits<int>::max())) {
    throw std::runtime_error("PNG is too large to fit into a Texture");
  }

  return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

DecodedImage DecodePng(std::istream& input, const std::string& filename) {
  std::string png_error;
  PngHandle png_handle(&png_error);

  if (!png_handle.Valid()) {
    throw std::runtime_error(
        "Failed to create PNG reader for " + filename + ": " +
        DescribeError("libpng could not be initialised", png_error));
  }

  if (setjmp(png_jmpbuf(png_handle.Png())) != 0) {
    throw std::runtime_error("Failed to read PNG " + filename + ": " +
                             DescribeError("unknown libpng error", png_error));
  }

  png_set_read_fn(png_handle.Png(), &input, ReadFromStream);
  png_read_info(png_handle.Png(), png_handle.Info());

  png_uint_32 png_width = 0;
  png_uint_32 png_height = 0;
  int bit_depth = 0;
  int color_type = 0;

  if (png_get_IHDR(png_handle.Png(), png_handle.Info(), &png_width, &png_height,
                   &bit_depth, &color_type, nullptr, nullptr, nullptr) == 0) {
    throw std::runtime_error("PNG has no readable header chunk");
  }

  const std::size_t cPixelCount = CheckedImageSize(png_width, png_height);

  NormalizeToRgb8(png_handle.Png(), png_handle.Info());
  png_set_interlace_handling(png_handle.Png());
  png_read_update_info(png_handle.Png(), png_handle.Info());

  if (png_get_bit_depth(png_handle.Png(), png_handle.Info()) != cBitDepth ||
      png_get_channels(png_handle.Png(), png_handle.Info()) != cChannels) {
    throw std::runtime_error("Unsupported PNG format, expected 8-bit RGB");
  }

  const std::size_t cRowBytes = static_cast<std::size_t>(
      png_get_rowbytes(png_handle.Png(), png_handle.Info()));

  if (cRowBytes != static_cast<std::size_t>(png_width) * cChannels) {
    throw std::runtime_error("Unexpected PNG row stride after conversion");
  }

  DecodedImage decoded{static_cast<int>(png_width),
                       static_cast<int>(png_height),
                       std::vector<std::uint8_t>(cPixelCount * cChannels, 0)};

  std::vector<png_bytep> rows(static_cast<std::size_t>(decoded.height));

  for (int y = 0; y < decoded.height; ++y) {
    rows[static_cast<std::size_t>(y)] =
        decoded.pixels.data() + static_cast<std::size_t>(y) * cRowBytes;
  }

  png_read_image(png_handle.Png(), rows.data());
  png_read_end(png_handle.Png(), nullptr);

  return decoded;
}

}  // namespace

Texture LoadTextureFromPng(const std::string& filename) {
  std::ifstream input(filename, std::ios::in | std::ios::binary);

  if (!input) {
    throw std::runtime_error("Failed to open PNG file: " + filename);
  }

  const DecodedImage cDecoded = DecodePng(input, filename);

  Texture texture(cDecoded.width, cDecoded.height);

  for (int y = 0; y < cDecoded.height; ++y) {
    for (int x = 0; x < cDecoded.width; ++x) {
      const std::size_t cIndex = (static_cast<std::size_t>(y) *
                                      static_cast<std::size_t>(cDecoded.width) +
                                  static_cast<std::size_t>(x)) *
                                 cChannels;

      texture.TexelAt(x, y) = {
          static_cast<double>(cDecoded.pixels[cIndex]) / cMaxComponent,
          static_cast<double>(cDecoded.pixels[cIndex + 1]) / cMaxComponent,
          static_cast<double>(cDecoded.pixels[cIndex + 2]) / cMaxComponent,
      };
    }
  }

  return texture;
}

}  // namespace raster
