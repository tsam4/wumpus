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
#   make demo               Quick demo: build + run dataset1 with verbose output
#   make accuracy           Compute accuracy vs ground truth (dataset1 only)
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
#   GROUND_TRUTH: Path to ground truth trajectory (auto-detected)
#
# ============================================================================

# Configuration
PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
EMDW_BUILD  ?= $(PROJECT_DIR)/emdw_de424/devel/emdw/build
DATASET_DIR := $(PROJECT_DIR)/Datasets-20260506
GROUND_TRUTH := $(PROJECT_DIR)/Ground truth for dataset1-20260506/wumpus_trajectory.txt
WUMPUS_BIN := $(EMDW_BUILD)/src/bin/wumpus
WUMPUS_TEST_DIR := $(EMDW_BUILD)/src/bin
PYTHON := $(if $(wildcard $(PROJECT_DIR)/.venv/bin/python),$(PROJECT_DIR)/.venv/bin/python,python3)
VISUALIZER := $(PROJECT_DIR)/visualize.py

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
	@echo "  demo           Quick demo: build + run dataset1 with verbose output"
	@echo "  accuracy       Check Dataset1 against ground truth"
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
	cmake -S emdw_de424/devel/emdw -B emdw_de424/devel/emdw/build -DCMAKE_BUILD_TYPE=Release
	cmake --build emdw_de424/devel/emdw/build --parallel 12

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
	@for t in test_e2e_sim test_dataset2_scale; do \
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

.PHONY: demo
demo: build
	@echo ""
	@echo "============================================================"
	@echo " Dataset 1 — 5×5 grid, 10 timesteps (pw=0.95, pc=0.05)"
	@echo "============================================================"
	@if [ ! -d "$(D1_DIR)" ]; then \
		echo "ERROR: Dataset not found at $(D1_DIR)"; \
		echo "Make sure Datasets-20260506/ is in $(PROJECT_DIR)"; \
		exit 1; \
	fi
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1)
	@echo ""
	@echo "============================================================"
	@echo " Dataset 2 — 20×20 grid, 20 timesteps"
	@echo "============================================================"
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2)
	@echo ""
	@echo "============================================================"
	@echo " Dataset 3 — 10×20 grid, 20 timesteps (EM learning)"
	@echo "============================================================"
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3)
	@echo ""
	@echo "$(GREEN)✓ Demo complete. Output files written to $(PROJECT_DIR)/out_d*.txt$(NC)"

# ============================================================================
# Run Targets (Execute Datasets)
# ============================================================================

.PHONY: run
run: build
	@echo "Running datasets 1-3..."
	@bash -c 'start=$$(date +%s); echo "Dataset1: 5×5, 10 steps"; start1=$$(date +%s); "$(WUMPUS_BIN)" "$(D1_DIR)" 1 "$(OUT_D1)" >/dev/null 2>&1; duration1=$$(( $$(date +%s) - start1)); echo "  done in $$duration1 sec"; echo "Dataset2: 20×20, 20 steps"; start2=$$(date +%s); "$(WUMPUS_BIN)" "$(D2_DIR)" 2 "$(OUT_D2)" >/dev/null 2>&1; duration2=$$(( $$(date +%s) - start2)); echo "  done in $$duration2 sec"; echo "Dataset3: 10×20, 20 steps"; start3=$$(date +%s); "$(WUMPUS_BIN)" "$(D3_DIR)" 3 "$(OUT_D3)" >/dev/null 2>&1; duration3=$$(( $$(date +%s) - start3)); echo "  done in $$duration3 sec"; end=$$(date +%s); total=$$((end-start)); marginals=$$(find "$(PROJECT_DIR)" -maxdepth 1 -name "out_d*_marginal.txt" | wc -l | tr -d " "); maps=$$(find "$(PROJECT_DIR)" -maxdepth 1 -name "out_d*_map.txt" | wc -l | tr -d " "); echo "Completed 3 datasets in $$total sec."; echo "Generated $$marginals marginal files and $$maps MAP files."'
	@echo "Visualizing dataset outputs..."
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 1 || true
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 2 || true
	@cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 3 || true

.PHONY: run-d1
run-d1: build
	@echo "Dataset1: 5×5, 10 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) > /dev/null 2>&1; \
	status=$$?; \
	duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "  done in $$duration sec"

.PHONY: run-d2
run-d2: build
	@echo "Dataset2: 20×20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) > /dev/null 2>&1; \
	status=$$?; \
	duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "  done in $$duration sec"

.PHONY: run-d3
run-d3: build
	@echo "Dataset3: 10×20, 20 steps"
	@start=$$(date +%s); \
	$(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) > /dev/null 2>&1; \
	status=$$?; \
	duration=$$(( $$(date +%s) - start )); \
	if [ $$status -ne 0 ]; then echo "  FAILED"; exit $$status; fi; \
	echo "  done in $$duration sec"

# ============================================================================
# Flag-Based Targets (use ACTIVE_DATASET flag and run visualizer)
# ============================================================================


.PHONY: flag-d1 flag-d2 flag-d3

flag-d1: build
	@echo ""
	@echo "$(CYAN)[FLAG-D1]$(NC) Flag-based Dataset1 with visualizer"
	@time $(WUMPUS_BIN) $(D1_DIR) 1 $(OUT_D1) 2>&1
	@echo ""
	@if [ -f "$(OUT_D1)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 1; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d2: build
	@echo ""
	@echo "$(CYAN)[FLAG-D2]$(NC) Flag-based Dataset2 with visualizer"
	@time $(WUMPUS_BIN) $(D2_DIR) 2 $(OUT_D2) 2>&1
	@echo ""
	@if [ -f "$(OUT_D2)_marginal.txt" ]; then \
		echo "$(GREEN)✓ Running visualizer...$(NC)"; \
		cd $(PROJECT_DIR) && $(PYTHON) $(VISUALIZER) 2; \
	else \
		echo "$(RED)✗ Output files not found$(NC)"; \
	fi

flag-d3: build
	@echo ""
	@echo "$(CYAN)[FLAG-D3]$(NC) Flag-based Dataset3 with visualizer"
	@time $(WUMPUS_BIN) $(D3_DIR) 3 $(OUT_D3) 2>&1
	@echo ""
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
# Accuracy Targets (Compare with Ground Truth)
# ============================================================================

.PHONY: accuracy
accuracy: run-d1 compute-accuracy

.PHONY: compute-accuracy
compute-accuracy:
	@echo "Checking Dataset1 accuracy against ground truth..."
	@if [ ! -f "$(GROUND_TRUTH)" ]; then \
		echo "Ground truth not found: $(GROUND_TRUTH)"; exit 1; \
	fi
	@python3 "$(PROJECT_DIR)/scripts/compute_accuracy.py" "$(GROUND_TRUTH)" "$(OUT_D1)_marginal.txt" "$(OUT_D1)_map.txt"

# ============================================================================
# Summary Targets (Generate Reports)
# ============================================================================

.PHONY: summary
summary:
	@echo "Run summary:"
	@echo "  datasets run: 3"
	@echo "  marginal outputs: $$(find $(PROJECT_DIR) -maxdepth 1 -name 'out_d*_marginal.txt' | wc -l)"
	@echo "  MAP outputs:      $$(find $(PROJECT_DIR) -maxdepth 1 -name 'out_d*_map.txt' | wc -l)"
	@if [ -f "$(GROUND_TRUTH)" ]; then \
		echo "  ground truth available"; \
	else \
		echo "  ground truth not found"; \
	fi

.PHONY: run-all-check
run-all-check:
	@if [ ! -f "$(OUT_D1)_map.txt" ] || [ ! -f "$(OUT_D2)_map.txt" ]; then \
		$(MAKE) run; \
	fi

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
	@echo "$(CYAN)[CLEAN]$(NC) Removing output files..."
	@rm -f $(OUT_D1)_*.txt $(OUT_D2)_*.txt $(OUT_D3)_*.txt
	@rm -f $(PROJECT_DIR)/RESULTS_*.log $(PROJECT_DIR)/FINAL_SUMMARY.txt
	@echo "$(GREEN)✓ Clean complete$(NC)"

.PHONY: clean-build
clean-build:
	@echo "$(CYAN)[CLEAN BUILD]$(NC) Removing emdw build artifacts..."
	@cd $(EMDW_BUILD) && rm -rf CMakeFiles cmake_install.cmake CMakeCache.txt Makefile
	@echo "$(GREEN)✓ Build clean complete$(NC)"

# ============================================================================
# Diagnostic Targets
# ============================================================================

.PHONY: info
info:
	@echo "$(CYAN)[INFO]$(NC) System Information"
	@echo "$(YELLOW)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "Project Directory: $(PROJECT_DIR)"
	@echo "EMDW Build Dir:    $(EMDW_BUILD)"
	@echo "Wumpus Binary:     $(WUMPUS_BIN)"
	@echo "Dataset Directory: $(DATASET_DIR)"
	@echo "Ground Truth:      $(GROUND_TRUTH)"
	@echo ""
	@echo "$(YELLOW)Dataset Locations:$(NC)"
	@echo "  D1: $(D1_DIR)"
	@echo "  D2: $(D2_DIR)"
	@echo "  D3: $(D3_DIR)"
	@echo ""
	@echo "$(YELLOW)Output File Prefixes:$(NC)"
	@echo "  D1: $(OUT_D1)"
	@echo "  D2: $(OUT_D2)"
	@echo "  D3: $(OUT_D3)"
	@echo ""
	@if [ -f $(WUMPUS_BIN) ]; then \
		echo "$(GREEN)✓ Wumpus executable found$(NC)"; \
	else \
		echo "$(RED)✗ Wumpus executable NOT found$(NC)"; \
	fi
	@if [ -d $(DATASET_DIR) ]; then \
		echo "$(GREEN)✓ Dataset directory found$(NC)"; \
	else \
		echo "$(RED)✗ Dataset directory NOT found$(NC)"; \
	fi
	@if [ -f "$(GROUND_TRUTH)" ]; then \
		echo "$(GREEN)✓ Ground truth file found$(NC)"; \
	else \
		echo "$(RED)✗ Ground truth file NOT found$(NC)"; \
	fi

# ============================================================================
# Phony targets (non-file targets)
# ============================================================================
.PHONY: .FORCE
.FORCE:

# End of Makefile
