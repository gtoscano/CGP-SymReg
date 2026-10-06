# ===============================
# Optional dependencies
# ===============================
.DEFAULT_GOAL := all

SYMENGINE_CFLAGS := $(shell pkg-config --cflags symengine 2>/dev/null)
SYMENGINE_LIBS := $(shell pkg-config --libs symengine 2>/dev/null)

# ===============================
# Compiler
# ===============================
CXX ?= g++

# ===============================
# Compiler & Linker Flags
# ===============================
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
            -MMD -MP -Iinclude
LDFLAGS :=

ifneq ($(strip $(SYMENGINE_LIBS)),)
    CXXFLAGS += $(SYMENGINE_CFLAGS) -DCGP_HAVE_SYMENGINE
    LDFLAGS += $(SYMENGINE_LIBS)
endif

# ===============================
# Project Structure
# ===============================
BIN_DIR := bin
OBJ_DIR := build

SRCS := src/main.cpp src/cgp.cpp src/benchmarks.cpp
OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/src/%.o,$(SRCS))

APP := $(BIN_DIR)/cgp_symreg
CGP_TEST_APP := $(BIN_DIR)/test_cgp
BENCH_TEST_APP := $(BIN_DIR)/test_benchmarks
DEPS := $(OBJS:.o=.d) $(OBJ_DIR)/tests/test_main.d $(OBJ_DIR)/tests/test_benchmarks.d

-include $(DEPS)

# ===============================
# Build Rules
# ===============================

all: $(APP)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/src
	mkdir -p $(OBJ_DIR)/tests

$(OBJ_DIR)/src/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(APP): $(BIN_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(APP) $(LDFLAGS)

$(CGP_TEST_APP): $(OBJ_DIR)/tests/test_main.o $(OBJ_DIR)/src/cgp.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BENCH_TEST_APP): $(OBJ_DIR)/tests/test_benchmarks.o $(OBJ_DIR)/src/benchmarks.o | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# ===============================
# Utility Targets
# ===============================

run: $(APP)
	$(APP)

test: $(CGP_TEST_APP) $(BENCH_TEST_APP)
	$(CGP_TEST_APP)
	$(BENCH_TEST_APP)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
