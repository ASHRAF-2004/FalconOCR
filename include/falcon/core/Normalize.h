#pragma once

#include <array>

#include "falcon/core/Geometry.h"
#include "falcon/core/Raster.h"

namespace falcon::core {

constexpr int kGlyphSize = 16;
using GlyphBitmap = std::array<uint8_t, kGlyphSize * kGlyphSize>;

GlyphBitmap NormalizeGlyph(const BinaryImage& image, const RectI& bounds);

}  // namespace falcon::core
