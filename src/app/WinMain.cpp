#include "falcon/app/MainWindow.h"
#include "falcon/core/Image.h"
#include "falcon/ocr/Pipeline.h"
#include "falcon/util/String.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifndef _WIN32
void RunCli(int argc, char** argv) {
  std::cout << "FalconOCR CLI preview" << std::endl;
  if (argc < 2) {
    std::cout << "Usage: falcon_app <image.bmp/pgm/ppm>" << std::endl;
    return;
  }

  const std::filesystem::path path = argv[1];
  try {
    const auto raster = falcon::core::LoadImage(path);
    falcon::ocr::OcrOptions options;
    if (const char* env_langs = std::getenv("FALCON_LANGS"); env_langs != nullptr) {
      std::string_view view(env_langs);
      std::size_t start = 0;
      while (start <= view.size()) {
        const std::size_t end = view.find_first_of(",;", start);
        const auto length = (end == std::string_view::npos) ? view.size() - start : end - start;
        if (length > 0) {
          options.languages.emplace_back(view.substr(start, length));
        }
        if (end == std::string_view::npos) {
          break;
        }
        start = end + 1;
      }
    }
    options.ascii_only = false;
    const auto page = falcon::ocr::RunOcr(raster, options);

    for (const auto& line : page.lines) {
      std::u32string text;
      for (const auto& ch : line.characters) {
        text.push_back(ch.classification.codepoint);
      }
      std::cout << falcon::util::ToUtf8(text) << std::endl;
    }
  } catch (const std::exception& ex) {
    std::cerr << "OCR failed: " << ex.what() << std::endl;
  }
}
#endif

}  // namespace

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  return falcon::app::RunGuiApp();
}
#else
int main(int argc, char** argv) {
  if (argc > 1) {
    RunCli(argc, argv);
    return 0;
  }

  std::cout << "No input file specified. Launching GUI stub..." << std::endl;
  return falcon::app::RunGuiApp();
}
#endif
