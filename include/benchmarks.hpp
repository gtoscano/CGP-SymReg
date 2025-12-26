#pragma once
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <stdexcept>
#include "cgp.hpp"
#include "csv.hpp"   // uses your Dataset type

struct BenchSpec {
  std::string name;
  int n_inputs = 1;
  int n_outputs = 1;

  // per-input uniform sampling ranges
  std::vector<std::pair<double,double>> ranges;

  // y = f(x)
  std::function<std::vector<double>(const std::vector<double>&)> fn;

  // suggested default sizes (you can override by CLI)
  int default_train = 200;
  int default_test  = 2000;
};




// List available benchmark names
std::vector<std::string> benchmark_names();

// Lookup by name (throws if not found)
BenchSpec get_benchmark(const std::string& name);

// Generate train/test datasets (uniform sampling with seed)
Dataset make_benchmark_dataset(const BenchSpec& b, int n_samples, unsigned long long seed);
