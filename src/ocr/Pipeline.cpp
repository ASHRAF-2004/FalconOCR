#include "falcon/ocr/Pipeline.h"

#include <algorithm>
#include <stdexcept>

#include "falcon/core/Binarize.h"
#include "falcon/core/Classifier.h"
#include "falcon/core/GlyphDB.h"
#include "falcon/core/Image.h"
#include "falcon/core/Normalize.h"
#include "falcon/core/Segment.h"
#include "falcon/ocr/OcrTypes.h"

namespace falcon::ocr {

namespace {

bool Intersects(const falcon::core::RectI& a, const falcon::core::RectI& b) {
  const bool no_overlap = a.Right() <= b.x || b.Right() <= a.x || a.Bottom() <= b.y || b.Bottom() <= a.y;
  return !no_overlap;
}

}  // namespace

OcrPage RunOcr(const falcon::core::Raster& raster, const OcrOptions& options) {
  if (raster.Empty()) {
    throw std::invalid_argument("RunOcr requires a non-empty raster");
  }

  const falcon::core::BinaryImage binary = falcon::core::BinarizeOtsu(raster);
  auto components = falcon::core::ConnectedComponents(binary);
  const auto templates = falcon::core::CollectGlyphTemplates(options.languages, !options.ascii_only);
  if (templates.empty()) {
    throw std::runtime_error("No glyph templates available for requested languages");
  }

  const falcon::core::RectI region = options.has_region ? options.region
                                                        : falcon::core::RectI{0, 0, raster.width, raster.height};

  components.erase(std::remove_if(components.begin(), components.end(), [&](const auto& component) {
                    if (component.area < 4) {
                      return true;
                    }
                    if (!options.has_region) {
                      return false;
                    }
                    return !Intersects(component.bounds, region);
                  }),
                  components.end());

  std::sort(components.begin(), components.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.bounds.y == rhs.bounds.y) {
      return lhs.bounds.x < rhs.bounds.x;
    }
    return lhs.bounds.y < rhs.bounds.y;
  });

  OcrPage page;
  page.image_size = raster.Size();

  const int line_merge_threshold = falcon::core::kGlyphSize * 2;

  for (const auto& component : components) {
    const falcon::core::GlyphBitmap glyph_bitmap = falcon::core::NormalizeGlyph(binary, component.bounds);
    falcon::core::ClassificationResult classification = falcon::core::ClassifyGlyph(glyph_bitmap, templates);

    if (options.ascii_only && classification.codepoint > 0x7F) {
      classification.codepoint = U'?';
    }

    if (page.lines.empty() ||
        component.bounds.y > page.lines.back().characters.back().bounds.y + line_merge_threshold) {
      page.lines.push_back(OcrLine{});
    }

    OcrChar ocr_char;
    ocr_char.bounds = component.bounds;
    ocr_char.classification = classification;
    page.lines.back().characters.push_back(ocr_char);
  }

  return page;
}

}  // namespace falcon::ocr
