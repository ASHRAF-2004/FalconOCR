#include <algorithm>
#include <string>

#include <gtest/gtest.h>

#include "falcon/core/Classifier.h"
#include "falcon/core/GlyphDB.h"
#include "falcon/core/Raster.h"
#include "falcon/ocr/Pipeline.h"
#include "falcon/util/String.h"

using namespace falcon;

namespace {

core::Raster RasterFromGlyph(const core::GlyphTemplate& glyph) {
  core::Raster raster;
  raster.width = core::kGlyphSize;
  raster.height = core::kGlyphSize;
  raster.pixels.resize(static_cast<std::size_t>(core::kGlyphSize) * core::kGlyphSize);
  for (int y = 0; y < core::kGlyphSize; ++y) {
    for (int x = 0; x < core::kGlyphSize; ++x) {
      raster.pixels[static_cast<std::size_t>(y) * raster.width + x] =
          glyph.bitmap[static_cast<std::size_t>(y) * core::kGlyphSize + x];
    }
  }
  return raster;
}

}  // namespace

TEST(Classifier, MatchesSelectedGlyphs) {
  const auto glyphs = core::CollectGlyphTemplates(std::vector<std::string>{}, true);
  ASSERT_FALSE(glyphs.empty());

  const std::u32string probe = U"ABCD1234";
  for (char32_t codepoint : probe) {
    const auto it = std::find_if(glyphs.begin(), glyphs.end(), [codepoint](const core::GlyphTemplate& glyph) {
      return glyph.codepoint == codepoint;
    });
    ASSERT_NE(it, glyphs.end());

    const auto result = core::ClassifyGlyph(it->bitmap, glyphs);
    EXPECT_EQ(result.codepoint, it->codepoint);
    EXPECT_GT(result.confidence, 0.5f);
  }
}

TEST(OcrPipeline, RecognizesSingleGlyph) {
  const auto glyphs = core::CollectGlyphTemplates(std::vector<std::string>{}, true);
  const auto it = std::find_if(glyphs.begin(), glyphs.end(), [](const core::GlyphTemplate& glyph) {
    return glyph.codepoint == U'I';
  });
  ASSERT_NE(it, glyphs.end());

  const auto raster = RasterFromGlyph(*it);
  const auto page = ocr::RunOcr(raster, {});
  ASSERT_FALSE(page.lines.empty());
  std::u32string recognized;
  for (const auto& line : page.lines) {
    for (const auto& ch : line.characters) {
      recognized.push_back(ch.classification.codepoint);
    }
  }
  SCOPED_TRACE(std::string("Recognized: ") + falcon::util::ToUtf8(recognized));
  ASSERT_FALSE(recognized.empty());
  EXPECT_GT(page.lines.front().characters.front().classification.confidence, 0.0f);
}
