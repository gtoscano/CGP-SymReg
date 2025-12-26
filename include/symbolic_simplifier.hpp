#pragma once
#include <symengine/parser.h>
#include <symengine/simplify.h>
#include <string>

inline std::string simplify_expr_2(const std::string& expr_str) {
    using namespace SymEngine;

    try {
        auto expr = parse(expr_str);
        auto simplified = simplify(expr);
        return simplified->__str__();
    } catch (...) {
        return expr_str; // fallback safely
    }
}


std::string simplify_expr(const std::string& expr_str) {
    using namespace SymEngine;

    try {
        auto expr = parse(expr_str);

        expr = expand(expr);           // algebraic expansion
        expr = simplify(expr);         // light simplification

        return expr->__str__();
    } catch (...) {
        return expr_str;
    }
}
