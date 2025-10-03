#pragma once

#include <array>

#include "falcon/core/Normalize.h"

namespace falcon::core {

constexpr int kFeatureCount = 16;
using FeatureVector = std::array<float, kFeatureCount>;

FeatureVector ComputeZoningFeatures(const GlyphBitmap& bitmap);

}  // namespace falcon::core
