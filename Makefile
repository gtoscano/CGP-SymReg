# ===============================
# Platform detection
# ===============================
UNAME_S := $(shell uname -s)

# ===============================
# Compiler
# ===============================
ifeq ($(UNAME_S),Darwin)
    CXX := /usr/bin/clang++
    SYMENGINE_PREFIX := /opt/homebrew
else
    CXX := g++
    SYMENGINE_PREFIX := /usr
endif

# ===============================
# Compiler & Linker Flags
# ===============================
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
            -Iinclude -I$(SYMENGINE_PREFIX)/include


LDFLAGS  := -L$(SYMENGINE_PREFIX)/lib \
            -lsymengine -lgmp

# ===============================
# Project Structure
# ===============================
BIN_DIR := bin
OBJ_DIR := build

SRCS := src/main.cpp src/cgp.cpp src/benchmarks.cpp
OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/src/%.o,$(SRCS))

TEST_SRCS := tests/test_main.cpp src/cgp.cpp
TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))

APP := $(BIN_DIR)/cgp_symreg
TEST_APP := $(BIN_DIR)/tests

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

$(TEST_APP): $(BIN_DIR) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $(TEST_APP) $(LDFLAGS)

# ===============================
# Utility Targets
# ===============================

run: $(APP)
	$(APP)

test: $(TEST_APP)
	$(TEST_APP)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

