#pragma once

#include <cstddef>
#include <cstdint>

#include "falcon/core/Raster.h"

namespace falcon::core {

uint8_t OtsuThreshold(const Raster& image);
BinaryImage ApplyThreshold(const Raster& image, uint8_t threshold);
BinaryImage BinarizeOtsu(const Raster& image);

}  // namespace falcon::core
