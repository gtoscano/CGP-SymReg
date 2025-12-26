#include "benchmarks.hpp"
#include <cassert>
#include <iostream>

int main() {
  // deterministic generation with same seed
  auto b = get_benchmark("Nguyen-10");
  auto d1 = make_benchmark_dataset(b, 10, 42);
  auto d2 = make_benchmark_dataset(b, 10, 42);

  assert(d1.n_samples() == 10);
  assert(d1.n_inputs() == (size_t)b.n_inputs);
  assert(d1.n_outputs() == (size_t)b.n_outputs);

  for (int i=0;i<10;++i) {
    for (int j=0;j<b.n_inputs;++j) assert(d1.X[i][j] == d2.X[i][j]);
    assert(d1.Y[i][0] == d2.Y[i][0]);
  }

  // multi-input Korns exists
  auto k = get_benchmark("Korns-8");
  assert(k.n_inputs == 5);

  std::cout << "benchmarks: OK\n";
  return 0;
}
