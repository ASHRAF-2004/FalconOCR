#pragma once

#include <cstdint>
#include <vector>

#include "falcon/core/Geometry.h"

namespace falcon::core {

struct Raster {
  int width{};
  int height{};
  int dpi_x{96};
  int dpi_y{96};
  std::vector<uint8_t> pixels;  // grayscale 0-255

  [[nodiscard]] bool Empty() const noexcept { return width == 0 || height == 0 || pixels.empty(); }
  [[nodiscard]] SizeI Size() const noexcept { return SizeI{width, height}; }
};

struct BinaryImage {
  int width{};
  int height{};
  std::vector<uint8_t> data;  // 0 or 1

  [[nodiscard]] bool Empty() const noexcept { return width == 0 || height == 0 || data.empty(); }
};

}  // namespace falcon::core
