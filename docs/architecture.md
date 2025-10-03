# FalconOCR Architecture Overview

## 1. High-Level System Layout

```
┌──────────────────────────────────────────────────────────────────────┐
│                              GUI Layer                               │
│   (Win32 native)                                                     │
│  • Viewer: Image/PDF canvas, page nav, zoom, selection               │
│  • OCR controls: page scope (current/all/range), languages, charset  │
│  • Job panel: queue, progress, cancel, logs                          │
└───────────────▲───────────────────────────▲───────────────────────────┘
                │                           │
      ┌─────────┴─────────┐         ┌───────┴───────────┐
      │  Document Layer    │         │  Job Orchestrator │
      │ (unified model)    │         │  & Pipeline Exec  │
      │ • ImageDoc         │         │ • Schedules OCR   │
      │ • PdfDoc           │         │ • Threads/pools   │
      │ • Page, Layer,     │         │ • Cancellation    │
      │   Raster, TextRun  │         │ • Telemetry/logs  │
      └─────────▲──────────┘         └────────▲──────────┘
                │                               │
   ┌────────────┴─────────────┐     ┌───────────┴──────────────┐
   │       I/O Subsystem      │     │     OCR Pipeline          │
   │ • Image decoders         │     │ • Preprocess → Segment →  │
   │   (BMP/PGM/PPM core;     │     │   Normalize → Classify →  │
   │    PNG/JPEG optional)    │     │   Postprocess → Reconstruct│
   │ • PDF parser/rasterizer  │     │ • Language Packs (Unicode) │
   └───────▲──────────▲───────┘     └──────────▲────────────────┘
           │          │                        │
   ┌───────┴───┐  ┌───┴──────────┐     ┌───────┴─────────────┐
   │Core Render│  │ Font Subsys  │     │  Recognition Engines │
   │ • Scanline│  │ • Basic font │     │  • Template/kNN      │
   │ • Composit│  │   bitmap core│     │  • Lightweight CNN   │
   │ • Filters │  │ • Script shaping    │  • Layout/line finder│
   └───────────┘  └──────────────┘     └──────────────────────┘
```

Design goals: pure core (BMP/PGM/PPM, limited PDF), scalable plug-in decoders and OCR engines, precision with multi-language and per-region OCR, robustness with job queue and deterministic memory.

## 2. Modules & Responsibilities

### 2.1 GUI Layer (Win32 Only)
- Main frame with file/view/OCR menus, toolbar, status bar.
- Canvas control: owner-drawn, double-buffered, handles pan, zoom, selection rectangles, and overlays (page boxes, OCR bounding boxes, text baseline guides).
- Dockable panels:
  - **Pages**: thumbnails, current page highlight, range selection.
  - **OCR panel**: multi-select languages, character set filters, scope selection (current/range/all), output format (plain text, TSV, HOCR/ALTO).
  - **Jobs**: queue list with progress, ETA, pause/cancel.
  - **Log/Diagnostics**: recent messages.
- Key flows: open file → DocumentFactory loads → first page rasterized → canvas displays. OCR commands spawn jobs via orchestrator.

### 2.2 Document Layer (Unified Model)
- Abstract `Document` with `PageCount()`, `GetPage()`, metadata.
- `ImageDocument`: vector of `RasterPage{width,height,DPI,pixels}`.
- `PdfDocument`: parses PDF into `PdfPage` entries exposing `RenderToRaster`.
- Common `Page` model offering `Raster getRaster(zoom)` backed by render cache and layer placeholders.

### 2.3 I/O Subsystem
- Pure core decoders: BMP, PGM, PPM. Interface `IImageDecoder` for optional PNG/JPEG modules.
- Minimal PDF parser: detect embedded image XObjects with `/FlateDecode`, implement custom DEFLATE inflater. Later phases add simple graphics and text adapters.
- Parsers registered via `IDocumentParser::TryOpen(stream)`.

### 2.4 Core Rendering Engine
- Scanline compositing with RGBA32 internal format, nearest/bilinear scaling, 90° increments rotation, simple affine transforms for PDFs.
- Filters: grayscale, binarization (Otsu), sharpen, deskew preview.
- Tile cache with mip levels for zoom performance.

### 2.5 OCR Pipeline (Pluggable Stages)
- Contract: `Raster → Preprocess → Segmentation → Glyph Extraction → Normalization → Classification → Postprocess → Reconstruction → Output`.
- **Preprocess**: denoise (median/box), deskew (Hough/projection), contrast stretch, Otsu/Sauvola binarization.
- **Segmentation**: connected components, line finding, word gap statistics.
- **Normalization**: scale glyphs to 16×16 or 24×24, center-of-mass alignment.
- **Classification engines**:
  - Template/kNN per language pack.
  - Tiny CNN (2–3 conv layers) with offline training.
- **Postprocess**: script shaping rules, language bigram Viterbi decode, punctuation normalization.
- **Reconstruction**: produce Unicode text, char/word/line boxes, direction flags, column ordering.
- **Outputs**: UTF-8 text, TSV, HOCR-like HTML, optional ALTO XML.
- **Language packs**: directories with glyph data, language models, script metadata. Support multi-pack fusion and auto-script detection.

### 2.6 Font & Shaping
- No external shaping engine. Apply contextual shaping post-recognition (e.g., Arabic joining forms, basic Indic matra positioning). GUI font handles final shaping display.

### 2.7 Job Orchestrator
- Thread pool (`cores - 1`).
- Job types: rasterize page, OCR page, export selection, save text.
- Scheduling prioritizes current page tasks, batches for all-page OCR.
- Progress/cancellation with stage reporting and cooperative flags.
- Telemetry: per-stage timings, confidence histograms.

## 3. Core Data Structures

```cpp
struct RectI { int x,y,w,h; };
struct SizeI { int w,h; };

struct Raster {
  int width, height; int dpiX, dpiY;
  std::vector<uint32_t> rgba; // row-major
};

class Page {
public:
  virtual SizeI PixelSize(int zoom) const = 0;
  virtual bool Render(Raster& out, int zoom, RectI* bbox=nullptr) = 0;
};

class Document {
public:
  virtual int PageCount() const = 0;
  virtual Page& GetPage(int idx) = 0;
  virtual ~Document() = default;
};

struct CharBox { RectI box; uint32_t codepoint; float conf; };
struct Word { std::vector<CharBox> chars; RectI box; float conf; };
struct Line { std::vector<Word> words; RectI box; float conf; bool rtl; };
struct OcrPage { std::vector<Line> lines; SizeI imageSize; };
```

## 4. Public APIs

```cpp
std::unique_ptr<Document> OpenDocument(std::istream&);

bool RenderPage(Document&, int pageIndex, int zoom, Raster&);

struct OcrOptions {
  std::vector<std::string> languages; // e.g., {"en","ar"}
  bool ascii_only = false;
  RectI* region = nullptr;  // optional ROI
  bool detect_orientation = true;
};
OcrPage RunOcr(const Raster&, const OcrOptions&);

std::vector<OcrPage> RunOcr(Document&, std::vector<int> pageIndices, const OcrOptions&);
```

## 5. PDF Support Plan
- **Phase A (MVP)**: parse xref/trailer/page tree, extract image XObjects (FlateDecode), custom inflater, composite images for OCR.
- **Phase B**: add simple graphics (fills, strokes, CTM), basic alpha compositing.
- **Phase C**: optional bitmap font handling for searchable text overlays.

## 6. Multi-Language Strategy
- Script auto-detection per line using connected component features, density metrics, small SVM.
- Language pack contents: templates/CNN weights, char bigrams, rules (direction, normalization).
- Always load universal ASCII+Digits pack as fallback.
- Runtime loader searches `assets/langpacks` (configurable via `FALCON_LANG_PATHS`) and merges glyph templates from every discovered pack; callers may restrict packs with `OcrOptions::languages` or the `FALCON_LANGS` env var.

## 7. Performance & Quality
- Tile rasterization + mipmaps for smooth zoom.
- Parallel OCR by page/line, use SIMD for hotspots.
- Confidence-driven retries: reprocess low-confidence lines with alternate language mixes.

## 8. Storage & Output
- Project file (`.ocrsession`) storing doc path, per-page OCR results, options, corrections.
- Export formats: `.txt`, `.tsv`, `.hocr.html`, optional searchable PDF with hidden text layer.

## 9. Error Handling & Diagnostics
- Structured error enums (IO, Decode, PDFSyntax, OCRPipeline, Memory).
- Per-page diagnostics (skew, binarization threshold, average confidence).
- Visual overlays (red boxes) for low-confidence regions.

