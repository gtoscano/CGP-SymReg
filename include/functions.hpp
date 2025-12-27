#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string_view>



enum class Op : int {
  ADD = 0,
  SUB,
  MUL,
  DIV,
  NEG,
  POW,
  SIN,
  COS,
  TAN,
  ASIN,
  ACOS,
  ATAN,
  ITE,   // if (a > 0) then b else c
};

inline int arity(Op op) {
  switch (op) {
    case Op::NEG:
    case Op::SIN:
    case Op::COS:
    case Op::TAN:
    case Op::ASIN:
    case Op::ACOS:
    case Op::ATAN:
      return 1;
    case Op::ITE:
      return 3;
    default:
      return 2;
  }
}

inline std::string_view op_name(Op op) {
  switch (op) {
    case Op::ADD: return "+";
    case Op::SUB: return "-";
    case Op::MUL: return "*";
    case Op::DIV: return "/";
    case Op::NEG: return "neg";
    case Op::POW: return "pow";
    case Op::SIN: return "sin";
    case Op::COS: return "cos";
    case Op::TAN: return "tan";
    case Op::ASIN: return "asin";
    case Op::ACOS: return "acos";
    case Op::ATAN: return "atan";
    case Op::ITE: return "ite";
  }
  return "?";
}

// Helpers to keep values sane
inline double clamp_finite(double x) {
  if (!std::isfinite(x)) return 0.0;
  const double LIM = 1e6;
  if (x > LIM) return LIM;
  if (x < -LIM) return -LIM;
  return x;
}

inline double safe_acos(double x) {
    if (x < -1.0) x = -1.0;
    if (x >  1.0) x =  1.0;
    return std::acos(x);
}

inline double safe_asin(double x) {
    if (x < -1.0) x = -1.0;
    if (x >  1.0) x =  1.0;
    return std::asin(x);
}

inline double safe_tan(double x) {
    if (std::fabs(x) > 1.5) return 0.0;
    return std::tan(x);
}

inline double safe_div(double a, double b) {
  const double eps = 1e-12;
  if (std::fabs(b) < eps) return a; // "protected division"
  return a / b;
}

inline double safe_pow(double a, double b) {
  // Avoid nasty complex results and overflows
  a = clamp_finite(a);
  b = clamp_finite(b);

  // If a < 0 and b is non-integer, return 0
  if (a < 0.0) {
    double rb = std::round(b);
    if (std::fabs(b - rb) > 1e-9) return 0.0;
  }

  // Limit exponent magnitude
  const double exp_lim = 8.0;
  if (b > exp_lim) b = exp_lim;
  if (b < -exp_lim) b = -exp_lim;

  double r = std::pow(a, b);
  return clamp_finite(r);
}

inline double eval_op(Op op, const std::vector<double>& in) {
  //auto f1 = [&](double x) { return clamp_finite(x); };
  //  auto f2 = [&](double x, double y) { return clamp_finite(x + y); };

  switch (op) {
    case Op::ADD: return clamp_finite(in[0] + in[1]);
    case Op::SUB: return clamp_finite(in[0] - in[1]);
    case Op::MUL: return clamp_finite(in[0] * in[1]);
    case Op::DIV: return clamp_finite(safe_div(in[0], in[1]));
    case Op::NEG: return clamp_finite(-in[0]);
    case Op::POW: return clamp_finite(safe_pow(in[0], in[1]));
    case Op::SIN: return clamp_finite(std::sin(in[0]));
    case Op::COS: return clamp_finite(std::cos(in[0]));
    case Op::TAN: return clamp_finite(safe_tan(in[0]));
    case Op::ASIN: return clamp_finite(safe_asin(std::clamp(in[0], -1.0, 1.0)));
    case Op::ACOS: return clamp_finite(safe_acos(std::clamp(in[0], -1.0, 1.0)));
    case Op::ATAN: return clamp_finite(std::atan(in[0]));
    case Op::ITE:  return (in[0] > 0.0) ? clamp_finite(in[1]) : clamp_finite(in[2]);
  }
  return 0.0;
}

inline double eval_op(Op op, double a, double b, double c) {
  switch (op) {
    case Op::ADD: return clamp_finite(a + b);
    case Op::SUB: return clamp_finite(a - b);
    case Op::MUL: return clamp_finite(a * b);
    case Op::DIV: return clamp_finite(safe_div(a, b));
    case Op::NEG: return clamp_finite(-a);
    case Op::POW: return clamp_finite(safe_pow(a, b));
    case Op::SIN: return clamp_finite(std::sin(a));
    case Op::COS: return clamp_finite(std::cos(a));
    case Op::TAN: return clamp_finite(safe_tan(a));
    case Op::ASIN: return clamp_finite(safe_asin(std::clamp(a, -1.0, 1.0)));
    case Op::ACOS: return clamp_finite(safe_acos(std::clamp(a, -1.0, 1.0)));
    case Op::ATAN: return clamp_finite(std::atan(a));
    case Op::ITE:  return (a > 0.0) ? clamp_finite(b) : clamp_finite(c);
  }
  return 0.0;
}
