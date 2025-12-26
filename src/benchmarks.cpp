#include "benchmarks.hpp"
#include <cmath>
#include <unordered_map>
#include <algorithm>

static inline double sq(double x) { return x*x; }
static inline double cube(double x) { return x*x*x; }
static inline double quart(double x) { double y=x*x; return y*y; }

static inline double safe_log(double x) {
  // log(<=0) -> log(eps) (keeps finite)
  const double eps = 1e-12;
  return std::log(std::max(x, eps));
}
static inline double safe_sqrt(double x) {
  return std::sqrt(std::max(x, 0.0));
}

// --- Benchmark registry ---
// Equations aligned with common SR benchmark lists (Nguyen, Keijzer, Korns). :contentReference[oaicite:2]{index=2}
// Korns input ranges often U[-50,50]. :contentReference[oaicite:3]{index=3}
static std::unordered_map<std::string, BenchSpec> mk_registry() {
  std::unordered_map<std::string, BenchSpec> m;

  // --------------------
  // Nguyen (1D + 2D)
  // Typical domains chosen to avoid log/sqrt issues while still standard in practice.
  // --------------------
  // Nguyen-1..4: polynomials, x in [-1,1]
  m["Nguyen-1"] = {"Nguyen-1", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ cube(t)+sq(t)+t }; }, 200, 2000};

  m["Nguyen-2"] = {"Nguyen-2", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ quart(t)+cube(t)+sq(t)+t }; }, 200, 2000};

  m["Nguyen-3"] = {"Nguyen-3", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ std::pow(t,5)+quart(t)+cube(t)+sq(t)+t }; }, 200, 2000};

  m["Nguyen-4"] = {"Nguyen-4", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ std::pow(t,6)+std::pow(t,5)+quart(t)+cube(t)+sq(t)+t }; }, 200, 2000};

  // Nguyen-5..8: use x in [0,4] to keep sqrt/log tame
  m["Nguyen-5"] = {"Nguyen-5", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ std::sin(sq(t))*std::cos(t) - 1.0 }; }, 200, 2000};

  m["Nguyen-6"] = {"Nguyen-6", 1, 1, {{-1, 1}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ std::sin(t) + std::sin(t + sq(t)) }; }, 200, 2000};

  m["Nguyen-7"] = {"Nguyen-7", 1, 1, {{0, 2}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ safe_log(t + 1.0) + safe_log(sq(t) + 1.0) }; }, 200, 2000};

  m["Nguyen-8"] = {"Nguyen-8", 1, 1, {{0, 4}},
    [](const std::vector<double>& x){ double t=x[0]; return std::vector<double>{ safe_sqrt(t) }; }, 200, 2000};

  // Nguyen-9..12: 2D, x,y in [-1,1] (multi-input)
  m["Nguyen-9"] = {"Nguyen-9", 2, 1, {{-1,1},{-1,1}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ std::sin(a) + std::sin(sq(b)) }; }, 200, 2000};

  m["Nguyen-10"] = {"Nguyen-10", 2, 1, {{-1,1},{-1,1}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ 2.0*std::sin(a)*std::cos(b) }; }, 200, 2000};

  // HeuristicLab list labels x^y as Nguyen-11. :contentReference[oaicite:4]{index=4}
  m["Nguyen-11"] = {"Nguyen-11", 2, 1, {{0,2},{0,2}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ std::pow(a, b) }; }, 200, 2000};

  // Nguyen-12 matches Keijzer-13 expression in many lists. :contentReference[oaicite:5]{index=5}
  m["Nguyen-12"] = {"Nguyen-12", 2, 1, {{-1,1},{-1,1}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ std::pow(a,4) - std::pow(a,3) + 0.5*sq(b) - b }; }, 200, 2000};


  // --------------------
  // Keijzer (focus on multi-input ones you asked for)
  // from common lists (Keijzer 11..16 are 2D). :contentReference[oaicite:6]{index=6}
  // --------------------
  m["Keijzer-11"] = {"Keijzer-11", 2, 1, {{0,2},{0,2}},
    [](const std::vector<double>& x){ return std::vector<double>{ std::pow(x[0], x[1]) }; }, 200, 2000};

  m["Keijzer-12"] = {"Keijzer-12", 2, 1, {{-3,3},{-3,3}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ a*b + std::sin((a-1.0)*(b-1.0)) }; }, 200, 2000};

  m["Keijzer-13"] = {"Keijzer-13", 2, 1, {{-1,1},{-1,1}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ std::pow(a,4) - std::pow(a,3) + 0.5*sq(b) - b }; }, 200, 2000};

  m["Keijzer-14"] = {"Keijzer-14", 2, 1, {{-1,1},{-1,1}},
    [](const std::vector<double>& x){ return std::vector<double>{ 6.0*std::sin(x[0])*std::cos(x[1]) }; }, 200, 2000};

  m["Keijzer-15"] = {"Keijzer-15", 2, 1, {{-2,2},{-2,2}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ 8.0 / (2.0 + a*a + b*b) }; }, 200, 2000};

  m["Keijzer-16"] = {"Keijzer-16", 2, 1, {{-2,2},{-2,2}},
    [](const std::vector<double>& x){ double a=x[0], b=x[1]; return std::vector<double>{ (cube(a)/5.0) + (cube(b)/2.0) - b - a }; }, 200, 2000};


  // --------------------
  // Korns (multi-input suite; typical U[-50,50] in many benchmark tables). :contentReference[oaicite:7]{index=7}
  // Note: some Korns entries use specific Xi indices (X0..X4). We'll treat inputs as x0..x4.
  // --------------------
  auto r5 = std::vector<std::pair<double,double>>(5, {-50,50});

  m["Korns-1"] = {"Korns-1", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ 1.57 + 24.3*x[3] }; }, 10000, 10000};

  m["Korns-2"] = {"Korns-2", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ 0.23 + 14.2*((x[3]+x[1])/(3.0*x[4])) }; }, 10000, 10000};

  m["Korns-3"] = {"Korns-3", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ -5.41 + 4.9*(((x[3]-x[0]) + (x[1]/x[4]))/(3.0*x[4])) }; }, 10000, 10000};

  m["Korns-4"] = {"Korns-4", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ -2.3 + 0.13*std::sin(x[2]) }; }, 10000, 10000};

  m["Korns-5"] = {"Korns-5", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ 3.0 + 2.13*safe_log(std::fabs(x[4]) + 1e-12) }; }, 10000, 10000};

  m["Korns-6"] = {"Korns-6", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ 1.3 + 0.13*safe_sqrt(std::fabs(x[0])) }; }, 10000, 10000};

  m["Korns-7"] = {"Korns-7", 5, 1, r5,
    [](const std::vector<double>& x){ return std::vector<double>{ 213.80940889 - (213.80940889*std::exp(-0.54723748542*x[0])) }; }, 10000, 10000};

  m["Korns-8"] = {"Korns-8", 5, 1, r5,
    [](const std::vector<double>& x){
      double v = 7.23*x[0]*x[3]*x[4];
      return std::vector<double>{ 6.87 + 11.0*safe_sqrt(std::fabs(v)) };
    }, 10000, 10000};

  m["Korns-9"] = {"Korns-9", 5, 1, r5,
    [](const std::vector<double>& x){
      double a = safe_sqrt(std::fabs(x[0])) / safe_log(std::fabs(x[1]) + 1e-12);
      double b = std::exp(x[2]) / (sq(x[3]) + 1e-12);
      return std::vector<double>{ a*b };
    }, 10000, 10000};

  m["Korns-10"] = {"Korns-10", 5, 1, r5,
    [](const std::vector<double>& x){
      double num = (2.0*x[1]) + (3.0*sq(x[2]));
      double den = (4.0*cube(x[3])) + (5.0*quart(x[4])) + 1e-12;
      return std::vector<double>{ 0.81 + 24.3*(num/den) };
    }, 10000, 10000};

  m["Korns-11"] = {"Korns-11", 5, 1, r5,
    [](const std::vector<double>& x){
      return std::vector<double>{ 6.87 + 11.0*std::cos(7.23*cube(x[0])) };
    }, 10000, 10000};

  m["Korns-12"] = {"Korns-12", 5, 1, r5,
    [](const std::vector<double>& x){
      return std::vector<double>{ 2.0 - 2.1*(std::cos(9.8*x[0]) * std::sin(1.3*x[4])) };
    }, 10000, 10000};

  m["Korns-13"] = {"Korns-13", 5, 1, r5,
    [](const std::vector<double>& x){
      double a = std::tan(x[0])/(std::tan(x[1]) + 1e-12);
      double b = std::tan(x[2])/(std::tan(x[3]) + 1e-12);
      return std::vector<double>{ 32.0 - 3.0*(a*b) };
    }, 10000, 10000};

  m["Korns-14"] = {"Korns-14", 5, 1, r5,
    [](const std::vector<double>& x){
      // original lists use tanh; if your function set does not include tanh,
      // dataset is still fine; solver may struggle unless tanh is included.
      double a = (std::cos(x[0]) - std::tan(x[1]));
      double b = std::tanh(x[2]) / (std::sin(x[3]) + 1e-12);
      return std::vector<double>{ 22.0 + 4.2*(a*b) };
    }, 10000, 10000};

  m["Korns-15"] = {"Korns-15", 5, 1, r5,
    [](const std::vector<double>& x){
      double a = std::tan(x[0])/(std::exp(x[1]) + 1e-12);
      double b = (safe_log(std::fabs(x[2]) + 1e-12) - std::tan(x[3]));
      return std::vector<double>{ 12.0 - 6.0*(a*b) };
    }, 10000, 10000};

  return m;
}

static const std::unordered_map<std::string, BenchSpec> REG = mk_registry();

std::vector<std::string> benchmark_names() {
  std::vector<std::string> out;
  out.reserve(REG.size());
  for (const auto& kv : REG) out.push_back(kv.first);
  std::sort(out.begin(), out.end());
  return out;
}

BenchSpec get_benchmark(const std::string& name) {
  auto it = REG.find(name);
  if (it == REG.end()) throw std::runtime_error("Unknown benchmark: " + name);
  return it->second;
}

Dataset make_benchmark_dataset(const BenchSpec& b, int n_samples, unsigned long long seed) {
  Dataset d;
  d.X.assign(n_samples, std::vector<double>(b.n_inputs, 0.0));
  d.Y.assign(n_samples, std::vector<double>(b.n_outputs, 0.0));

  // use your RNG via csv.hpp or just a tiny local LCG; here we use a simple splitmix64
  auto splitmix64 = [](unsigned long long& s) {
    unsigned long long z = (s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  };
  auto u01 = [&](unsigned long long& s) {
    // [0,1)
    return (splitmix64(s) >> 11) * (1.0 / 9007199254740992.0);
  };

  unsigned long long s = seed ? seed : 88172645463393265ULL;

  for (int i = 0; i < n_samples; ++i) {
    for (int j = 0; j < b.n_inputs; ++j) {
      auto [lo, hi] = b.ranges[j];
      double r = u01(s);
      d.X[i][j] = lo + (hi - lo) * r;
    }
    d.Y[i] = b.fn(d.X[i]);
  }
  return d;
}
