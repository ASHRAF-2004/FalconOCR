#pragma once

#include <chrono>

namespace falcon::util {

class ScopedTimer {
 public:
  using Clock = std::chrono::steady_clock;

  explicit ScopedTimer(double& accumulator)
      : accumulator_(accumulator), start_(Clock::now()) {}

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;

  ~ScopedTimer() {
    const auto end = Clock::now();
    const std::chrono::duration<double, std::milli> diff = end - start_;
    accumulator_ += diff.count();
  }

 private:
  double& accumulator_;
  Clock::time_point start_;
};

}  // namespace falcon::util
