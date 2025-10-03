#include "falcon/core/Normalize.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace falcon::core {

GlyphBitmap NormalizeGlyph(const BinaryImage& image, const RectI& bounds) {
  if (image.Empty() || bounds.width <= 0 || bounds.height <= 0) {
    throw std::invalid_argument("NormalizeGlyph requires a valid component");
  }

  GlyphBitmap bitmap{};

  const int max_dim = std::max(bounds.width, bounds.height);
  const float scale = static_cast<float>(max_dim) / static_cast<float>(kGlyphSize);
  const float pad_x = static_cast<float>(max_dim - bounds.width) / 2.0f;
  const float pad_y = static_cast<float>(max_dim - bounds.height) / 2.0f;

  for (int y = 0; y < kGlyphSize; ++y) {
    for (int x = 0; x < kGlyphSize; ++x) {
      const float sample_x = static_cast<float>(bounds.x) - pad_x + (static_cast<float>(x) + 0.5f) * scale;
      const float sample_y = static_cast<float>(bounds.y) - pad_y + (static_cast<float>(y) + 0.5f) * scale;

      int src_x = static_cast<int>(std::round(sample_x));
      int src_y = static_cast<int>(std::round(sample_y));

      src_x = std::clamp(src_x, 0, image.width - 1);
      src_y = std::clamp(src_y, 0, image.height - 1);

      const std::size_t src_idx = static_cast<std::size_t>(src_y) * image.width + src_x;
      bitmap[static_cast<std::size_t>(y) * kGlyphSize + x] = image.data[src_idx] ? 255 : 0;
    }
  }

  return bitmap;
}

}  // namespace falcon::core
