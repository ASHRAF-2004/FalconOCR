#pragma once

#include <filesystem>
#include <string>

#include "falcon/core/Raster.h"

namespace falcon::core {

Raster LoadImage(const std::filesystem::path& path);
Raster LoadBmp(const std::filesystem::path& path);
Raster LoadPnm(const std::filesystem::path& path);  // supports PGM/PPM (P2/P3/P5/P6)

Raster ConvertToGrayscale(const Raster& src);
Raster ResizeNearest(const Raster& src, int new_width, int new_height);

}  // namespace falcon::core
