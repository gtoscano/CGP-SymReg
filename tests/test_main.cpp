#include "cgp.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <stdexcept>

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
  cfg.stagnation_generations = 100;
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

  // Fitness must not depend on the value already stored in the genome.
  Genome reevaluated = best;
  const double fitness1 = cgp.evaluate_fitness(reevaluated, data, 1.0);
  reevaluated.fitness = fitness1 * 100.0;
  const double fitness2 = cgp.evaluate_fitness(reevaluated, data, 1.0);
  assert(std::abs(fitness1 - fitness2) < 1e-12);

  // Missing weights mean uniform sampling, and a fixed seed is reproducible.
  CGP cgp_again(cfg);
  Genome best_again = cgp_again.evolve(data);
  assert(best.fitness == best_again.fitness);
  for (int o = 0; o < cfg.n_outputs; ++o)
    assert(cgp.to_expression(best, o) == cgp_again.to_expression(best_again, o));

  // levels_back=1 permits primary sources and only the immediately prior node.
  CGPConfig wiring_cfg;
  wiring_cfg.n_inputs = 1;
  wiring_cfg.n_outputs = 1;
  wiring_cfg.n_columns = 50;
  wiring_cfg.levels_back = 1;
  wiring_cfg.use_constants = false;
  wiring_cfg.use_erc = false;
  wiring_cfg.generations = 0;
  wiring_cfg.function_set = {Op::ADD};
  wiring_cfg.seed = 7;
  CGP wiring_cgp(wiring_cfg);
  Genome wiring = wiring_cgp.evolve(data);
  for (int i = 0; i < wiring_cfg.n_columns; ++i) {
    assert(wiring.nodes[i].in0 == 0 || (i > 0 && wiring.nodes[i].in0 == i));
    assert(wiring.nodes[i].in1 == 0 || (i > 0 && wiring.nodes[i].in1 == i));
  }

  // Constants and ERCs must be available as node inputs.
  CGPConfig source_cfg = wiring_cfg;
  source_cfg.n_constants = 8;
  source_cfg.use_constants = true;
  source_cfg.use_erc = true;
  source_cfg.seed = 11;
  CGP source_cgp(source_cfg);
  Genome sources = source_cgp.evolve(data);
  const int primary_end = source_cfg.n_inputs + source_cfg.n_constants + source_cfg.n_columns;
  bool saw_constant = false;
  bool saw_erc = false;
  for (const auto& node : sources.nodes) {
    for (int source : {node.in0, node.in1}) {
      saw_constant |= source >= source_cfg.n_inputs &&
                      source < source_cfg.n_inputs + source_cfg.n_constants;
      saw_erc |= source >= source_cfg.n_inputs + source_cfg.n_constants &&
                 source < primary_end;
    }
  }
  assert(saw_constant);
  assert(saw_erc);

  bool rejected_bad_weights = false;
  try {
    CGPConfig bad_cfg;
    bad_cfg.function_set = {Op::ADD, Op::SUB};
    bad_cfg.op_weights = {1.0};
    CGP invalid(bad_cfg);
  } catch (const std::invalid_argument&) {
    rejected_bad_weights = true;
  }
  assert(rejected_bad_weights);

  std::cout << "All tests passed.\n";
  return 0;
}
