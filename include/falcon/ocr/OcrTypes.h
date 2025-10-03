#pragma once

#include <string>
#include <vector>

#include "falcon/core/Classifier.h"
#include "falcon/core/Geometry.h"

namespace falcon::ocr {

struct OcrChar {
  falcon::core::RectI bounds{};
  falcon::core::ClassificationResult classification{};
};

struct OcrLine {
  std::vector<OcrChar> characters;
  bool rtl{false};
};

struct OcrPage {
  std::vector<OcrLine> lines;
  falcon::core::SizeI image_size{};
};

struct OcrOptions {
  std::vector<std::string> languages;
  bool ascii_only{false};
  bool detect_orientation{true};
  falcon::core::RectI region{};
  bool has_region{false};
};

}  // namespace falcon::ocr
