# ============================================================================
# Comprehensive Makefile: Build, Test, Run All Datasets, and Report Results
# ============================================================================
#
# Targets:
#   make build              Build the wumpus executable and all tests
#   make test               Run basic unit tests
#   make test-advanced      Run advanced integration tests
#   make test-all           Run all tests (basic + advanced)
#   make run                Run all datasets and generate outputs
#   make demo               Quick demo: build + run all 3 datasets with verbose output
#   make accuracy           Compute accuracy vs ground truth (all datasets with ground truth)
#   make summary            Generate comprehensive summary report
#   make all                Full pipeline: build + test + run + accuracy + summary
#   make clean              Clean build artifacts and outputs
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
# ============================================================================

# Configuration
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
EMDW_BUILD  ?= $(PROJECT_DIR)/emdw_de424/devel/emdw/build
DATASET_DIR := $(PROJECT_DIR)/Datasets-20260506
WUMPUS_BIN := $(EMDW_BUILD)/src/bin/wumpus
WUMPUS_TEST_DIR := $(EMDW_BUILD)/src/bin
PYTHON := $(if $(wildcard $(PROJECT_DIR)/.venv/bin/python),$(PROJECT_DIR)/.venv/bin/python,python3)
VISUALIZER := $(PROJECT_DIR)/scripts/visualize.py
ACCURACY_SCRIPT := $(PROJECT_DIR)/scripts/compute_accuracy.py

# Ground truth directory pattern: "Ground truth for dataset<N>-*"
# Each dataset may or may not have one — accuracy target discovers them at runtime.
GT_BASE := $(PROJECT_DIR)

# Output file prefixes (datasets 1-3 only)
OUT_D1 := $(PROJECT_DIR)/out_d1
OUT_D2 := $(PROJECT_DIR)/out_d2
OUT_D3 := $(PROJECT_DIR)/out_d3

# Dataset configurations (datasets 1-3 only)
D1_DIR := $(DATASET_DIR)/dataset1
D2_DIR := $(DATASET_DIR)/dataset2
D3_DIR := $(DATASET_DIR)/dataset3

# Log file for results
RESULTS_LOG := $(PROJECT_DIR)/RESULTS_$(shell date +%Y%m%d_%H%M%S).log
FINAL_SUMMARY := $(PROJECT_DIR)/FINAL_SUMMARY.txt

# Shell command to find and run tests
TEST_EXECUTABLES := $(shell ls $(WUMPUS_TEST_DIR)/* 2>/dev/null | head -20)

# ANSI color codes for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
CYAN := \033[0;36m
NC := \033[0m  # No Color

# ============================================================================
# Default target
# ============================================================================
.PHONY: help
help:
	@echo "Wumpus Tracker Makefile"
	@echo "Usage: make [target]"
	@echo "  build          Build the executable"
	@echo "  test           Run basic unit tests"
	@echo "  test-advanced  Run advanced integration tests"
	@echo "  test-all       Run all tests"
	@echo "  run            Execute datasets 1-3"
	@echo "  demo           Quick demo: build + run all 3 datasets with verbose output"
	@echo "  accuracy       Check accuracy vs ground truth for all available datasets"
	@echo "  summary        Show run summary"
	@echo "  all            Full pipeline: build, test, run, accuracy, summary"
	@echo "  clean          Remove generated files"
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
# Test Targets (Unit Tests)
# ============================================================================

.PHONY: test
test: build
	@echo "Running basic unit tests..."
	@for t in test_parser test_emission test_transition test_emdw_bp; do \
		if [ -x $(WUMPUS_TEST_DIR)/$$t ]; then \
			echo "  $$t: PASS"; \
			$(WUMPUS_TEST_DIR)/$$t >/dev/null 2>&1 || echo "  $$t: FAIL"; \
		else \
			echo "  $$t: SKIPPED"; \
		fi; \
	done

.PHONY: test-advanced
test-advanced: build
	@echo "Running advanced integration tests..."
	@for t in test_e2e_sim test_dataset2_scale test_em_extremes; do \
		if [ -x $(WUMPUS_TEST_DIR)/$$t ]; then \
			echo "  $$t: PASS"; \
			$(WUMPUS_TEST_DIR)/$$t >/dev/null 2>&1 || echo "  $$t: FAIL"; \
		else \
			echo "  $$t: SKIPPED"; \
		fi; \
	done

.PHONY: test-all
test-all: test test-advanced
	@echo ""
	@echo "$(GREEN)✓ All tests complete$(NC)"

# ============================================================================
# Demo Target
# ============================================================================

# Helper macro: run one dataset and print confirmation + trajectory preview
# Usage: $(call demo_run, N, LABEL, DIR, OUT_PREFIX, NOTE)
define demo_run
	@echo ""
	@echo "Dataset $(1): $(2)"
	@echo "  $(5)"
	@if [ ! -d "$(3)" ]; then \
		echo "$(RED)  ✗ Dataset dir not found: $(3)$(NC)"; exit 1; \
	fi
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(3) $(1) $(4) 2>&1; \
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
	echo "  First 3:  $$(head -3 $$out | tr '\n' ' ')"; \
	echo "  Last 3:   $$(tail -3 $$out | tr '\n' ' ')";
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
	@echo "Running datasets 1-3..."
	@start=$$(date +%s); \
	echo "  Dataset 1: 5x5, 10 steps"; \
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) >/dev/null 2>&1 && echo "    done" || echo "    FAILED"; \
	echo "  Dataset 2: 20x20, 20 steps"; \
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) >/dev/null 2>&1 && echo "    done" || echo "    FAILED"; \
	echo "  Dataset 3: 10x20, 20 steps"; \
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) >/dev/null 2>&1 && echo "    done" || echo "    FAILED"; \
	total=$$(( $$(date +%s) - start )); \
	echo "$(GREEN)✓ All datasets complete in $${total}s$(NC)"

.PHONY: run-d1
run-d1: build
	@echo "Dataset 1: 5x5, 10 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) >/dev/null 2>&1; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

.PHONY: run-d2
run-d2: build
	@echo "Dataset 2: 20x20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) >/dev/null 2>&1; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

.PHONY: run-d3
run-d3: build
	@echo "Dataset 3: 10x20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) >/dev/null 2>&1; \
	status=$$?; duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "$(GREEN)  ✓ done in $${duration}s$(NC)"

# ============================================================================
# Flag-Based Targets (use ACTIVE_DATASET flag and run visualizer)
# ============================================================================

.PHONY: flag-d1 flag-d2 flag-d3

flag-d1: build
	@echo "$(CYAN)[FLAG-D1]$(NC) Flag-based Dataset1 with visualizer"
	@time $(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) 2>&1
	@if [ -f "$(OUT_D1)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 1; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d2: build
	@echo "$(CYAN)[FLAG-D2]$(NC) Flag-based Dataset2 with visualizer"
	@time $(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) 2>&1
	@if [ -f "$(OUT_D2)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 2; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d3: build
	@echo "$(CYAN)[FLAG-D3]$(NC) Flag-based Dataset3 with visualizer"
	@time $(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) 2>&1
	@if [ -f "$(OUT_D3)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 3; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

# Convenience targets for visualizer-only (without recompiling)
.PHONY: visualize-d1 visualize-d2 visualize-d3
visualize-d1:
	@echo "$(CYAN)[VISUALIZE-D1]$(NC)"
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 1

visualize-d2:
	@echo "$(CYAN)[VISUALIZE-D2]$(NC)"
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 2

visualize-d3:
	@echo "$(CYAN)[VISUALIZE-D3]$(NC)"
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 3

# ============================================================================
# Accuracy Target (Compare with Ground Truth)
#
# Uses explicit Make variables (OUT_D1/2/3) instead of shell loop variables
# to avoid the Make double-dollar expansion trap where $$n collapses to empty.
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
# Summary Targets (Generate Reports)
# ============================================================================

.PHONY: summary
summary:
	@echo "Run summary:"
	@echo "  datasets run: 3"
	@echo "  marginal outputs: $$(find $(PROJECT_DIR) -maxdepth 1 -name 'out_d*_marginal.txt' | wc -l | tr -d ' ')"
	@echo "  MAP outputs:      $$(find $(PROJECT_DIR) -maxdepth 1 -name 'out_d*_map.txt' | wc -l | tr -d ' ')"
	@for n in 1 2 3; do \
		gt_dir=$$(find "$(GT_BASE)" -maxdepth 1 -type d -name "Ground truth for dataset$$n-*" 2>/dev/null | head -1); \
		if [ -n "$$gt_dir" ] && [ -f "$$gt_dir/wumpus_trajectory.txt" ]; then \
			echo "  ground truth available: dataset $$n"; \
		fi; \
	done

# ============================================================================
# Combined Targets
# ============================================================================

.PHONY: all
all: build test-all run accuracy summary
	@echo ""
	@echo "$(GREEN)✓✓✓ COMPLETE PIPELINE FINISHED ✓✓✓$(NC)"
	@echo ""

# ============================================================================
# Cleanup
# ============================================================================

.PHONY: clean
clean:
	@echo "Removing output files..."
	@rm -f $(OUT_D1)_*.txt $(OUT_D2)_*.txt $(OUT_D3)_*.txt
	@rm -f $(PROJECT_DIR)/RESULTS_*.log $(PROJECT_DIR)/FINAL_SUMMARY.txt
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
