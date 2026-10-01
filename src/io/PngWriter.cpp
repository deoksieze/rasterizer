#include "io/PngWriter.h"

#include <png.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace raster {
namespace {

const int cChannels = 3;
const int cBitDepth = 8;

void OnPngError(png_structp png_ptr, png_const_charp message) {
  auto* error_text = static_cast<std::string*>(png_get_error_ptr(png_ptr));

  if (error_text != nullptr) {
    *error_text = (message != nullptr) ? message : "unknown libpng error";
  }

  png_longjmp(png_ptr, 1);
}

void OnPngWarning(png_structp /* png_ptr */, png_const_charp /* message */) {}

void WriteToStream(png_structp png_ptr, png_bytep data, png_size_t length) {
  auto* stream = static_cast<std::ostream*>(png_get_io_ptr(png_ptr));

  if (stream == nullptr) {
    return;
  }

  stream->write(reinterpret_cast<const char*>(data),
                static_cast<std::streamsize>(length));
}

void FlushStream(png_structp png_ptr) {
  auto* stream = static_cast<std::ostream*>(png_get_io_ptr(png_ptr));

  if (stream == nullptr) {
    return;
  }

  stream->flush();
}

class PngHandle {
 public:
  explicit PngHandle(std::string* error_text)
      : png_ptr_(png_create_write_struct(PNG_LIBPNG_VER_STRING, error_text,
                                         OnPngError, OnPngWarning)),
        info_ptr_(png_ptr_ != nullptr ? png_create_info_struct(png_ptr_)
                                      : nullptr) {}

  ~PngHandle() {
    if (png_ptr_ != nullptr) {
      png_destroy_write_struct(&png_ptr_,
                               info_ptr_ != nullptr ? &info_ptr_ : nullptr);
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

}  // namespace

void SavePng(const std::string& path, const Framebuffer& buff) {
  const std::filesystem::path cPath(path);

  if (cPath.has_parent_path()) {
    std::filesystem::create_directories(cPath.parent_path());
  }

  std::ofstream output(cPath, std::ios::out | std::ios::binary);

  if (!output) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }

  const int cWidth = buff.Width();
  const int cHeight = buff.Height();

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(cWidth) *
                                   static_cast<std::size_t>(cHeight) *
                                   cChannels);

  for (int y = 0; y < cHeight; y++) {
    for (int x = 0; x < cWidth; x++) {
      const Color cColor = buff.At(x, y);
      const std::size_t cIndex =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(cWidth) +
           static_cast<std::size_t>(x)) *
          cChannels;

      pixels[cIndex] = ToByte(cColor.r);
      pixels[cIndex + 1] = ToByte(cColor.g);
      pixels[cIndex + 2] = ToByte(cColor.b);
    }
  }

  std::vector<png_bytep> rows(static_cast<std::size_t>(cHeight));
  for (int y = 0; y < cHeight; y++) {
    rows[y] = pixels.data() + static_cast<std::size_t>(y) *
                                  static_cast<std::size_t>(cWidth) * cChannels;
  }

  std::string png_error;
  PngHandle png_handle(&png_error);

  if (!png_handle.Valid()) {
    throw std::runtime_error(
        "Failed to create PNG writer for " + path + ": " +
        DescribeError("libpng could not be initialised", png_error));
  }

  if (setjmp(png_jmpbuf(png_handle.Png())) != 0) {
    throw std::runtime_error("Failed to write PNG " + path + ": " +
                             DescribeError("unknown libpng error", png_error));
  }

  png_set_write_fn(png_handle.Png(), &output, WriteToStream, FlushStream);

  png_set_IHDR(png_handle.Png(), png_handle.Info(),
               static_cast<png_uint_32>(cWidth),
               static_cast<png_uint_32>(cHeight), cBitDepth, PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_handle.Png(), png_handle.Info());
  png_write_image(png_handle.Png(), rows.data());
  png_write_end(png_handle.Png(), nullptr);

  output.flush();

  if (!output) {
    throw std::runtime_error("Failed to write PNG file: " + path);
  }
}

}  // namespace raster