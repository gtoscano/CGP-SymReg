#include "cgp.hpp"
#include <regex>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <functional>

namespace {
template <typename F>
inline void for_each_input(const NodeGene& node, int a, F&& f) {
  if (a >= 1) f(node.in0);
  if (a >= 2) f(node.in1);
  if (a >= 3) f(node.in2);
}
} // namespace

int CGP::count_used_inputs(const Genome& g) const {
    std::vector<char> used(cfg_.n_inputs, 0);
    std::vector<int> stack;

    for (int o = 0; o < cfg_.n_outputs; ++o) {
        int s = g.outputs[o];
        if (s < cfg_.n_inputs) used[s] = 1;
    }

    return std::count(used.begin(), used.end(), 1);
}

Op CGP::sample_operator() {
    double total = 0.0;
    for (double w : cfg_.op_weights) total += w;

    double r = rng_.randreal(0.0, total);
    double acc = 0.0;

    for (size_t i = 0; i < cfg_.function_set.size(); ++i) {
        acc += cfg_.op_weights[i];
        if (r <= acc)
            return cfg_.function_set[i];
    }

    return cfg_.function_set.back(); // fallback
}

double CGP::input_usage_penalty(const Genome& g) const {
  std::vector<char> used(cfg_.n_inputs, 0);

  std::function<void(int)> visit = [&](int src) {
    if (src < cfg_.n_inputs) {
      used[src] = 1;
      return;
    }

    int offset = cfg_.n_inputs;
    if (cfg_.use_constants) offset += cfg_.n_constants;
    if (cfg_.use_erc) offset += num_nodes();

    int ni = src - offset;
    if (ni < 0 || ni >= num_nodes()) return;

    const auto& node = g.nodes[ni];
    for_each_input(node, arity(node.op), visit);
  };

  for (int o = 0; o < cfg_.n_outputs; ++o)
    visit(g.outputs[o]);

  int count = 0;
  for (bool u : used) if (u) count++;

  return (count == 0) ? 1.0 : 0.0;
}


CGP::CGP(CGPConfig cfg)
  : cfg_(std::move(cfg)), rng_(cfg_.seed) {
  // Provide a sensible default function set if empty
  if (cfg_.function_set.empty()) {
    cfg_.function_set = {
      Op::ADD, Op::SUB, Op::MUL, Op::DIV,
      Op::NEG, Op::POW,
      Op::SIN, Op::COS, Op::TAN,
      Op::ASIN, Op::ACOS, Op::ATAN,
      Op::ITE
    };
  }
}

int CGP::base_sources() const {
  int n = cfg_.n_inputs;
  if (cfg_.use_constants) n += cfg_.n_constants;
  if (cfg_.use_erc) n += num_nodes();
  return n;
}

int CGP::max_source_for_node(int node_idx) const {
  // can connect to any input/global const/ERC plus any previous node output
  return base_sources() + node_idx - 1;
}

int CGP::min_source_for_node(int node_idx) const {
  // levels-back applies to node-output sources only; inputs/consts/ERCs always allowed
  int min_node = std::max(0, node_idx - cfg_.levels_back);
  return base_sources() + min_node;
}

int CGP::random_source_for_node(int node_idx) {
    int n_inputs = cfg_.n_inputs;
    int first_node = base_sources();
    int last_node  = first_node + node_idx - 1;

    std::vector<int> candidates;

    // Always allow inputs
    for (int i = 0; i < n_inputs; ++i)
        candidates.push_back(i);

    // Allow previous node outputs only
    if (node_idx > 0) {
        for (int i = first_node; i <= last_node; ++i)
            candidates.push_back(i);
    }

    if (candidates.empty())
        return rng_.randint(0, n_inputs - 1);

    return candidates[rng_.randint(0, (int)candidates.size() - 1)];
}


Genome CGP::random_genome() {
  Genome g;
  g.nodes.resize(num_nodes());
  g.outputs.resize(cfg_.n_outputs);

  if (cfg_.use_constants) {
    g.constants.resize(cfg_.n_constants);
    for (auto &c : g.constants) c = rng_.randreal(cfg_.const_min, cfg_.const_max);
  }
  if (cfg_.use_erc) {
    g.erc.resize(num_nodes());
    for (auto &e : g.erc) e = rng_.randreal(cfg_.erc_min, cfg_.erc_max);
  }

  for (int i = 0; i < num_nodes(); ++i) {
    NodeGene ng;
    //ng.op = cfg_.function_set[rng_.randint(0, (int)cfg_.function_set.size() - 1)];
    ng.op = sample_operator();

    const int a = arity(ng.op);
    if (a >= 1) ng.in0 = random_source_for_node(i);
    if (a >= 2) ng.in1 = random_source_for_node(i);
    if (a >= 3) ng.in2 = random_source_for_node(i);
    g.nodes[i] = std::move(ng);
  }

  //const int max_out = base_sources() + num_nodes() - 1;
  int first_node = base_sources();  // inputs + constants + ERCs
  int last_node  = base_sources() + num_nodes() - 1;
  for (int o = 0; o < cfg_.n_outputs; ++o) 
    g.outputs[o] = rng_.randint(first_node, last_node);
    //g.outputs[o] = rng_.randint(0, max_out);

  g.fitness = 1e300;
  return g;
}

void CGP::mark_active(const Genome& g, std::vector<char>& active) const {
  active.assign(num_nodes(), 0);
  std::vector<int> stack;

  for (int o = 0; o < cfg_.n_outputs; ++o) {
    int src = g.outputs[o];
    if (src >= base_sources()) {
      int ni = src - base_sources();
      if (0 <= ni && ni < num_nodes()) stack.push_back(ni);
    }
  }

  while (!stack.empty()) {
    int ni = stack.back();
    stack.pop_back();
    if (active[ni]) continue;
    active[ni] = 1;

    const auto& node = g.nodes[ni];
    auto push_prev = [&](int s) {
      if (s >= base_sources()) {
        int pj = s - base_sources();
        if (0 <= pj && pj < num_nodes()) stack.push_back(pj);
      }
    };
    for_each_input(node, arity(node.op), push_prev);
  }
}

int CGP::count_active_nodes(const Genome& g) const {
  std::vector<char> active;
  mark_active(g, active);
  int c = 0;
  for (char a : active) c += (a ? 1 : 0);
  return c;
}

double CGP::reuse_penalty(const Genome& g) const {
  // Penalize fan-out: sum(max(0, fanout(node)-1)) over active nodes
  std::vector<char> active;
  mark_active(g, active);

  std::vector<int> fan(num_nodes(), 0);

  auto add_ref = [&](int src) {
    if (src >= base_sources()) {
      int ni = src - base_sources();
      if (0 <= ni && ni < num_nodes()) fan[ni] += 1;
    }
  };

  for (int o = 0; o < cfg_.n_outputs; ++o) add_ref(g.outputs[o]);

  for (int i = 0; i < num_nodes(); ++i) {
    if (!active[i]) continue;
    const auto& node = g.nodes[i];
    for_each_input(node, arity(node.op), add_ref);
  }

  double pen = 0.0;
  for (int i = 0; i < num_nodes(); ++i) {
    if (!active[i]) continue;
    if (fan[i] > 1) pen += (double)(fan[i] - 1);
  }
  return pen;
}

double CGP::get_source_value(
  const Genome& g,
  const std::vector<double>& node_out,
  const std::vector<double>& x,
  int src
) const {
  // inputs
  if (src < cfg_.n_inputs) return x[src];

  int offset = cfg_.n_inputs;

  // global constants
  if (cfg_.use_constants) {
    if (src < offset + cfg_.n_constants) {
      return g.constants[src - offset];
    }
    offset += cfg_.n_constants;
  }

  // ERC per node
  if (cfg_.use_erc) {
    if (src < offset + num_nodes()) {
      return g.erc[src - offset];
    }
    offset += num_nodes();
  }

  // node outputs
  int ni = src - offset;
  if (0 <= ni && ni < num_nodes()) return node_out[ni];
  return 0.0;
}

std::vector<double> CGP::forward(const Genome& g, const std::vector<double>& x) const {
  std::vector<double> node_out(num_nodes(), 0.0);

  for (int i = 0; i < num_nodes(); ++i) {
    const auto& node = g.nodes[i];
    const int a = arity(node.op);
    const double v0 = (a >= 1) ? get_source_value(g, node_out, x, node.in0) : 0.0;
    const double v1 = (a >= 2) ? get_source_value(g, node_out, x, node.in1) : 0.0;
    const double v2 = (a >= 3) ? get_source_value(g, node_out, x, node.in2) : 0.0;
    node_out[i] = eval_op(node.op, v0, v1, v2);
  }

  std::vector<double> out(cfg_.n_outputs, 0.0);
  for (int o = 0; o < cfg_.n_outputs; ++o) {
    out[o] = get_source_value(g, node_out, x, g.outputs[o]);
  }
  return out;
}

void CGP::forward(const Genome& g, const double* x, double* out, std::vector<double>& node_out) const {
  if ((int)node_out.size() != num_nodes()) {
    node_out.assign(num_nodes(), 0.0);
  } else {
    std::fill(node_out.begin(), node_out.end(), 0.0);
  }

  auto get_source_value_ptr = [&](int src) -> double {
    if (src < cfg_.n_inputs) return x[src];
    int offset = cfg_.n_inputs;

    if (cfg_.use_constants) {
      if (src < offset + cfg_.n_constants) return g.constants[src - offset];
      offset += cfg_.n_constants;
    }

    if (cfg_.use_erc) {
      if (src < offset + num_nodes()) return g.erc[src - offset];
      offset += num_nodes();
    }

    int ni = src - offset;
    if (0 <= ni && ni < num_nodes()) return node_out[ni];
    return 0.0;
  };

  for (int i = 0; i < num_nodes(); ++i) {
    const auto& node = g.nodes[i];
    const int a = arity(node.op);
    const double v0 = (a >= 1) ? get_source_value_ptr(node.in0) : 0.0;
    const double v1 = (a >= 2) ? get_source_value_ptr(node.in1) : 0.0;
    const double v2 = (a >= 3) ? get_source_value_ptr(node.in2) : 0.0;
    node_out[i] = eval_op(node.op, v0, v1, v2);
  }

  for (int o = 0; o < cfg_.n_outputs; ++o) {
    out[o] = get_source_value_ptr(g.outputs[o]);
  }
}


std::vector<double> CGP::target_variance(const Dataset& data) const {
  std::vector<double> var(cfg_.n_outputs, 1.0);
  if (data.n_samples() == 0) return var;

  std::vector<double> mean(cfg_.n_outputs, 0.0);
  for (size_t i = 0; i < data.n_samples(); ++i) {
    for (int j = 0; j < cfg_.n_outputs; ++j) mean[j] += data.Y[i][j];
  }
  for (int j = 0; j < cfg_.n_outputs; ++j) mean[j] /= (double)data.n_samples();

  for (size_t i = 0; i < data.n_samples(); ++i) {
    for (int j = 0; j < cfg_.n_outputs; ++j) {
      double d = data.Y[i][j] - mean[j];
      var[j] += d * d;
    }
  }
  for (int j = 0; j < cfg_.n_outputs; ++j) {
    var[j] /= (double)data.n_samples();
    if (var[j] < 1e-12) var[j] = 1.0;
  }
  return var;
}

double CGP::evaluate_fitness(Genome& g, const Dataset& data, double anneal01) const {
    // Guard checks
    if (data.n_samples() == 0) return g.fitness = 1e300;
    if ((int)data.n_inputs() != cfg_.n_inputs) return g.fitness = 1e300;
    if ((int)data.n_outputs() != cfg_.n_outputs) return g.fitness = 1e300;
    const double prev_fitness = g.fitness;

    // ---- Mean Squared Error ----
    std::vector<double> var(cfg_.n_outputs, 1.0);
    if (cfg_.normalize_mse) {
        var = target_variance(data);
    }

    double se = 0.0;
    for (size_t i = 0; i < data.n_samples(); ++i) {
        auto pred = forward(g, data.X[i]);
        for (int j = 0; j < cfg_.n_outputs; ++j) {
            if (!std::isfinite(pred[j])) {
              return g.fitness = 1e12;  // hard kill
            }
            if (std::abs(pred[j]) > 1e6)
              pred[j] = std::copysign(1e6, pred[j]);

            double e = pred[j] - data.Y[i][j];
            se += (e * e) / var[j];
        }
    }

    double mse = se / (double)(data.n_samples() * cfg_.n_outputs);

    // ----------------------------------------------------
    // Regularization terms
    // ----------------------------------------------------

    double fitness = mse;

    // Complexity penalty (annealed)
    if (cfg_.complexity_alpha > 0.0) {
        double complexity = (double)count_active_nodes(g);
        double weight = cfg_.complexity_alpha * anneal01;
        fitness += weight * complexity;
    }

    // Reuse / fan-out penalty
    if (cfg_.reuse_beta > 0.0) {
        fitness += cfg_.reuse_beta * reuse_penalty(g);
    }

    // ----------------------------------------------------
    // Improvement reward (anti-stagnation)
    // ----------------------------------------------------
    if (prev_fitness < 1e29) {
        double improvement = prev_fitness - fitness;
        if (improvement > 0.0) {
            // reward improvement but do not overshoot
            //fitness -= 0.5 * improvement;
            double reward = std::min(improvement, 0.5 * prev_fitness);
            reward = std::min(reward, 0.5 * fitness);
            fitness -= reward;

        }
    }

    // floor to avoid runaway negatives
    fitness = std::max(fitness, 1e-12);

    g.fitness = fitness;
    return fitness;
}


void CGP::mutate(Genome& g) {
    std::vector<char> active;
    mark_active(g, active);

    auto mutate_node = [&](int i) {
        NodeGene& node = g.nodes[i];

        if (rng_.coin(cfg_.mutation_rate)) {
            node.op = sample_operator();
            int a = arity(node.op);
            if (a >= 1) node.in0 = random_source_for_node(i);
            if (a >= 2) node.in1 = random_source_for_node(i);
            if (a >= 3) node.in2 = random_source_for_node(i);
        }

        int a = arity(node.op);
        if (a >= 1 && rng_.coin(cfg_.mutation_rate)) node.in0 = random_source_for_node(i);
        if (a >= 2 && rng_.coin(cfg_.mutation_rate)) node.in1 = random_source_for_node(i);
        if (a >= 3 && rng_.coin(cfg_.mutation_rate)) node.in2 = random_source_for_node(i);
    };

    for (int i = 0; i < num_nodes(); ++i) {
        if (active[i] || rng_.coin(0.05))
            mutate_node(i);
    }

    if (cfg_.use_constants) {
        for (double& c : g.constants) {
            if (rng_.coin(cfg_.const_mutation_rate))
                c = std::clamp(c + rng_.randreal(-1.0, 1.0) * cfg_.const_mutation_sigma,
                               cfg_.const_min, cfg_.const_max);
        }
    }

    if (cfg_.use_erc) {
        for (double& e : g.erc) {
            if (rng_.coin(cfg_.const_mutation_rate))
                e = std::clamp(e + rng_.randreal(-1.0, 1.0) * cfg_.const_mutation_sigma,
                               cfg_.erc_min, cfg_.erc_max);
        }
    }

    // Output mutation (important!)
    int node_start = base_sources();
    int node_end   = base_sources() + num_nodes() - 1;

    for (int o = 0; o < cfg_.n_outputs; ++o) {
        if (rng_.coin(cfg_.mutation_rate))
            g.outputs[o] = rng_.randint(node_start, node_end);
    }
}


Genome CGP::evolve(const Dataset& data) {
  Genome parent = random_genome();
  evaluate_fitness(parent, data, 0.0);

  Genome best = parent;

  fitness_history_.clear();
  fitness_history_.reserve(cfg_.generations + 1);
  fitness_history_.push_back(parent.fitness);

  for (int gen = 0; gen < cfg_.generations; ++gen) {
    const double anneal = (cfg_.generations <= 1) ? 1.0
                      : (double)gen / (double)(cfg_.generations - 1);

    Genome gen_best = parent;

    for (int k = 0; k < cfg_.lambda; ++k) {
      Genome child = parent;
      mutate(child);
      evaluate_fitness(child, data, anneal);
      if (child.fitness < gen_best.fitness) gen_best = std::move(child);
    }

    if (gen_best.fitness <= parent.fitness) parent = gen_best;
    if (parent.fitness < best.fitness) best = parent;

    fitness_history_.push_back(best.fitness);
  }

  return best;
}

void CGP::plot_convergence(std::ostream& os, int width, int height) const {
  if (fitness_history_.empty()) {
    os << "No convergence history available.\n";
    return;
  }

  if (width < 2) width = 2;
  if (height < 2) height = 2;

  const size_t n = fitness_history_.size();
  const auto mm = std::minmax_element(fitness_history_.begin(), fitness_history_.end());
  double vmin = *mm.first;
  double vmax = *mm.second;
  double range = vmax - vmin;
  if (range <= 0.0) range = 1.0;

  std::vector<std::string> rows(height, std::string(width, ' '));
  for (int col = 0; col < width; ++col) {
    double t = (width == 1) ? 0.0 : (double)col / (double)(width - 1);
    size_t idx = (size_t)std::llround(t * (double)(n - 1));
    double v = fitness_history_[idx];
    double y = (vmax - v) / range;
    int row = (int)std::llround(y * (double)(height - 1));
    row = std::max(0, std::min(height - 1, row));
    rows[row][col] = '*';
  }

  for (int r = 0; r < height; ++r) {
    os << rows[r] << "\n";
  }
  os << "min=" << std::setprecision(6) << vmin
     << " max=" << std::setprecision(6) << vmax
     << " generations=" << (n > 0 ? n - 1 : 0) << "\n";
}

std::string CGP::to_expression(const Genome& g, int out_index, bool print_constants) const {
  std::vector<std::string> memo(num_nodes());
  std::vector<char> visiting(num_nodes(), 0);

  std::function<std::string(int)> rec = [&](int src) -> std::string {
    if (src < cfg_.n_inputs) return "x" + std::to_string(src);

    int offset = cfg_.n_inputs;

    if (cfg_.use_constants) {
      if (src < offset + cfg_.n_constants) return "c" + std::to_string(src - offset);
      offset += cfg_.n_constants;
    }

    if (cfg_.use_erc) {
      if (src < offset + num_nodes()) return "e" + std::to_string(src - offset);
      offset += num_nodes();
    }

    int ni = src - offset;
    if (ni < 0 || ni >= num_nodes()) return "0";
    if (!memo[ni].empty()) return memo[ni];
    if (visiting[ni]) return "0";

    visiting[ni] = 1;
    const auto& node = g.nodes[ni];

    const int a = arity(node.op);
    const std::string a0 = (a >= 1) ? rec(node.in0) : "0";
    const std::string a1 = (a >= 2) ? rec(node.in1) : "0";
    const std::string a2 = (a >= 3) ? rec(node.in2) : "0";

    std::string expr;
    switch (node.op) {
      case Op::NEG: expr = "(-" + a0 + ")"; break;
      case Op::SIN: expr = "sin(" + a0 + ")"; break;
      case Op::COS: expr = "cos(" + a0 + ")"; break;
      case Op::TAN: expr = "tan(" + a0 + ")"; break;
      case Op::ASIN: expr = "asin(" + a0 + ")"; break;
      case Op::ACOS: expr = "acos(" + a0 + ")"; break;
      case Op::ATAN: expr = "atan(" + a0 + ")"; break;
      case Op::POW: expr = "pow(" + a0 + "," + a1 + ")"; break;
      case Op::ITE: expr = "ite(" + a0 + "," + a1 + "," + a2 + ")"; break;
      default:      expr = "(" + a0 + " " + std::string(op_name(node.op)) + " " + a1 + ")"; break;
    }

    visiting[ni] = 0;
    memo[ni] = expr;
    return expr;
  };

  std::ostringstream oss;
  oss << rec(g.outputs[out_index]);
  if (print_constants) {
    if (cfg_.use_constants && !g.constants.empty()) {
      oss << "\n// global constants: ";
      for (size_t i = 0; i < g.constants.size(); ++i) {
        oss << "c" << i << "=" << std::setprecision(6) << g.constants[i];
        if (i + 1 < g.constants.size()) oss << ", ";
      }
    }
    if (cfg_.use_erc && !g.erc.empty()) {
      oss << "\n// ERCs (per node): e0..e" << (g.erc.size() - 1);
    }
  }

  return oss.str();
}



std::string CGP::simplify_expr(const std::string& expr) {
  std::string s = expr;
  bool changed = true;

  auto trim = [](std::string& str) {
    str.erase(0, str.find_first_not_of(" \t"));
    str.erase(str.find_last_not_of(" \t") + 1);
  };

  while (changed) {
    changed = false;

    auto replace = [&](const std::string& from, const std::string& to) {
      size_t pos = 0;
      while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        changed = true;
      }
    };

    // Basic algebraic simplifications
    replace("(x0 * 1)", "x0");
    replace("(1 * x0)", "x0");
    replace("(x0 + 0)", "x0");
    replace("(0 + x0)", "x0");
    replace("(x0 - 0)", "x0");
    replace("(x0 / 1)", "x0");

    // Constant folding
    replace("(0 + 0)", "0");
    replace("(1 * 1)", "1");

    // Remove redundant parentheses
    if (s.size() > 2 && s.front() == '(' && s.back() == ')') {
      int depth = 0;
      bool ok = true;
      for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') depth--;
        if (depth == 0 && i != s.size() - 1) {
          ok = false;
          break;
        }
      }
      if (ok) {
        s = s.substr(1, s.size() - 2);
        changed = true;
      }
    }
  }
  trim(s);


  return s;
}
