# ============================================================================
# Wumpus Tracker — DE424 Mini-Project Makefile
# ============================================================================
#
# USAGE (most common):
#   make            → configure + build everything (first time, or after clean)
#   make demo       → run tracker on all three datasets, print trajectories
#   make tests      → build + run unit tests (transition, emission, parser, BP)
#   make clean      → remove all build artefacts
#
# Less common:
#   make configure  → cmake configure only (called automatically by 'make')
#   make build      → compile only (skips cmake)
#   make help       → print this list
#
# Paths (adjust if your tree differs):
#   BUILD_DIR  — CMake build directory
#   DATA_DIR   — root folder containing dataset1/, dataset2/, dataset3/
#   OUT_DIR    — where trajectory files are written
# ============================================================================

BUILD_DIR := emdw_de424/devel/emdw/build
BIN_DIR   := $(BUILD_DIR)/src/bin
DATA_DIR  := data
OUT_DIR   := results

# macOS uses sysctl; Linux uses nproc
NPROC := $(shell sysctl -n hw.logicalcpu 2>/dev/null || nproc)

.PHONY: all configure build demo tests clean help

# ----------------------------------------------------------------------------
# all: configure + build (default target)
# ----------------------------------------------------------------------------
all: configure build

configure:
	cmake -S emdw_de424/devel/emdw -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build:
	cmake --build $(BUILD_DIR) --parallel $(NPROC)

# ----------------------------------------------------------------------------
# demo: run the tracker on datasets 1-3 and print the marginal trajectories
#
# This is the main "show-your-work" target.  It replicates the three inference
# scenarios required by the mini-project spec:
#
#   dataset1 — 5×5, 10 steps, known pw=0.95 pc=0.05
#   dataset2 — 20×20, 20 steps, known pw=0.90 pc=0.10
#   dataset3 — 10×20, 20 steps, unknown params → learned via EM (6 iterations)
#
# Output files land in $(OUT_DIR)/:
#   ds1_marginal.txt, ds2_marginal.txt, ds3_marginal.txt
# Each file has one "x y" coordinate per timestep — the MAP cell at that step.
# ----------------------------------------------------------------------------
demo: build
	@mkdir -p $(OUT_DIR)

	@echo ""
	@echo "============================================================"
	@echo " Dataset 1 — 5×5 grid, 10 timesteps (pw=0.95, pc=0.05)"
	@echo "============================================================"
	$(BIN_DIR)/wumpus $(DATA_DIR)/dataset1 1 $(OUT_DIR)/ds1
	@echo "--- Marginal trajectory (x y per timestep) ---"
	@cat $(OUT_DIR)/ds1_marginal.txt

	@echo ""
	@echo "============================================================"
	@echo " Dataset 2 — 20×20 grid, 20 timesteps (pw=0.90, pc=0.10)"
	@echo "============================================================"
	$(BIN_DIR)/wumpus $(DATA_DIR)/dataset2 2 $(OUT_DIR)/ds2
	@echo "--- Marginal trajectory (x y per timestep) ---"
	@cat $(OUT_DIR)/ds2_marginal.txt

	@echo ""
	@echo "============================================================"
	@echo " Dataset 3 — 10×20 grid, 20 timesteps (EM: pw/pc unknown)"
	@echo "============================================================"
	$(BIN_DIR)/wumpus $(DATA_DIR)/dataset3 3 $(OUT_DIR)/ds3
	@echo "--- Marginal trajectory (x y per timestep) ---"
	@cat $(OUT_DIR)/ds3_marginal.txt

	@echo ""
	@echo "All output files written to $(OUT_DIR)/"

# ----------------------------------------------------------------------------
# tests: build + run the four unit tests required by the mini-project
#
#   test_transition  — transition matrix correctness (border self-loops, sums)
#   test_emission    — log-prob values, clamp, no NaN on degenerate input
#   test_parser      — data_file*.txt round-trip, missing/empty file handling
#   test_emdw_bp     — full 2×2 BP integration: MAP cell, normalisation, no NaN
# ----------------------------------------------------------------------------
tests: build
	@echo ""
	@echo "--- test_transition ---"
	$(BIN_DIR)/test_transition

	@echo "--- test_emission ---"
	$(BIN_DIR)/test_emission

	@echo "--- test_parser ---"
	$(BIN_DIR)/test_parser

	@echo "--- test_emdw_bp ---"
	$(BIN_DIR)/test_emdw_bp

	@echo ""
	@echo "All tests passed."

# ----------------------------------------------------------------------------
# clean: remove the entire build directory
# (cmake --target clean fails if the cache doesn't exist yet)
# ----------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)

# ----------------------------------------------------------------------------
# help
# ----------------------------------------------------------------------------
help:
	@echo ""
	@echo "Wumpus Tracker — DE424 Mini-Project"
	@echo ""
	@echo "  make            configure + build everything"
	@echo "  make demo       run tracker on datasets 1-3, print trajectories"
	@echo "  make tests      build + run unit tests"
	@echo "  make configure  cmake configure only"
	@echo "  make build      compile only"
	@echo "  make clean      remove build artefacts"
	@echo "  make help       show this message"
	@echo ""
	@echo "Key paths (override on command line):"
	@echo "  BUILD_DIR = $(BUILD_DIR)"
	@echo "  DATA_DIR  = $(DATA_DIR)"
	@echo "  OUT_DIR   = $(OUT_DIR)"
	@echo ""
