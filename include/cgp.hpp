#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <ostream>
#include "functions.hpp"
#include "rng.hpp"

struct Dataset {
  // X: n_samples x n_inputs
  // Y: n_samples x n_outputs
  std::vector<std::vector<double>> X;
  std::vector<std::vector<double>> Y;

  size_t n_samples() const { return X.size(); }
  size_t n_inputs()  const { return X.empty() ? 0 : X[0].size(); }
  size_t n_outputs() const { return Y.empty() ? 0 : Y[0].size(); }
};

struct CGPConfig {
  // Problem dimensions
  int n_inputs  = 1;
  int n_outputs = 1;

  // Graph
  int n_columns   = 90;   // number of nodes
  int levels_back = 25;

  // Function set
  std::vector<Op> function_set;
  std::vector<double> op_weights;   // same size as function_set

  // Global constants
  bool use_constants = true;
  int  n_constants   = 6;
  double const_min   = -5.0;
  double const_max   =  5.0;

  // ERCs (per-node constants as sources)
  bool use_erc   = true;
  double erc_min = -3.0;
  double erc_max =  3.0;

  // Evolution
  int lambda      = 10;
  int generations = 4000;
  int stagnation_generations = 0; // 0 disables random restarts

  // Mutation
  double mutation_rate        = 0.06; // operator/wiring/output
  double const_mutation_rate  = 0.15; // globals + ERC
  double const_mutation_sigma = 0.35; // step size

  // Regularization
  double complexity_alpha = 1e-4; // MSE + alpha * (#active_nodes)  (annealed)
  double reuse_beta       = 1e-4; // fanout penalty

  // Fitness details
  bool normalize_mse = true;

  // RNG
  uint64_t seed = 123456789ULL;



};

struct NodeGene {
  Op op{};
  // Fixed-arity inputs for cache-friendly evaluation.
  int in0 = 0;
  int in1 = 0;
  int in2 = 0;
};

struct Genome {
  std::vector<NodeGene> nodes;
  std::vector<int> outputs;

  std::vector<double> constants; // size n_constants (if enabled)
  std::vector<double> erc;       // size n_columns   (if enabled)

  double fitness = 1e300;
};

class CGP {
public:
  explicit CGP(CGPConfig cfg);

  Genome evolve(const Dataset& data);
  std::vector<double> forward(const Genome& g, const std::vector<double>& x) const;
  void forward( const Genome& g, const double* x, double* out, std::vector<double>& node_out) const;

  void plot_convergence(std::ostream& os, int width=60, int height=15) const;
  const std::vector<double>& fitness_history() const { return fitness_history_; }

  std::string to_expression(const Genome& g, int out_index, bool print_constants=false) const;

  std::string simplify_expr(const std::string& expr);
  double evaluate_fitness(Genome& g, const Dataset& data, double anneal01=0.0) const;

private:
  CGPConfig cfg_;
  RNG rng_;
  std::vector<double> fitness_history_;
  double input_usage_penalty(const Genome& g) const;
  Op sample_operator();


  int count_used_inputs(const Genome& g) const;
  int num_nodes() const { return cfg_.n_columns; }

  // Sources are indexed as:
  // [0 .. n_inputs-1] inputs
  // [n_inputs .. n_inputs+n_constants-1] global constants (optional)
  // [.. + n_nodes-1] ERC per node (optional)
  // [.. + n_nodes-1] previous node outputs (levels-back constrained)
  int base_sources() const;

  int max_source_for_node(int node_idx) const;
  int min_source_for_node(int node_idx) const;
  int random_source_for_node(int node_idx);


  Genome random_genome();

  void mark_active(const Genome& g, std::vector<char>& active) const;
  int  count_active_nodes(const Genome& g) const;
  double reuse_penalty(const Genome& g) const;

  double get_source_value(const Genome& g,
                          const std::vector<double>& node_out,
                          const std::vector<double>& x,
                          int src) const;

  std::vector<double> target_variance(const Dataset& data) const;

  void mutate(Genome& g);
};
