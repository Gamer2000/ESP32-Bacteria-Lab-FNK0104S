#pragma once

#include <stdint.h>

namespace BacteriaLab {

class Random {
 public:
  void seed(uint32_t value) { state_ = value ? value : 0xA341316Cu; }

  uint32_t next() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state_ = x;
  }

  float unit() { return static_cast<float>(next() >> 8) * (1.0f / 16777216.0f); }
  float range(float low, float high) { return low + (high - low) * unit(); }
  bool chance(float probability) { return unit() < probability; }

 private:
  uint32_t state_ = 0xA341316Cu;
};

}  // namespace BacteriaLab
