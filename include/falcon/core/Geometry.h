#pragma once

#include <cstdint>

namespace falcon::core {

struct SizeI {
  int width{};
  int height{};
};

struct PointI {
  int x{};
  int y{};
};

struct RectI {
  int x{};
  int y{};
  int width{};
  int height{};

  [[nodiscard]] int Right() const noexcept { return x + width; }
  [[nodiscard]] int Bottom() const noexcept { return y + height; }
};

inline bool operator==(const RectI& lhs, const RectI& rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator!=(const RectI& lhs, const RectI& rhs) noexcept { return !(lhs == rhs); }

}  // namespace falcon::core
