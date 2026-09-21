#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

#include "Texture.h"

namespace {

std::string ReadPpmHeaderToken(std::istream& input) {
  std::string token;

  while (true) {
    input >> std::ws;

    if (input.peek() != '#') {
      break;
    }

    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  if (!(input >> token)) {
    throw std::runtime_error("Unexpected end of PPM header");
  }

  return token;
}

int ReadPpmHeaderInt(std::istream& input, const std::string& field_name) {
  const std::string cToken = ReadPpmHeaderToken(input);

  try {
    std::size_t parsed_characters = 0;
    const int cValue = std::stoi(cToken, &parsed_characters);

    if (parsed_characters != cToken.size()) {
      throw std::runtime_error("Invalid integer");
    }

    return cValue;
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid PPM " + field_name + ": '" + cToken +
                             "'");
  }
}

unsigned int ReadPpmBinarySample(std::istream& input, int max_value) {
  const int cFirstByte = input.get();

  if (cFirstByte == std::char_traits<char>::eof()) {
    throw std::runtime_error("Unexpected end of P6 pixel data");
  }

  if (max_value < 256) {  // NOLINT
    return static_cast<unsigned char>(cFirstByte);
  }

  const int cSecondByte = input.get();

  if (cSecondByte == std::char_traits<char>::eof()) {
    throw std::runtime_error("Unexpected end of P6 pixel data");
  }

  return (static_cast<unsigned int>(static_cast<unsigned char>(cFirstByte))
          << 8U) |  // NOLINT
         static_cast<unsigned char>(cSecondByte);
}

}  // namespace

Texture LoadPpmP6(const std::string& filename) {
  std::ifstream input(filename, std::ios::binary);

  if (!input) {
    throw std::runtime_error("Failed to open PPM file: " + filename);
  }

  const std::string cMagic = ReadPpmHeaderToken(input);

  if (cMagic != "P6") {
    throw std::runtime_error("Expected binary PPM P6, got: " + cMagic);
  }

  const int cWidth = ReadPpmHeaderInt(input, "width");
  const int cHeight = ReadPpmHeaderInt(input, "height");
  const int cMaxValue = ReadPpmHeaderInt(input, "maxval");

  if (cWidth <= 0 || cHeight <= 0) {
    throw std::runtime_error("PPM width and height must be positive");
  }

  if (cMaxValue <= 0 || cMaxValue > 65535) {  // NOLINT
    throw std::runtime_error("PPM maxval must be in range [1, 65535]");
  }

  // После maxval должен быть ровно один whitespace character,
  // который отделяет ASCII header от binary raster.
  const int cSeparator = input.get();

  if (cSeparator == std::char_traits<char>::eof() ||
      (std::isspace(static_cast<unsigned char>(cSeparator)) == 0)) {
    throw std::runtime_error("Expected whitespace after P6 maxval");
  }

  Texture texture(cWidth, cHeight);

  for (int y = 0; y < cHeight; ++y) {
    for (int x = 0; x < cWidth; ++x) {
      const unsigned int cRed = ReadPpmBinarySample(input, cMaxValue);
      const unsigned int cGreen = ReadPpmBinarySample(input, cMaxValue);
      const unsigned int cBlue = ReadPpmBinarySample(input, cMaxValue);

      if (cRed > static_cast<unsigned int>(cMaxValue) ||
          cGreen > static_cast<unsigned int>(cMaxValue) ||
          cBlue > static_cast<unsigned int>(cMaxValue)) {
        throw std::runtime_error("P6 pixel component is outside [0, maxval]");
      }

      texture.TexelAt(x, y) = {
          static_cast<double>(cRed) / cMaxValue,
          static_cast<double>(cGreen) / cMaxValue,
          static_cast<double>(cBlue) / cMaxValue,
      };
    }
  }

  return texture;
}