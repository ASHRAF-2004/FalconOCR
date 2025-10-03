#include "falcon/core/GlyphDB.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace falcon::core {

namespace {

struct RawGlyph {
  char32_t codepoint;
  std::array<const char*, 7> rows;
};

GlyphBitmap From5x7(const RawGlyph& glyph) {
  GlyphBitmap bitmap{};
  for (int y = 0; y < kGlyphSize; ++y) {
    const float src_y = (static_cast<float>(y) + 0.5f) * 7.0f / static_cast<float>(kGlyphSize);
    int iy = static_cast<int>(src_y);
    if (iy < 0) {
      iy = 0;
    }
    if (iy > 6) {
      iy = 6;
    }
    for (int x = 0; x < kGlyphSize; ++x) {
      const float src_x = (static_cast<float>(x) + 0.5f) * 5.0f / static_cast<float>(kGlyphSize);
      int ix = static_cast<int>(src_x);
      if (ix < 0) {
        ix = 0;
      }
      if (ix > 4) {
        ix = 4;
      }
      const char ch = glyph.rows[static_cast<std::size_t>(iy)][static_cast<std::size_t>(ix)];
      bitmap[static_cast<std::size_t>(y) * kGlyphSize + x] = (ch == '#') ? 255 : 0;
    }
  }
  return bitmap;
}

std::vector<RawGlyph> BuildRawGlyphs() {
  using RG = RawGlyph;
  return {
      RG{U' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
      RG{U'.', {".....", ".....", ".....", ".....", ".....", "..##.", "..##."}},
      RG{U',', {".....", ".....", ".....", ".....", "..##.", "..##.", ".##.."}},
      RG{U'-', {".....", ".....", ".....", "#####", ".....", ".....", "....."}},
      RG{U':', {".....", "..##.", "..##.", ".....", "..##.", "..##.", "....."}},
      RG{U'!', {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}},
      RG{U'?', {".###.", "#...#", "....#", "..##.", "..#..", ".....", "..#.."}},
      RG{U'0', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
      RG{U'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
      RG{U'2', {".###.", "#...#", "....#", "..##.", ".#...", "#....", "#####"}},
      RG{U'3', {".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."}},
      RG{U'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
      RG{U'5', {"#####", "#....", "#....", "####.", "....#", "#...#", ".###."}},
      RG{U'6', {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."}},
      RG{U'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
      RG{U'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
      RG{U'9', {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}},
      RG{U'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
      RG{U'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
      RG{U'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
      RG{U'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
      RG{U'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
      RG{U'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
      RG{U'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###."}},
      RG{U'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
      RG{U'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
      RG{U'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
      RG{U'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
      RG{U'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
      RG{U'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
      RG{U'N', {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"}},
      RG{U'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
      RG{U'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
      RG{U'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
      RG{U'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
      RG{U'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
      RG{U'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
      RG{U'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
      RG{U'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
      RG{U'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"}},
      RG{U'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
      RG{U'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
      RG{U'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
      RG{U'a', {".....", "..##.", "...#.", ".###.", "#..#.", "#..#.", ".###."}},
      RG{U'b', {"#....", "#....", "#....", "####.", "#...#", "#...#", "####."}},
      RG{U'c', {".....", ".###.", "#...#", "#....", "#....", "#...#", ".###."}},
      RG{U'd', {"....#", "....#", "....#", ".####", "#...#", "#...#", ".####"}},
      RG{U'e', {".....", ".###.", "#...#", "#####", "#....", "#...#", ".###."}},
      RG{U'f', {"..##.", ".#..#", ".#...", "###..", ".#...", ".#...", ".#..."}},
      RG{U'g', {".....", ".####", "#...#", "#...#", ".####", "....#", ".###."}},
      RG{U'h', {"#....", "#....", "#....", "####.", "#...#", "#...#", "#...#"}},
      RG{U'i', {"..#..", ".....", ".##..", "..#..", "..#..", "..#..", ".###."}},
      RG{U'j', {"...#.", ".....", "..##.", "...#.", "...#.", "#..#.", ".##.."}},
      RG{U'k', {"#....", "#....", "#..#.", "#.#..", "##...", "#.#..", "#..#."}},
      RG{U'l', {".##..", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
      RG{U'm', {".....", "##.##", "#.#.#", "#.#.#", "#.#.#", "#.#.#", "#.#.#"}},
      RG{U'n', {".....", "####.", "#...#", "#...#", "#...#", "#...#", "#...#"}},
      RG{U'o', {".....", ".###.", "#...#", "#...#", "#...#", "#...#", ".###."}},
      RG{U'p', {".....", "####.", "#...#", "#...#", "####.", "#....", "#...."}},
      RG{U'q', {".....", ".####", "#...#", "#...#", ".####", "....#", "....#"}},
      RG{U'r', {".....", "#.##.", "##..#", "#....", "#....", "#....", "#...."}},
      RG{U's', {".....", ".####", "#....", ".###.", "....#", "....#", "####."}},
      RG{U't', {".#...", ".#...", "###..", ".#...", ".#...", ".#..#", "..##."}},
      RG{U'u', {".....", "#...#", "#...#", "#...#", "#...#", "#..##", ".##.#"}},
      RG{U'v', {".....", "#...#", "#...#", "#...#", ".#.#.", ".#.#.", "..#.."}},
      RG{U'w', {".....", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}},
      RG{U'x', {".....", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
      RG{U'y', {".....", "#...#", "#...#", ".####", "....#", "#...#", ".###."}},
      RG{U'z', {".....", "#####", "...#.", "..#..", ".#...", "#....", "#####"}},
  };
}

std::string Trim(std::string_view value) {
  const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  std::size_t start = 0;
  while (start < value.size() && is_space(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  std::size_t end = value.size();
  while (end > start && is_space(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return std::string(value.substr(start, end - start));
}

std::optional<char32_t> ParseCodepoint(std::string_view token) {
  std::string upper;
  upper.reserve(token.size());
  for (char ch : token) {
    upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  const std::string prefix = "U+";
  std::string_view hex;
  if (upper.rfind(prefix, 0) == 0) {
    hex = std::string_view(upper).substr(prefix.size());
  } else {
    hex = upper;
  }
  if (hex.empty() || hex.size() > 6) {
    return std::nullopt;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(std::string(hex).c_str(), &end, 16);
  if (end == nullptr || *end != '\0') {
    return std::nullopt;
  }
  if (value > 0x10FFFFUL) {
    return std::nullopt;
  }
  return static_cast<char32_t>(value);
}

bool IsComment(std::string_view line) {
  for (char ch : line) {
    if (std::isspace(static_cast<unsigned char>(ch)) == 0) {
      return ch == '#';
    }
  }
  return true;
}

GlyphBitmap GlyphFromRasterLines(const std::vector<std::string>& rows) {
  GlyphBitmap bitmap{};
  for (int y = 0; y < kGlyphSize; ++y) {
    const std::string& row = rows[static_cast<std::size_t>(y)];
    for (int x = 0; x < kGlyphSize; ++x) {
      const char ch = (x < static_cast<int>(row.size())) ? row[static_cast<std::size_t>(x)] : '.';
      bitmap[static_cast<std::size_t>(y) * kGlyphSize + x] = (ch == '#' || ch == '1') ? 255 : 0;
    }
  }
  return bitmap;
}

std::vector<GlyphTemplate> LoadGlyphPack(const std::filesystem::path& file, const std::string& language) {
  std::ifstream stream(file);
  if (!stream.is_open()) {
    return {};
  }

  std::vector<GlyphTemplate> templates;
  std::string line;
  while (std::getline(stream, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || IsComment(trimmed)) {
      continue;
    }
    std::istringstream header(trimmed);
    std::string glyph_token;
    header >> glyph_token;
    if (glyph_token != "glyph") {
      continue;
    }
    std::string codepoint_token;
    header >> codepoint_token;
    const auto codepoint = ParseCodepoint(codepoint_token);
    if (!codepoint.has_value()) {
      continue;
    }
    std::vector<std::string> rows;
    rows.reserve(kGlyphSize);
    for (int i = 0; i < kGlyphSize && std::getline(stream, line); ++i) {
      rows.push_back(line);
    }
    if (rows.size() != static_cast<std::size_t>(kGlyphSize)) {
      break;
    }
    GlyphTemplate tmpl{};
    tmpl.codepoint = *codepoint;
    tmpl.bitmap = GlyphFromRasterLines(rows);
    tmpl.language = language;
    templates.push_back(std::move(tmpl));
  }

  return templates;
}

std::vector<std::filesystem::path> CandidateSearchPaths() {
  std::vector<std::filesystem::path> paths;
  const char* env = std::getenv("FALCON_LANG_PATHS");
  if (env != nullptr) {
    std::string_view view(env);
    std::size_t start = 0;
    while (start <= view.size()) {
      const std::size_t end = view.find_first_of(";:", start);
      const auto length = (end == std::string_view::npos) ? view.size() - start : end - start;
      if (length > 0) {
        paths.emplace_back(std::string(view.substr(start, length)));
      }
      if (end == std::string_view::npos) {
        break;
      }
      start = end + 1;
    }
  }

  std::filesystem::path current = std::filesystem::current_path();
  for (int depth = 0; depth < 4; ++depth) {
    const std::filesystem::path candidate = current / "assets" / "langpacks";
    if (std::filesystem::exists(candidate)) {
      paths.push_back(candidate);
    }
    if (!current.has_parent_path()) {
      break;
    }
    current = current.parent_path();
  }

  std::unordered_set<std::string> dedupe;
  std::vector<std::filesystem::path> result;
  for (const auto& path : paths) {
    if (!std::filesystem::exists(path)) {
      continue;
    }
    const std::string canonical = std::filesystem::weakly_canonical(path).string();
    if (dedupe.insert(canonical).second) {
      result.emplace_back(canonical);
    }
  }
  return result;
}

}  // namespace

const std::vector<GlyphTemplate>& BuiltInGlyphTemplates() {
  static const std::vector<GlyphTemplate> templates = [] {
    std::vector<GlyphTemplate> result;
    const auto raw = BuildRawGlyphs();
    result.reserve(raw.size());
    for (const RawGlyph& glyph : raw) {
      GlyphTemplate tmpl{};
      tmpl.codepoint = glyph.codepoint;
      tmpl.bitmap = From5x7(glyph);
      tmpl.language = "builtin";
      result.push_back(std::move(tmpl));
    }
    return result;
  }();

  return templates;
}

std::vector<std::string> DiscoverLanguagePacks() {
  std::set<std::string> languages;
  for (const auto& root : CandidateSearchPaths()) {
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
      continue;
    }
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
      if (entry.is_directory()) {
        languages.insert(entry.path().filename().string());
      }
    }
  }
  return {languages.begin(), languages.end()};
}

std::vector<GlyphTemplate> CollectGlyphTemplates(const std::vector<std::string>& languages,
                                                 bool include_ascii_fallback) {
  std::vector<GlyphTemplate> templates;
  if (include_ascii_fallback) {
    const auto& builtin = BuiltInGlyphTemplates();
    templates.insert(templates.end(), builtin.begin(), builtin.end());
  }

  std::vector<std::string> packs;
  if (languages.empty()) {
    packs = DiscoverLanguagePacks();
  } else {
    packs = languages;
  }

  if (packs.empty()) {
    return templates;
  }

  std::unordered_set<char32_t> seen;
  for (const auto& tmpl : templates) {
    seen.insert(tmpl.codepoint);
  }

  const auto roots = CandidateSearchPaths();
  for (const std::string& language : packs) {
    bool loaded = false;
    for (const auto& root : roots) {
      const auto lang_root = root / language;
      if (!std::filesystem::exists(lang_root) || !std::filesystem::is_directory(lang_root)) {
        continue;
      }
      for (const auto& file_entry : std::filesystem::directory_iterator(lang_root)) {
        if (!file_entry.is_regular_file()) {
          continue;
        }
        const auto& file_path = file_entry.path();
        if (file_path.extension() != ".txt") {
          continue;
        }
        auto pack_templates = LoadGlyphPack(file_path, language);
        for (auto& tmpl : pack_templates) {
          if (seen.insert(tmpl.codepoint).second) {
            templates.push_back(std::move(tmpl));
          }
        }
        loaded = true;
      }
    }
    if (!loaded) {
      std::filesystem::path direct(language);
      if (std::filesystem::exists(direct)) {
        auto pack_templates = LoadGlyphPack(direct, direct.stem().string());
        for (auto& tmpl : pack_templates) {
          if (seen.insert(tmpl.codepoint).second) {
            templates.push_back(std::move(tmpl));
          }
        }
      }
    }
  }

  return templates;
}

}  // namespace falcon::core

