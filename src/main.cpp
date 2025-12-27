#include "cgp.hpp"
#include "csv.hpp"
#include "benchmarks.hpp"
#include "symbolic_simplifier.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

std::string simplify_expression(const std::string& expr) {
    std::string cmd = "echo \"" + expr + "\" | python3 src/simplify_expression.py";

    std::array<char, 256> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed");

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        result += buffer.data();

    pclose(pipe);

    return result;
}




static Dataset make_synthetic() {
  Dataset d;
  d.X.reserve(250);
  d.Y.reserve(250);

  // 1 input (x), 2 outputs:
  // y0 = sin(x) + x^2
  // y1 = -x
  for (int i = 0; i < 250; ++i) {
    double x = -2.5 + 5.0 * (double)i / 249.0;
    d.X.push_back({x});
    d.Y.push_back({std::sin(x) + x * x, -x});
  }
  return d;
}

static Dataset load_dataset_csv(const std::string& path) {
  auto rows = CSV::load_numeric(path);
  if (rows.empty()) return {};

  // Convention:
  // first n_inputs columns are X, remaining are Y
  // For this demo, infer n_inputs=1 if 2+ columns; else error.
  Dataset d;
  int cols = (int)rows[0].size();
  if (cols < 2) throw std::runtime_error("CSV needs at least 2 columns (X..., Y...)");

  int n_inputs = 1;
  int n_outputs = cols - n_inputs;

  d.X.reserve(rows.size());
  d.Y.reserve(rows.size());
  for (auto &r : rows) {
    d.X.push_back({r[0]});
    std::vector<double> y;
    y.reserve(n_outputs);
    for (int j = 1; j < cols; ++j) y.push_back(r[j]);
    d.Y.push_back(std::move(y));
  }
  return d;
}
static std::string argval(int& i, int argc, char** argv) {
  if (i+1 >= argc) throw std::runtime_error(std::string("Missing value for ") + argv[i]);
  return argv[++i];
}

int main(int argc, char** argv) {
  std::string bench = "Nguyen-10";
  int ntrain = -1, ntest = -1;
  unsigned long long seed =  std::random_device{}();
  bool list = false;

  for (int i=1;i<argc;++i) {
    std::string a = argv[i];
    if (a == "--benchmark") bench = argval(i, argc, argv);
    else if (a == "--train") ntrain = std::stoi(argval(i, argc, argv));
    else if (a == "--test")  ntest  = std::stoi(argval(i, argc, argv));
    else if (a == "--seed")  seed   = std::stoull(argval(i, argc, argv));
    else if (a == "--list-benchmarks") list = true;
  }

  if (list) {
    for (auto& n : benchmark_names()) std::cout << n << "\n";
    return 0;
  }

  Dataset train, test;
  CGPConfig cfg;

  if (bench == "synthetic") {

      train = make_synthetic();
      test  = make_synthetic();
      std::cout << "Using synthetic dataset\n";

      cfg.n_inputs = (int)train.n_inputs();
      cfg.n_outputs = (int)train.n_outputs();
  } else {
      auto b = get_benchmark(bench);
  
      if (ntrain <= 0) ntrain = b.default_train;
      if (ntest  <= 0) ntest  = b.default_test;
  
      train = make_benchmark_dataset(b, ntrain, seed);
      test  = make_benchmark_dataset(b, ntest, seed + 1);
  
      std::cout << "Benchmark: " << b.name
                << " (inputs=" << b.n_inputs << ")\n";
      cfg.n_inputs = b.n_inputs;
      cfg.n_outputs = b.n_outputs;
  }

  std::cout << "Train=" << train.n_samples() << " Test=" << test.n_samples() << "\n";

  cfg.seed = std::random_device{}();

  cfg.function_set = {
    Op::ADD,
    Op::SUB,
    Op::MUL,
    Op::DIV,
    Op::NEG,
    Op::POW,
    Op::SIN,
    Op::COS,
    Op::TAN,
    Op::ASIN,
    Op::ACOS,
    Op::ATAN,
    Op::ITE
  };

  
  cfg.op_weights = {
      6.0,  // ADD
      4.0,  // SUB
      6.0,  // MUL
      1.0,  // DIV
      1.0,  // NEG 
      0.3,  // POW
      1.5,  // SIN
      0.5,  // COS
      0.1,  // TAN
      0.05,  // ASIN
      0.05,  // ACOS
      0.05,   // ATAN
      0.0   //  ITE
  };

  cfg.n_columns = 90;
  cfg.levels_back = 25;

  cfg.use_constants = true;
  cfg.n_constants = 6;
  cfg.const_min = -5.0;
  cfg.const_max = 5.0;

  cfg.use_erc = true;
  cfg.erc_min = -1.0;
  cfg.erc_max = 1.0;

  cfg.lambda = 20;
  cfg.generations = 200;

  cfg.mutation_rate = 0.06;
  cfg.const_mutation_rate = 0.15;
  cfg.const_mutation_sigma = 0.35;

  cfg.complexity_alpha = 1e-3; // annealed in evolve()
  cfg.reuse_beta = 1e-4;

  cfg.normalize_mse = true;

  CGP cgp(cfg);
  Genome best = cgp.evolve(train);
  std::cout << "Best fitness: " << best.fitness << "\n";
  std::cout << "\nConvergence:\n";
  //cgp.plot_convergence(std::cout);

  std::ofstream hist("history.csv");
  hist << "generation,fitness\n";
  const auto& h = cgp.fitness_history();
  for (size_t i = 0; i < h.size(); ++i) {
    hist << i << "," << h[i] << "\n";
  }

  for (int o = 0; o < cfg.n_outputs; ++o) {
      std::string expr = cgp.to_expression(best, o);
      std::cout << "Raw expression for "<<o<<":\n" << expr << "\n";
      //auto simplified_py = simplify_expression(expr);
      //std::cout << "Simplified Python expression:\n" << simplified_py<< "\n";
      //auto simplified_internal = cgp.simplify_expr(expr);
      //std::cout << "Simplified C++ expression:\n" << simplified_internal<< "\n";
      std::string simplified = simplify_expr(expr);
      std::cout << "Simplified C++ expression:\n" << simplified << "\n";
      //std::cout << "Output " << o << " expr:\n" << expr << "\n";
  }




  // ---- Print predictions (your old code, unchanged) ----
  std::cout << "\nSample predictions:\n";
  for (int i = 0; i < 5 && i < (int)test.n_samples(); ++i) {
      auto pred = cgp.forward(best, test.X[i]);
  
      std::cout << "x=" << test.X[i][0] << " pred=[";
      for (int j = 0; j < (int)pred.size(); ++j) {
          std::cout << pred[j];
          if (j + 1 < (int)pred.size()) std::cout << ", ";
      }
      std::cout << "] true=[";
  
      for (int j = 0; j < (int)test.Y[i].size(); ++j) {
          std::cout << test.Y[i][j];
          if (j + 1 < (int)test.Y[i].size()) std::cout << ", ";
      }
      std::cout << "]\n";
  }

  // optional: report test MSE
  Genome tmp = best;
  double test_fit = cgp.evaluate_fitness(tmp, test);
  std::cout << "Test fitness: " << test_fit << "\n";

}

int main_previous(int argc, char** argv) {
  Dataset data;
  if (argc >= 2) {
    data = load_dataset_csv(argv[1]);
    std::cout << "Using CSV dataset: samples=" << data.n_samples() << "\n";
  } else {
    data = make_synthetic();
    std::cout << "Using synthetic dataset: samples=" << data.n_samples() << "\n";
  }

  CGPConfig cfg;
  cfg.n_inputs = (int)data.n_inputs();
  cfg.n_outputs = (int)data.n_outputs();

  cfg.n_columns = 90;
  cfg.levels_back = 25;

  cfg.use_constants = true;
  cfg.n_constants = 6;
  cfg.const_min = -5.0;
  cfg.const_max = 5.0;

  cfg.use_erc = true;
  cfg.erc_min = -1.0;
  cfg.erc_max = 1.0;

  cfg.lambda = 20;
  cfg.generations = 8000;

  cfg.mutation_rate = 0.06;
  cfg.const_mutation_rate = 0.15;
  cfg.const_mutation_sigma = 0.35;

  cfg.complexity_alpha = 1e-3; // annealed in evolve()
  cfg.reuse_beta = 1e-4;

  cfg.normalize_mse = true;
  cfg.seed = 123456789ULL;
  cfg.seed = std::random_device{}();



  // Function set (explicit)
  cfg.function_set = {
    Op::ADD, Op::SUB, Op::MUL, Op::DIV,
    Op::NEG, Op::POW,
    Op::SIN, Op::COS, Op::TAN,
    Op::ASIN, Op::ACOS, Op::ATAN,
    Op::ITE
  };

  CGP cgp(cfg);
  Genome best = cgp.evolve(data);

  std::cout << "Best fitness: " << best.fitness << "\n";
  for (int o = 0; o < cfg.n_outputs; ++o) {
    std::string expr = cgp.to_expression(best, o);
    expr = cgp.simplify_expr(expr);
    std::cout << "Output " << o << " expr:\n" << expr << "\n";
  }

  // Print a few sample predictions
  for (int i = 0; i < 5 && i < (int)data.n_samples(); ++i) {
    double x = data.X[i][0];
    auto pred = cgp.forward(best, data.X[i]);
    std::cout << "x=" << x << " pred=[";
    for (int j = 0; j < (int)pred.size(); ++j) {
      std::cout << pred[j] << (j+1<(int)pred.size()? ", " : "");
    }
    std::cout << "] true=[";
    for (int j = 0; j < (int)data.Y[i].size(); ++j) {
      std::cout << data.Y[i][j] << (j+1<(int)data.Y[i].size()? ", " : "");
    }
    std::cout << "]\n";
  }

  return 0;
}
