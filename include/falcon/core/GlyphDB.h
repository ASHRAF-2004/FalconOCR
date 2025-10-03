#pragma once

#include <string>
#include <vector>

#include "falcon/core/Normalize.h"

namespace falcon::core {

struct GlyphTemplate {
  char32_t codepoint{};
  GlyphBitmap bitmap{};
  std::string language;
};

const std::vector<GlyphTemplate>& BuiltInGlyphTemplates();

std::vector<GlyphTemplate> CollectGlyphTemplates(const std::vector<std::string>& languages,
                                                 bool include_ascii_fallback);

std::vector<std::string> DiscoverLanguagePacks();

}  // namespace falcon::core
