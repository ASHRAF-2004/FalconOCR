#pragma once

#include <string>
#include <vector>

#include "falcon/core/Normalize.h"
#include "falcon/core/GlyphDB.h"

namespace falcon::core {

struct ClassificationResult {
  char32_t codepoint{};
  float confidence{};  // 0-1
};

ClassificationResult ClassifyGlyph(const GlyphBitmap& glyph, const std::vector<GlyphTemplate>& templates);
std::u32string GlyphsToString(const std::vector<ClassificationResult>& glyphs);

}  // namespace falcon::core
