#include "falcon/core/Features.h"

#include <algorithm>

namespace falcon::core {

FeatureVector ComputeZoningFeatures(const GlyphBitmap& bitmap) {
  FeatureVector features{};
  const int zones = 4;
  const int zone_size = kGlyphSize / zones;

  for (int zy = 0; zy < zones; ++zy) {
    for (int zx = 0; zx < zones; ++zx) {
      float sum = 0.0f;
      for (int y = 0; y < zone_size; ++y) {
        for (int x = 0; x < zone_size; ++x) {
          const int gx = zx * zone_size + x;
          const int gy = zy * zone_size + y;
          const std::size_t idx = static_cast<std::size_t>(gy) * kGlyphSize + gx;
          sum += static_cast<float>(bitmap[idx]) / 255.0f;
        }
      }
      features[zy * zones + zx] = sum / static_cast<float>(zone_size * zone_size);
    }
  }

  return features;
}

}  // namespace falcon::core
