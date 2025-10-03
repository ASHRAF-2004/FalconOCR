#pragma once

#include <vector>

#include "falcon/core/Geometry.h"
#include "falcon/core/Raster.h"

namespace falcon::core {

struct ConnectedComponent {
  int label{};
  RectI bounds{};
  std::size_t area{};
};

std::vector<ConnectedComponent> ConnectedComponents(const BinaryImage& image);

}  // namespace falcon::core
