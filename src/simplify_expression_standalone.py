# python simplify_expression_standalone.py --expr="sin(((x0 + x0) + (x0 * (x0 * (x0 + x0)))))"
import sympy as sp
import argparse
import sys

def simplify_expression(expr_str):
    x0 = sp.symbols('x0')

    try:
        expr = sp.sympify(expr_str, locals={"x0": x0})
    except Exception as e:
        print("❌ Parsing error:", e)
        sys.exit(1)

    simplified = sp.simplify(expr_str)
    expanded = sp.expand(simplified)

    print("\n=== ORIGINAL ===")
    print(expr_str)

    print("\n=== SIMPLIFIED ===")
    print(simplified)

    print("\n=== EXPANDED ===")
    print(expanded)

    print("\n=== STATS ===")
    print("Nodes:", len(list(sp.preorder_traversal(simplified))))
    print("Free symbols:", simplified.free_symbols)


if __name__ == "__main__":
    print(">>> simplify_expression.py started")

    parser = argparse.ArgumentParser()
    parser.add_argument("--expr", required=True, help="Expression to simplify")
    args = parser.parse_args()

    simplify_expression(args.expr)

