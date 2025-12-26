#!/usr/bin/env python3
import sys
import sympy as sp

def main():
    expr_str = sys.stdin.read().strip()
    if not expr_str:
        print("ERROR: empty input", file=sys.stderr)
        sys.exit(1)

    x0 = sp.symbols('x0')

    try:
        expr = sp.sympify(expr_str, locals={"x0": x0})
        simplified = sp.simplify(expr)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(2)

    print(simplified)

if __name__ == "__main__":
    main()
