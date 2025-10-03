#pragma once

#include "falcon/core/Raster.h"
#include "falcon/ocr/OcrTypes.h"

namespace falcon::ocr {

OcrPage RunOcr(const falcon::core::Raster& raster, const OcrOptions& options = {});

}  // namespace falcon::ocr
