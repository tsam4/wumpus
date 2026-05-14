# Wumpus Tracker — Makefile
#
# Common targets:
#   make build       Build the wumpus executable and all unit tests
#   make test        Run the four unit tests (parser, emission, transition, BP)
#   make run         Run all three datasets, then visualize each
#   make demo        Same as run but prints a trajectory preview per dataset
#   make accuracy    Compare outputs against ground truth where available
#   make all         Full pipeline: build → test → run → accuracy
#   make clean       Delete results/ and emdw runtime artifacts
#
# Per-dataset convenience:
#   make run-d1/2/3        Run a single dataset (no visualizer)
#   make visualize-d1/2/3  Re-run visualizer on existing output files
#
# Environment you can override:
#   EMDW_BUILD   Path to the emdw cmake build dir  (default: repo-local)
#
# All output files (.txt trajectories, .gif animations) go into results/.
# emdw also writes err.txt and emdw_RandomEngine_seedVal.txt to the repo root
# (hardcoded inside emdw); make clean removes those too.

# --- Paths -------------------------------------------------------------------

PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
EMDW_BUILD  ?= $(PROJECT_DIR)/emdw_de424/devel/emdw/build
DATASET_DIR := $(PROJECT_DIR)/Datasets-20260506
RESULTS_DIR := $(PROJECT_DIR)/results
WUMPUS_BIN  := $(EMDW_BUILD)/src/bin/wumpus
WUMPUS_TEST_DIR := $(EMDW_BUILD)/src/bin
PYTHON      := $(if $(wildcard $(PROJECT_DIR)/.venv/bin/python),$(PROJECT_DIR)/.venv/bin/python,python3)
VISUALIZER  := $(PROJECT_DIR)/scripts/visualize.py
ACCURACY_SCRIPT := $(PROJECT_DIR)/scripts/compute_accuracy.py
GT_BASE     := $(PROJECT_DIR)

# Output prefixes — all outputs land in results/
OUT_D1 := $(RESULTS_DIR)/out_d1
OUT_D2 := $(RESULTS_DIR)/out_d2
OUT_D3 := $(RESULTS_DIR)/out_d3

# Dataset directories
D1_DIR := $(DATASET_DIR)/dataset1
D2_DIR := $(DATASET_DIR)/dataset2
D3_DIR := $(DATASET_DIR)/dataset3

# ANSI colours
RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[1;33m
CYAN   := \033[0;36m
NC     := \033[0m

# --- Default target ----------------------------------------------------------

.PHONY: help
help:
	@echo "Wumpus Tracker Makefile"
	@echo "Usage: make [target]"
	@echo "  build          Build the executable"
	@echo "  test           Run unit tests (parser, emission, transition, BP)"
	@echo "  run            Execute datasets 1-3 and visualize"
	@echo "  demo           Quick demo: build + run all 3 datasets with verbose output"
	@echo "  accuracy       Check accuracy vs ground truth for all available datasets"
	@echo "  all            Full pipeline: build, test, run, accuracy"
	@echo "  clean          Remove generated files (results/ + emdw runtime artifacts)"
	@echo ""
	@echo "Additional targets: run-d1 run-d2 run-d3 visualize-d1 visualize-d2 visualize-d3"
	@echo ""

# --- Build -------------------------------------------------------------------

.PHONY: build
build:
	@cmake -S emdw_de424/devel/emdw -B emdw_de424/devel/emdw/build -DCMAKE_BUILD_TYPE=Release -Wno-dev > /dev/null
	@cmake --build emdw_de424/devel/emdw/build --parallel 12 2>&1 | grep -E "^\[|error:|warning:" | grep -v "warning:" || true
	@echo "$(GREEN)✓ Build complete$(NC)"

# --- Tests -------------------------------------------------------------------
# Covers the four components required by the project spec:
#   test_parser     — detection file parsing
#   test_emission   — emission factor (pw / pc)
#   test_transition — transition factor (wumpus movement)
#   test_emdw_bp    — belief propagation via emdw

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

# --- Demo --------------------------------------------------------------------
# Runs each dataset and prints a short trajectory preview (first/last 5 steps)
# then immediately visualizes. Handy for a quick sanity check.
#
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
	echo "  Last 5:  $$last5"; \
	echo "$(CYAN)  Visualizing...$(NC)"; \
	$(PYTHON) $(VISUALIZER) $(1)
endef

.PHONY: demo
demo: build
	$(call demo_run,1,5x5 grid  10 timesteps  pw=0.95 pc=0.05,$(D1_DIR),$(OUT_D1),known parameters - single BP pass)
	$(call demo_run,2,20x20 grid  20 timesteps  pw=0.90 pc=0.10,$(D2_DIR),$(OUT_D2),known parameters - single BP pass)
	$(call demo_run,3,10x20 grid  20 timesteps  EM learning,$(D3_DIR),$(OUT_D3),unknown pw/pc - 6 EM iterations then final BP pass)
	@echo ""
	@echo "$(GREEN)✓ Demo complete$(NC)"

# --- Run ---------------------------------------------------------------------
# Runs all three datasets quietly, then visualizes each in turn.
# For a noisier run with trajectory previews, use make demo instead.

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
	@echo "Generating visualizations..."
	@$(PYTHON) $(VISUALIZER) 1
	@$(PYTHON) $(VISUALIZER) 2
	@$(PYTHON) $(VISUALIZER) 3
	@echo "$(GREEN)✓ Visualizations complete$(NC)"

# Single-dataset run targets — no visualizer, useful for iterating quickly.
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

# Re-run the visualizer on existing output files without rerunning wumpus.
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

# --- Accuracy ----------------------------------------------------------------
# Compares each dataset's marginal output against the ground truth trajectory
# if a matching "Ground truth for dataset<N>-*" directory exists at repo root.
# Silently skips datasets with no ground truth.
#
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

# --- Combined ----------------------------------------------------------------

.PHONY: all
all: build test run accuracy
	@echo ""
	@echo "$(GREEN)✓✓✓ COMPLETE PIPELINE FINISHED ✓✓✓$(NC)"
	@echo ""

# --- Cleanup -----------------------------------------------------------------

.PHONY: clean
clean:
	@echo "Removing results/ and emdw runtime artifacts..."
	@rm -rf $(RESULTS_DIR)
	@rm -f $(PROJECT_DIR)/err.txt $(PROJECT_DIR)/emdw_RandomEngine_seedVal.txt
	@echo "$(GREEN)✓ Clean complete$(NC)"

# Removes cmake-generated files from the build dir without touching source.
# Run this if cmake gets confused after moving files or switching branches.
.PHONY: clean-build
clean-build:
	@echo "Removing emdw build artifacts..."
	@rm -rf $(EMDW_BUILD)/CMakeFiles $(EMDW_BUILD)/cmake_install.cmake \
	         $(EMDW_BUILD)/CMakeCache.txt $(EMDW_BUILD)/Makefile
	@echo "$(GREEN)✓ Build clean complete$(NC)"

# --- Diagnostics -------------------------------------------------------------

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

# -----------------------------------------------------------------------------
.PHONY: .FORCE
.FORCE:
