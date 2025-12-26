#pragma once
#include <random>
#include <cstdint>

struct RNG {
  mutable std::mt19937_64 eng;

  explicit RNG(uint64_t seed = 0xC0FFEEULL) : eng(seed) {}

  int randint(int lo, int hi) const {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(eng);
  }

  double rand01() const {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(eng);
  }

  double randreal(double lo, double hi) const {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(eng);
  }

  bool coin(double p) const { return rand01() < p; }
};
