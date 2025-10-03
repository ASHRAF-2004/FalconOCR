#include "falcon/core/Image.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace falcon::core {

namespace {

uint16_t ReadLE16(const std::vector<uint8_t>& data, std::size_t offset) {
  return static_cast<uint16_t>(data[offset] | (static_cast<uint16_t>(data[offset + 1]) << 8));
}

uint32_t ReadLE32(const std::vector<uint8_t>& data, std::size_t offset) {
  return static_cast<uint32_t>(data[offset] | (static_cast<uint32_t>(data[offset + 1]) << 8) |
                               (static_cast<uint32_t>(data[offset + 2]) << 16) |
                               (static_cast<uint32_t>(data[offset + 3]) << 24));
}

Raster LoadBmpFromMemory(const std::vector<uint8_t>& data) {
  if (data.size() < 54) {
    throw std::runtime_error("BMP file too small");
  }

  const uint16_t signature = ReadLE16(data, 0);
  if (signature != 0x4D42) {
    throw std::runtime_error("Not a BMP file");
  }

  const uint32_t pixel_offset = ReadLE32(data, 10);
  const uint32_t dib_header_size = ReadLE32(data, 14);
  if (dib_header_size < 40) {
    throw std::runtime_error("Unsupported BMP DIB header");
  }

  const int32_t width = static_cast<int32_t>(ReadLE32(data, 18));
  const int32_t height = static_cast<int32_t>(ReadLE32(data, 22));
  const uint16_t planes = ReadLE16(data, 26);
  const uint16_t bpp = ReadLE16(data, 28);
  const uint32_t compression = ReadLE32(data, 30);

  if (planes != 1 || (bpp != 24 && bpp != 8) || compression != 0) {
    throw std::runtime_error("Unsupported BMP format (expecting 24-bit or 8-bit uncompressed)");
  }

  Raster raster;
  raster.width = width;
  raster.height = std::abs(height);
  raster.dpi_x = static_cast<int>(ReadLE32(data, 38) * 0.0254);
  raster.dpi_y = static_cast<int>(ReadLE32(data, 42) * 0.0254);
  raster.pixels.resize(static_cast<std::size_t>(raster.width) * raster.height);

  const bool bottom_up = height > 0;
  const int row_stride = ((bpp * width + 31) / 32) * 4;

  for (int row = 0; row < raster.height; ++row) {
    const int src_row = bottom_up ? (raster.height - 1 - row) : row;
    const std::size_t row_start = static_cast<std::size_t>(pixel_offset) + static_cast<std::size_t>(src_row) * row_stride;
    for (int col = 0; col < raster.width; ++col) {
      if (bpp == 24) {
        const std::size_t idx = row_start + static_cast<std::size_t>(col) * 3;
        const uint8_t b = data[idx];
        const uint8_t g = data[idx + 1];
        const uint8_t r = data[idx + 2];
        const uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
        raster.pixels[static_cast<std::size_t>(row) * raster.width + col] = gray;
      } else {  // 8-bit indexed
        const std::size_t idx = row_start + static_cast<std::size_t>(col);
        const uint8_t value = data[idx];
        raster.pixels[static_cast<std::size_t>(row) * raster.width + col] = value;
      }
    }
  }

  return raster;
}

std::string ReadNextToken(std::istream& stream) {
  std::string token;
  char ch;
  while (stream.get(ch)) {
    if (ch == '#') {
      std::string dummy;
      std::getline(stream, dummy);
      continue;
    }
    if (!std::isspace(static_cast<unsigned char>(ch))) {
      token.push_back(ch);
      break;
    }
  }

  while (stream.get(ch)) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      break;
    }
    token.push_back(ch);
  }
  return token;
}

Raster LoadAsciiPnm(std::istream& stream, int width, int height, int max_value, bool is_color) {
  Raster raster;
  raster.width = width;
  raster.height = height;
  raster.pixels.resize(static_cast<std::size_t>(width) * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (is_color) {
        const int r = std::stoi(ReadNextToken(stream));
        const int g = std::stoi(ReadNextToken(stream));
        const int b = std::stoi(ReadNextToken(stream));
        const float gray = 0.299f * static_cast<float>(r) + 0.587f * static_cast<float>(g) +
                           0.114f * static_cast<float>(b);
        raster.pixels[static_cast<std::size_t>(y) * width + x] =
            static_cast<uint8_t>(std::clamp(gray / static_cast<float>(max_value), 0.0f, 1.0f) * 255.0f + 0.5f);
      } else {
        const int v = std::stoi(ReadNextToken(stream));
        raster.pixels[static_cast<std::size_t>(y) * width + x] =
            static_cast<uint8_t>(std::clamp(static_cast<float>(v) / static_cast<float>(max_value), 0.0f, 1.0f) * 255.0f +
                                 0.5f);
      }
    }
  }

  return raster;
}

Raster LoadBinaryPnm(std::istream& stream, int width, int height, int max_value, bool is_color) {
  Raster raster;
  raster.width = width;
  raster.height = height;
  raster.pixels.resize(static_cast<std::size_t>(width) * height);

  const auto read_byte = [&stream]() -> uint8_t {
    const int value = stream.get();
    if (value == EOF) {
      throw std::runtime_error("Unexpected EOF while reading PNM file");
    }
    return static_cast<uint8_t>(value);
  };

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (is_color) {
        const uint8_t r = read_byte();
        const uint8_t g = read_byte();
        const uint8_t b = read_byte();
        const float gray = 0.299f * static_cast<float>(r) + 0.587f * static_cast<float>(g) +
                           0.114f * static_cast<float>(b);
        raster.pixels[static_cast<std::size_t>(y) * width + x] =
            static_cast<uint8_t>(std::clamp(gray / static_cast<float>(max_value), 0.0f, 1.0f) * 255.0f + 0.5f);
      } else {
        const uint8_t v = read_byte();
        raster.pixels[static_cast<std::size_t>(y) * width + x] =
            static_cast<uint8_t>(std::clamp(static_cast<float>(v) / static_cast<float>(max_value), 0.0f, 1.0f) * 255.0f +
                                 0.5f);
      }
    }
  }

  return raster;
}

Raster LoadPnmStream(std::istream& stream) {
  const std::string magic = ReadNextToken(stream);
  if (magic.size() != 2 || magic[0] != 'P') {
    throw std::runtime_error("Invalid PNM magic");
  }

  const int variant = magic[1] - '0';
  if (variant < 2 || variant > 6 || variant == 1) {
    throw std::runtime_error("Unsupported PNM variant");
  }

  const int width = std::stoi(ReadNextToken(stream));
  const int height = std::stoi(ReadNextToken(stream));
  const int max_value = std::stoi(ReadNextToken(stream));
  const bool is_binary = (variant == 5 || variant == 6);
  const bool is_color = (variant == 3 || variant == 6);

  if (max_value <= 0 || max_value > 65535) {
    throw std::runtime_error("Unsupported PNM max value");
  }

  if (is_binary) {
    stream.get();  // consume single whitespace
  }

  if (is_binary) {
    return LoadBinaryPnm(stream, width, height, max_value, is_color);
  }

  return LoadAsciiPnm(stream, width, height, max_value, is_color);
}

}  // namespace

Raster LoadImage(const std::filesystem::path& path) {
  const auto ext = path.extension().string();
  if (ext == ".bmp" || ext == ".BMP") {
    return LoadBmp(path);
  }
  if (ext == ".pgm" || ext == ".PGM" || ext == ".ppm" || ext == ".PPM" || ext == ".pnm" || ext == ".PNM") {
    return LoadPnm(path);
  }
  throw std::runtime_error("Unsupported image extension");
}

Raster LoadBmp(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open BMP file: " + path.string());
  }
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return LoadBmpFromMemory(data);
}

Raster LoadPnm(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open PNM file: " + path.string());
  }
  return LoadPnmStream(file);
}

Raster ConvertToGrayscale(const Raster& src) {
  // Already stored as grayscale. Return copy to keep API symmetrical.
  return src;
}

Raster ResizeNearest(const Raster& src, int new_width, int new_height) {
  if (new_width <= 0 || new_height <= 0) {
    throw std::invalid_argument("ResizeNearest expects positive dimensions");
  }

  Raster out;
  out.width = new_width;
  out.height = new_height;
  out.dpi_x = src.dpi_x;
  out.dpi_y = src.dpi_y;
  out.pixels.resize(static_cast<std::size_t>(new_width) * new_height);

  for (int y = 0; y < new_height; ++y) {
    const int src_y = static_cast<int>(static_cast<float>(y) * src.height / new_height);
    for (int x = 0; x < new_width; ++x) {
      const int src_x = static_cast<int>(static_cast<float>(x) * src.width / new_width);
      out.pixels[static_cast<std::size_t>(y) * new_width + x] =
          src.pixels[static_cast<std::size_t>(src_y) * src.width + src_x];
    }
  }

  return out;
}

}  // namespace falcon::core
