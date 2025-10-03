# FalconOCR

From-scratch C++ OCR with native Win32 GUI. No external libraries.

## Project Layout

```
src/
  app/         // Win32 entry point + CLI preview
  core/        // image IO, binarization, segmentation, normalization, glyph DB, classifier
  ocr/         // high-level pipeline orchestration
  util/        // support utilities (timers, string helpers)
include/       // public headers grouped by namespace
assets/        // sample inputs and glyph resources (placeholder)
docs/          // design documentation
tests/         // GoogleTest-based unit coverage
```

## Building

### Windows (Visual Studio / MSVC)

1. Configure with CMake (choose your preferred generator, e.g. Visual Studio 2022):

   ```
   cmake -S . -B build -G "Visual Studio 17 2022"
   ```

2. Build the solution:

   ```
   cmake --build build --config Release
   ```

### Linux/macOS (CLI preview)

The project defaults to the system `CXX` compiler. If your environment sets it to an unavailable toolchain, override it explicitly:

```
CXX=g++ cmake -S . -B build
cmake --build build
```

Run the smoke unit tests:

```
ctest --test-dir build
```

## Running FalconOCR

### Windows GUI

After building, launch `build/src/Release/falcon_app.exe` (or the configuration you built). The native window provides:

* **File → Open…** — choose a BMP/PGM/PPM image to preview.
* **OCR → Run OCR** — execute the built-in recognition pipeline using every discovered language pack and show the extracted text. The message box lists the active packs so you can confirm coverage.

The loaded page is rendered with simple letterboxing and you can re-run OCR after loading additional images.

### Linux/macOS Command-Line Preview

Non-Windows platforms expose a lightweight CLI front-end instead of the GUI:

```
./build/src/falcon_app <image.bmp/pgm/ppm>
```


### Unicode Language Support

FalconOCR discovers glyph templates from every folder within [`assets/langpacks`](assets/langpacks) and any directory listed in
the `FALCON_LANG_PATHS` environment variable. Each pack contributes one or more `glyphs.txt` files that describe normalized
bitmaps for arbitrary Unicode code points. When no explicit language list is provided, the runtime automatically merges every
pack alongside the built-in ASCII fallback so you can combine multiple scripts in a single OCR pass.

The CLI respects the optional `FALCON_LANGS` environment variable to restrict recognition to a subset of packs:

```bash
FALCON_LANGS="greek_basic,arabic_digits" ./build/src/falcon_app sample.bmp
```

Leave `FALCON_LANGS` unset to activate all discovered packs. The sample assets bundled with the repository serve as scaffolding;
swap them with high-quality templates trained for your target languages to achieve accurate recognition across global scripts.

## Architecture Overview

See [docs/architecture.md](docs/architecture.md) for the high-level system design, module responsibilities, and roadmap for the FalconOCR platform.
