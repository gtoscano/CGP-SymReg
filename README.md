# 🧬 Cartesian Genetic Programming (CGP) – Symbolic Regression Framework

This project implements a **Cartesian Genetic Programming (CGP)** engine for symbolic regression in C++.

It supports:

* Multi-output symbolic regression
* ERCs (ephemeral random constants)
* Expression simplification
* Benchmark problems (Nguyen, Keijzer, etc.)
* Reproducible experiments
* Clean separation between evolution, evaluation, and reporting

---

## 📦 Project Structure

```
.
├── include/
│   ├── cgp.hpp
│   ├── dataset.hpp
│   ├── benchmarks.hpp
│   └── functions.hpp
├── src/
│   ├── cgp.cpp
│   ├── benchmarks.cpp
│   └── main.cpp
├── build/
├── bin/
├── Makefile
└── README.md
```

---

## ⚙️ Build Instructions

### Requirements

* C++17 or newer
* `g++` or `clang++`
* GNU Make

### Build

```bash
make clean
make
```

This produces:

```
bin/cgp_symreg
```

---

## ▶️ Running the Program

### Default run (synthetic benchmark)

```bash
./bin/cgp_symreg
```

Output:

```
Benchmark: Nguyen-10
Best fitness: 0.0123
Output 0 expr:
sin(x) + x*x
```

---

### Run a specific benchmark

```bash
./bin/cgp_symreg --benchmark Nguyen-5
```

List available benchmarks:

```bash
./bin/cgp_symreg --list-benchmarks
```

---

### Control dataset size

```bash
./bin/cgp_symreg --benchmark Nguyen-10 --train 200 --test 100
```

---

## 🧠 Example Output

```
Best fitness: 0.00341

Output 0 expr:
(x * x) + sin(x)

Sample predictions:
x=-2.50 pred=5.62 true=5.65
x=-1.25 pred=1.81 true=1.78
x= 0.00 pred=0.00 true=0.00
x= 1.25 pred=1.81 true=1.78
```

---

## ⚙️ Configuration (inside `main.cpp`)

You can tune CGP behavior here:

```cpp
cfg.n_inputs = 1;
cfg.n_outputs = 1;

cfg.lambda = 20;
cfg.generations = 8000;

cfg.mutation_rate = 0.06;
cfg.const_mutation_rate = 0.15;

cfg.use_constants = true;
cfg.use_erc = true;

cfg.complexity_alpha = 1e-3;
cfg.reuse_beta = 1e-4;
```

---

## 🧠 Expression Simplification

After evolution, expressions are simplified using rule-based rewriting:

```cpp
expr = cgp.simplify_expr(expr);
```

Example:

```
((x * 1) + (0 + sin(x))) → sin(x)
```

---

## 📊 Benchmark Suite

Supported benchmark families:

* Nguyen (1–12)
* Keijzer (in progress)
* Custom synthetic functions

Each benchmark defines:

* Number of inputs
* Output formula
* Training and test sizes

---

## 📁 Adding a New Benchmark

1. Edit `benchmarks.cpp`
2. Add a new `BenchSpec`
3. Implement dataset generation
4. Recompile

---

## 🧪 Example Command Sequence

```bash
make clean
make
./bin/cgp_symreg --benchmark Nguyen-7 --train 200 --test 200
```

---

## 🧠 Tips

* Use **lower mutation** for fine tuning
* Increase **generations** for harder problems
* Enable **ERCs** for polynomial discovery
* Disable ERCs to force structure discovery

