#include "falcon/core/Segment.h"

#include <array>
#include <queue>
#include <stdexcept>

namespace falcon::core {

std::vector<ConnectedComponent> ConnectedComponents(const BinaryImage& image) {
  if (image.Empty()) {
    throw std::invalid_argument("ConnectedComponents requires non-empty image");
  }

  std::vector<uint8_t> visited(image.data.size(), 0);
  std::vector<ConnectedComponent> components;
  int label = 1;

  const auto index = [&image](int x, int y) {
    return static_cast<std::size_t>(y) * image.width + x;
  };

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      const std::size_t idx = index(x, y);
      if (visited[idx] || image.data[idx] == 0) {
        continue;
      }

      ConnectedComponent component;
      component.label = label++;
      component.bounds = RectI{x, y, 1, 1};
      component.area = 0;

      std::queue<PointI> queue;
      queue.push(PointI{x, y});
      visited[idx] = 1;

      while (!queue.empty()) {
        const PointI current = queue.front();
        queue.pop();
        ++component.area;

        if (current.x < component.bounds.x) {
          component.bounds.width += component.bounds.x - current.x;
          component.bounds.x = current.x;
        }
        if (current.y < component.bounds.y) {
          component.bounds.height += component.bounds.y - current.y;
          component.bounds.y = current.y;
        }
        if (current.x >= component.bounds.x + component.bounds.width) {
          component.bounds.width = current.x - component.bounds.x + 1;
        }
        if (current.y >= component.bounds.y + component.bounds.height) {
          component.bounds.height = current.y - component.bounds.y + 1;
        }

        const std::array<PointI, 4> neighbors{{
            PointI{current.x - 1, current.y},
            PointI{current.x + 1, current.y},
            PointI{current.x, current.y - 1},
            PointI{current.x, current.y + 1},
        }};

        for (const PointI& nb : neighbors) {
          if (nb.x < 0 || nb.x >= image.width || nb.y < 0 || nb.y >= image.height) {
            continue;
          }
          const std::size_t nb_idx = index(nb.x, nb.y);
          if (visited[nb_idx] || image.data[nb_idx] == 0) {
            continue;
          }
          visited[nb_idx] = 1;
          queue.push(nb);
        }
      }

      components.push_back(component);
    }
  }

  return components;
}

}  // namespace falcon::core
