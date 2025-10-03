# Language Pack Format

Language packs allow FalconOCR to load glyph templates for any Unicode script without
recompiling the application. Each subdirectory beneath this folder represents a logical
pack. The runtime automatically discovers packs and exposes them to the OCR pipeline.

## Directory Structure

```
assets/langpacks/
  ├─ greek_basic/
  │   └─ glyphs.txt
  ├─ arabic_digits/
  │   └─ glyphs.txt
  └─ …
```

Multiple search roots can be specified by setting the `FALCON_LANG_PATHS` environment
variable to a list of directories separated by `;` or `:`. Every pack directory must contain
at least one `glyphs.txt` file in the plain-text format described below.

## glyphs.txt Format

Each glyph entry starts with a header line and is followed by `16` rows describing the
normalized bitmap (where `#` or `1` indicates an inked pixel and any other character is
treated as background).

```
glyph U+0041
....########....
...##......##...
..##........##..
..##........##..
################
##............##
##............##
##............##
##............##
##............##
##............##
##............##
##............##
##............##
##............##
##............##
```

You can generate these bitmaps from font outlines, scanner exemplars, or synthetic
renderings. The OCR pipeline merges glyphs from every active pack (or all packs if no
explicit language list is provided) and falls back to the built-in ASCII template set unless
the caller disables it.

## Shipping Custom Packs

1. Create a new folder beneath `assets/langpacks` (or in another directory referenced by
   `FALCON_LANG_PATHS`).
2. Place one or more glyph definition files named `glyphs.txt` inside the folder. You can
   split packs across multiple files if needed; all files will be scanned.
3. Launch FalconOCR with the environment variable `FALCON_LANGS` to restrict recognition
   to specific packs (comma or semicolon separated). Leave it unset to enable every
   available pack automatically.

The sample packs included with the repository provide scaffold bitmaps for a variety of
scripts. Replace them with high-quality templates trained for your languages to obtain
accurate recognition.
