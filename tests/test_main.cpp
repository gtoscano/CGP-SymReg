#include "cgp.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

static Dataset make_data() {
  Dataset d;
  for (int i = 0; i < 50; ++i) {
    double x = -2.0 + 4.0 * (double)i / 49.0;
    d.X.push_back({x});
    d.Y.push_back({std::sin(x) + x*x, -x});
  }
  return d;
}

int main() {
  Dataset data = make_data();

  CGPConfig cfg;
  cfg.n_inputs = (int)data.n_inputs();
  cfg.n_outputs = (int)data.n_outputs();
  cfg.n_columns = 60;
  cfg.levels_back = 20;

  cfg.use_constants = true;
  cfg.n_constants = 6;

  cfg.use_erc = true;
  cfg.erc_min = -3.0;
  cfg.erc_max = 3.0;

  cfg.lambda = 8;
  cfg.generations = 800;
  cfg.mutation_rate = 0.08;
  cfg.const_mutation_rate = 0.15;
  cfg.const_mutation_sigma = 0.3;

  cfg.complexity_alpha = 1e-4;
  cfg.reuse_beta = 1e-4;
  cfg.normalize_mse = true;

  cfg.seed = 42;

  cfg.function_set = {
    Op::ADD, Op::SUB, Op::MUL, Op::DIV,
    Op::NEG, Op::SIN, Op::COS, Op::POW
  };

  CGP cgp(cfg);

  Genome best = cgp.evolve(data);
  assert(std::isfinite(best.fitness));
  // Should do better than a terrible baseline most of the time
  assert(best.fitness < 10.0);

  // Forward pass shape
  auto p = cgp.forward(best, data.X[0]);
  assert((int)p.size() == cfg.n_outputs);

  std::cout << "All tests passed.\n";
  return 0;
}

