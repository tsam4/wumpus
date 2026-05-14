# ============================================================================
# Comprehensive Makefile: Build, Test, Run All Datasets, and Report Results
# ============================================================================
#
# Targets:
#   make build              Build the wumpus executable and all tests
#   make test               Run unit tests (parser, emission, transition, BP)
#   make run                Run all datasets and generate outputs
#   make demo               Quick demo: build + run all 3 datasets with verbose output
#   make accuracy           Compute accuracy vs ground truth (all datasets with ground truth)
#   make all                Full pipeline: build + test + run + accuracy
#   make clean              Remove generated output files (results/ + emdw runtime artifacts)
#
#   Flag-Based Targets (use ACTIVE_DATASET flag in src/wumpus.cc):
#   make flag-d1            Compile with ACTIVE_DATASET=1, run, visualize
#   make flag-d2            Compile with ACTIVE_DATASET=2, run, visualize
#   make flag-d3            Compile with ACTIVE_DATASET=3, run, visualize
#   make visualize-d<N>     Generate visualizations for dataset N
#
# Environment:
#   EMDW_BUILD:  Path to emdw build directory (default: repo-local)
#   WUMPUS_BIN:  Path to wumpus executable
#   PROJECT_DIR: Path to this project (auto-detected)
#   DATASET_DIR: Path to datasets (auto-detected)
#
# Outputs:
#   All .txt trajectory files and .gif animations are written to results/.
#   err.txt and emdw_RandomEngine_seedVal.txt are written by emdw to the
#   project root (hardcoded in emdw source); make clean removes them.
#
# ============================================================================

# Configuration
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
EMDW_BUILD  ?= $(PROJECT_DIR)/emdw_de424/devel/emdw/build
DATASET_DIR := $(PROJECT_DIR)/Datasets-20260506
RESULTS_DIR := $(PROJECT_DIR)/results
WUMPUS_BIN  := $(EMDW_BUILD)/src/bin/wumpus
WUMPUS_TEST_DIR := $(EMDW_BUILD)/src/bin
PYTHON      := $(if $(wildcard $(PROJECT_DIR)/.venv/bin/python),$(PROJECT_DIR)/.venv/bin/python,python3)
VISUALIZER  := $(PROJECT_DIR)/scripts/visualize.py
ACCURACY_SCRIPT := $(PROJECT_DIR)/scripts/compute_accuracy.py

# Ground truth directory pattern: "Ground truth for dataset<N>-*"
GT_BASE := $(PROJECT_DIR)

# Output file prefixes -- all outputs land in results/
OUT_D1 := $(RESULTS_DIR)/out_d1
OUT_D2 := $(RESULTS_DIR)/out_d2
OUT_D3 := $(RESULTS_DIR)/out_d3

# Dataset directories
D1_DIR := $(DATASET_DIR)/dataset1
D2_DIR := $(DATASET_DIR)/dataset2
D3_DIR := $(DATASET_DIR)/dataset3

# ANSI color codes for output
RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[1;33m
CYAN   := \033[0;36m
NC     := \033[0m  # No Color

# ============================================================================
# Default target
# ============================================================================
.PHONY: help
help:
	@echo "Wumpus Tracker Makefile"
	@echo "Usage: make [target]"
	@echo "  build          Build the executable"
	@echo "  test           Run unit tests (parser, emission, transition, BP)"
	@echo "  run            Execute datasets 1-3"
	@echo "  demo           Quick demo: build + run all 3 datasets with verbose output"
	@echo "  accuracy       Check accuracy vs ground truth for all available datasets"
	@echo "  all            Full pipeline: build, test, run, accuracy"
	@echo "  clean          Remove generated files (results/ + emdw runtime artifacts)"
	@echo ""
	@echo "Additional targets: run-d1 run-d2 run-d3 visualize-d1 visualize-d2 visualize-d3"
	@echo ""

# ============================================================================
# Build Targets
# ============================================================================

.PHONY: build
build:
	@cmake -S emdw_de424/devel/emdw -B emdw_de424/devel/emdw/build -DCMAKE_BUILD_TYPE=Release -Wno-dev > /dev/null
	@cmake --build emdw_de424/devel/emdw/build --parallel 12 2>&1 | grep -E "^\[|error:|warning:" | grep -v "warning:" || true
	@echo "$(GREEN)✓ Build complete$(NC)"

# ============================================================================
# Test Targets
# Tests cover the four components required by the project spec:
#   test_parser     -- detection file parsing
#   test_emission   -- emission factor (pw / pc)
#   test_transition -- transition factor (wumpus movement)
#   test_emdw_bp    -- belief propagation via emdw
# ============================================================================

.PHONY: test
test: build
	@echo "Running unit tests..."
	@for t in test_parser test_emission test_transition test_emdw_bp; do \
		if [ -x $(WUMPUS_TEST_DIR)/$$t ]; then \
			$(WUMPUS_TEST_DIR)/$$t >/dev/null 2>&1 \
				&& echo "  $$t: PASS" \
				|| echo "  $$t: FAIL"; \
		else \
			echo "  $$t: SKIPPED (binary not found)"; \
		fi; \
	done
	@echo ""
	@echo "$(GREEN)✓ Tests complete$(NC)"

# ============================================================================
# Demo Target
# ============================================================================

# Helper macro: run one dataset and print confirmation + trajectory preview.
# Usage: $(call demo_run, N, LABEL, DIR, OUT_PREFIX, NOTE)
define demo_run
	@mkdir -p $(RESULTS_DIR)
	@echo ""
	@echo "Dataset $(1): $(2)"
	@echo "  $(5)"
	@if [ ! -d "$(3)" ]; then \
		echo "$(RED)  ✗ Dataset dir not found: $(3)$(NC)"; exit 1; \
	fi
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(3) $(1) $(4); \
	status=$$?; \
	duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then \
		echo "$(RED)  ✗ wumpus exited with status $$status$(NC)"; exit $$status; \
	fi; \
	out="$(4)_marginal.txt"; \
	if [ ! -f "$$out" ]; then \
		echo "$(RED)  ✗ Output file not written: $$out$(NC)"; exit 1; \
	fi; \
	lines=$$(wc -l < "$$out" | tr -d ' '); \
	echo "$(GREEN)  ✓ done in $${duration}s — $${lines} timesteps written to $$(basename $$out)$(NC)"; \
	first5=$$(head -5 "$$out" | awk '{printf "(" $$1 ", " $$2 ")"}' | sed 's/)(/, /g; s/^/[/; s/$$/]/'); \
	last5=$$(tail -5  "$$out" | awk '{printf "(" $$1 ", " $$2 ")"}' | sed 's/)(/, /g; s/^/[/; s/$$/]/'); \
	echo "  First 5: $$first5"; \
	echo "  Last 5:  $$last5";
endef

.PHONY: demo
demo: build
	$(call demo_run,1,5x5 grid  10 timesteps  pw=0.95 pc=0.05,$(D1_DIR),$(OUT_D1),known parameters - single BP pass)
	$(call demo_run,2,20x20 grid  20 timesteps  pw=0.90 pc=0.10,$(D2_DIR),$(OUT_D2),known parameters - single BP pass)
	$(call demo_run,3,10x20 grid  20 timesteps  EM learning,$(D3_DIR),$(OUT_D3),unknown pw/pc - 6 EM iterations then final BP pass)
	@echo ""
	@echo "$(GREEN)✓ Demo complete$(NC)"

# ============================================================================
# Run Targets (Execute Datasets)
# ============================================================================

.PHONY: run
run: build
	@mkdir -p $(RESULTS_DIR)
	@echo "Running datasets 1-3..."
	@start=$$(date +%s); \
	echo "  Dataset 1: 5x5, 10 steps"; \
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) >/dev/null && echo "    done" || echo "    FAILED"; \
	echo "  Dataset 2: 20x20, 20 steps"; \
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) >/dev/null && echo "    done" || echo "    FAILED"; \
	echo "  Dataset 3: 10x20, 20 steps"; \
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) >/dev/null && echo "    done" || echo "    FAILED"; \
	total=$$(( $$(date +%s) - start )); \
	echo "$(GREEN)✓ All datasets complete in $${total}s$(NC)"

.PHONY: run-d1
run-d1: build
	@mkdir -p $(RESULTS_DIR)
	@echo "Dataset 1: 5x5, 10 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) >/dev/null; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

.PHONY: run-d2
run-d2: build
	@mkdir -p $(RESULTS_DIR)
	@echo "Dataset 2: 20x20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) >/dev/null; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

.PHONY: run-d3
run-d3: build
	@mkdir -p $(RESULTS_DIR)
	@echo "Dataset 3: 10x20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) >/dev/null; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

# ============================================================================
# Flag-Based Targets (run with visualizer)
# ============================================================================

.PHONY: flag-d1 flag-d2 flag-d3

flag-d1: build
	@mkdir -p $(RESULTS_DIR)
	@echo "$(CYAN)[FLAG-D1]$(NC) Dataset1 with visualizer"
	@time $(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1)
	@if [ -f "$(OUT_D1)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		$(PYTHON) $(VISUALIZER) 1; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d2: build
	@mkdir -p $(RESULTS_DIR)
	@echo "$(CYAN)[FLAG-D2]$(NC) Dataset2 with visualizer"
	@time $(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2)
	@if [ -f "$(OUT_D2)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		$(PYTHON) $(VISUALIZER) 2; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d3: build
	@mkdir -p $(RESULTS_DIR)
	@echo "$(CYAN)[FLAG-D3]$(NC) Dataset3 with visualizer"
	@time $(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3)
	@if [ -f "$(OUT_D3)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		$(PYTHON) $(VISUALIZER) 3; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

# Convenience targets for visualizer-only (without recompiling)
.PHONY: visualize-d1 visualize-d2 visualize-d3
visualize-d1:
	@echo "$(CYAN)[VISUALIZE-D1]$(NC)"
	@$(PYTHON) $(VISUALIZER) 1

visualize-d2:
	@echo "$(CYAN)[VISUALIZE-D2]$(NC)"
	@$(PYTHON) $(VISUALIZER) 2

visualize-d3:
	@echo "$(CYAN)[VISUALIZE-D3]$(NC)"
	@$(PYTHON) $(VISUALIZER) 3

# ============================================================================
# Accuracy Target (Compare with Ground Truth)
# ============================================================================

# Internal macro: check one dataset against its ground truth
# Usage: $(call check_accuracy, N, OUT_PREFIX)
define check_accuracy
	@gt_dir=$$(find "$(GT_BASE)" -maxdepth 1 -type d -name "Ground truth for dataset$(1)-*" 2>/dev/null | head -1); \
	if [ -z "$$gt_dir" ]; then \
		echo "  Dataset $(1): no ground truth directory found — skipping"; \
	else \
		gt_file="$$gt_dir/wumpus_trajectory.txt"; \
		out_file="$(2)_marginal.txt"; \
		if [ ! -f "$$gt_file" ]; then \
			echo "$(YELLOW)  Dataset $(1): ground truth dir found but wumpus_trajectory.txt missing — skipping$(NC)"; \
		elif [ ! -f "$$out_file" ]; then \
			echo "$(YELLOW)  Dataset $(1): output not found ($$out_file) — run 'make run-d$(1)' first$(NC)"; \
		else \
			echo ""; \
			echo "Dataset $(1) — $$gt_dir"; \
			$(PYTHON) "$(ACCURACY_SCRIPT)" "$$gt_file" "$$out_file"; \
		fi; \
	fi
endef

.PHONY: accuracy
accuracy:
	@echo ""
	@echo "Checking accuracy for all datasets with available ground truth..."
	$(call check_accuracy,1,$(OUT_D1))
	$(call check_accuracy,2,$(OUT_D2))
	$(call check_accuracy,3,$(OUT_D3))
	@echo ""
	@echo "$(GREEN)✓ Accuracy check complete$(NC)"

# ============================================================================
# Combined Targets
# ============================================================================

.PHONY: all
all: build test run accuracy
	@echo ""
	@echo "$(GREEN)✓✓✓ COMPLETE PIPELINE FINISHED ✓✓✓$(NC)"
	@echo ""

# ============================================================================
# Cleanup
# ============================================================================

.PHONY: clean
clean:
	@echo "Removing results/ and emdw runtime artifacts..."
	@rm -rf $(RESULTS_DIR)
	@rm -f $(PROJECT_DIR)/err.txt $(PROJECT_DIR)/emdw_RandomEngine_seedVal.txt
	@echo "$(GREEN)✓ Clean complete$(NC)"

.PHONY: clean-build
clean-build:
	@echo "Removing emdw build artifacts..."
	@rm -rf $(EMDW_BUILD)/CMakeFiles $(EMDW_BUILD)/cmake_install.cmake \
	         $(EMDW_BUILD)/CMakeCache.txt $(EMDW_BUILD)/Makefile
	@echo "$(GREEN)✓ Build clean complete$(NC)"

# ============================================================================
# Diagnostic Targets
# ============================================================================

.PHONY: info
info:
	@echo "Project Directory: $(PROJECT_DIR)"
	@echo "EMDW Build Dir:    $(EMDW_BUILD)"
	@echo "Wumpus Binary:     $(WUMPUS_BIN)"
	@echo "Dataset Directory: $(DATASET_DIR)"
	@echo "Results Directory: $(RESULTS_DIR)"
	@echo ""
	@echo "Dataset locations:"
	@echo "  D1: $(D1_DIR)"
	@echo "  D2: $(D2_DIR)"
	@echo "  D3: $(D3_DIR)"
	@echo ""
	@echo "Output prefixes:"
	@echo "  D1: $(OUT_D1)"
	@echo "  D2: $(OUT_D2)"
	@echo "  D3: $(OUT_D3)"
	@echo ""
	@if [ -f $(WUMPUS_BIN) ]; then \
		echo "$(GREEN)✓ Wumpus binary found$(NC)"; \
	else \
		echo "$(RED)✗ Wumpus binary NOT found$(NC)"; \
	fi
	@if [ -d $(DATASET_DIR) ]; then \
		echo "$(GREEN)✓ Dataset directory found$(NC)"; \
	else \
		echo "$(RED)✗ Dataset directory NOT found$(NC)"; \
	fi

# ============================================================================
# Phony targets
# ============================================================================
.PHONY: .FORCE
.FORCE:

# End of Makefile
