#include "falcon/core/Binarize.h"

#include <array>
#include <numeric>
#include <stdexcept>

namespace falcon::core {

uint8_t OtsuThreshold(const Raster& image) {
  if (image.Empty()) {
    throw std::invalid_argument("OtsuThreshold requires non-empty image");
  }

  std::array<int, 256> histogram{};
  for (uint8_t value : image.pixels) {
    ++histogram[value];
  }

  const int total = image.width * image.height;
  double sum = 0.0;
  for (int i = 0; i < 256; ++i) {
    sum += static_cast<double>(i) * histogram[i];
  }

  double sum_b = 0.0;
  int w_b = 0;
  int w_f = 0;
  double max_between = -1.0;
  uint8_t threshold = 0;

  for (int t = 0; t < 256; ++t) {
    w_b += histogram[t];
    if (w_b == 0) {
      continue;
    }

    w_f = total - w_b;
    if (w_f == 0) {
      break;
    }

    sum_b += static_cast<double>(t) * histogram[t];
    const double m_b = sum_b / w_b;
    const double m_f = (sum - sum_b) / w_f;
    const double between = static_cast<double>(w_b) * static_cast<double>(w_f) * (m_b - m_f) * (m_b - m_f);

    if (between > max_between) {
      max_between = between;
      threshold = static_cast<uint8_t>(t);
    }
  }

  return threshold;
}

BinaryImage ApplyThreshold(const Raster& image, uint8_t threshold) {
  if (image.Empty()) {
    throw std::invalid_argument("ApplyThreshold requires non-empty image");
  }

  BinaryImage binary;
  binary.width = image.width;
  binary.height = image.height;
  binary.data.resize(image.pixels.size());

  for (std::size_t i = 0; i < image.pixels.size(); ++i) {
    binary.data[i] = image.pixels[i] > threshold ? 1 : 0;
  }

  return binary;
}

BinaryImage BinarizeOtsu(const Raster& image) {
  const uint8_t threshold = OtsuThreshold(image);
  return ApplyThreshold(image, threshold);
}

}  // namespace falcon::core
