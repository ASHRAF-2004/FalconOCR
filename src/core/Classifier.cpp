#include "falcon/core/Classifier.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

#include "falcon/core/GlyphDB.h"

namespace falcon::core {

ClassificationResult ClassifyGlyph(const GlyphBitmap& glyph, const std::vector<GlyphTemplate>& templates) {
  if (templates.empty()) {
    throw std::runtime_error("No glyph templates available");
  }

  ClassificationResult best{};
  double best_distance = std::numeric_limits<double>::max();

  for (const auto& tmpl : templates) {
    double distance = 0.0;
    for (std::size_t i = 0; i < glyph.size(); ++i) {
      distance += std::abs(static_cast<int>(glyph[i]) - static_cast<int>(tmpl.bitmap[i]));
    }
    distance /= (255.0 * static_cast<double>(glyph.size()));
    if (distance < best_distance) {
      best_distance = distance;
      best.codepoint = tmpl.codepoint;
      best.confidence = static_cast<float>(std::clamp(1.0 - distance, 0.0, 1.0));
    }
  }

  return best;
}

std::u32string GlyphsToString(const std::vector<ClassificationResult>& glyphs) {
  std::u32string result;
  result.reserve(glyphs.size());
  for (const auto& glyph : glyphs) {
    result.push_back(glyph.codepoint);
  }
  return result;
}

}  // namespace falcon::core
