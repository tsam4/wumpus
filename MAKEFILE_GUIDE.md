# Wumpus Tracker - Comprehensive Makefile Guide

## Overview

This `Makefile` provides a complete build, test, and execution pipeline for the Wumpus Tracker project. It automates:

- **Building** the wumpus executable
- **Running unit tests** (basic and advanced)
- **Executing all 5 datasets** with appropriate configurations
- **Computing accuracy** metrics vs ground truth
- **Generating comprehensive summary reports**

## Quick Start

### Show Available Targets

```bash
make help
```

### Complete Pipeline (Recommended)

```bash
make all
```

Runs: build → test → run all datasets → compute accuracy → generate summary

### Just Build and Run Datasets

```bash
make build run
```

### Run Individual Datasets

```bash
make run-d1    # Dataset1: 5×5, 10 steps, known pw/pc
make run-d2    # Dataset2: 20×20, 20 steps, known pw/pc
make run-d3    # Dataset3: 10×20, 20 steps, learn pw/pc
make run-d4    # Dataset4: 4×4, 100 steps, learn occupancy
make run-d5    # Dataset5: 7×7, 200 steps, learn pw/pc+occ
```

## Main Targets

### Build Targets

#### `make build`

Compiles the wumpus executable using CMake + make.

```bash
make build
```

**Output:**

- Wumpus executable at `$EMDW_BUILD/src/bin/wumpus`
- Status message indicating success or failure

#### `make clean`

Removes all output files (trajectories, logs, summaries).

```bash
make clean
```

#### `make clean-build`

Removes emdw build artifacts (does NOT delete wumpus source).

```bash
make clean-build
```

---

### Test Targets

#### `make test`

Runs basic unit tests:

- `test_parser` - Grid file parsing
- `test_emission` - Emission model (pw/pc likelihood)
- `test_transition` - Transition model (random walk)
- `test_emdw_bp` - Factor construction and Belief Propagation

```bash
make test
```

#### `make test-advanced`

Runs advanced integration tests:

- `test_e2e_sim` - End-to-end simulation with accuracy validation
- `test_dataset2_scale` - Performance and memory scaling (20×20 grid)

```bash
make test-advanced
```

#### `make test-all`

Runs both basic and advanced tests.

```bash
make test-all
```

---

### Run Targets

#### `make run`

Executes all 5 datasets sequentially.

```bash
make run
```

**Runs:**

1. Dataset 1 (5×5, 10 steps)
2. Dataset 2 (20×20, 20 steps)
3. Dataset 3 (10×20, 20 steps, EM learning)
4. Dataset 4 (4×4, 100 steps, occupancy learning)
5. Dataset 5 (7×7, 200 steps, combined learning)

#### Individual Dataset Targets

Each dataset can be run independently:

```bash
# Dataset1: Fixed parameters (pw=0.95, pc=0.05)
make run-d1

# Dataset2: Fixed parameters (pw=0.90, pc=0.10)
make run-d2

# Dataset3: Learn pw/pc via EM (6 iterations)
make run-d3

# Dataset4: Learn occupancy via EM (6 iterations)
make run-d4

# Dataset5: Learn pw/pc + occupancy via EM (8 iterations)
make run-d5
```

**Output for Each Dataset:**

- `out_d<n>_marginal.txt` - Marginal trajectory (argmax per timestep)
- `out_d<n>_map.txt` - MAP trajectory (Viterbi, most likely complete path)

---

### Accuracy Targets

#### `make accuracy`

Computes and displays accuracy metrics for Dataset1 (which has ground truth).

```bash
make accuracy
```

**Outputs:**

- **Marginal Accuracy**: % of timesteps matching ground truth
- **MAP Accuracy**: % of timesteps matching ground truth
- Side-by-side comparison of trajectories

Example output:

```
[Ground Truth]
  Length: 10 timesteps
  First 5 steps: [(2, 4), (3, 4), (3, 3), (2, 3), (2, 4)]

[Marginal Trajectory]
  Length: 10 timesteps
  Accuracy: 100.0% (10/10 steps correct)
  First 5 steps: [(2, 4), (3, 4), (3, 3), (2, 3), (2, 4)]

[MAP Trajectory]
  Length: 10 timesteps
  Accuracy: 100.0% (10/10 steps correct)
  First 5 steps: [(2, 4), (3, 4), (3, 3), (2, 3), (2, 4)]
```

#### `make compute-accuracy`

(Called by `make accuracy`) Directly computes accuracy without re-running the dataset.

```bash
make compute-accuracy
```

---

### Summary Targets

#### `make summary`

Generates a comprehensive summary report showing:

- Output file sizes and line counts
- Accuracy metrics (for Dataset1)
- Dataset configurations (grid sizes, parameters, learning settings)
- Algorithm details (model, inference, learning)

```bash
make summary
```

**Sample Output:**

```
===========================================================================
               WUMPUS TRACKER - COMPREHENSIVE SUMMARY REPORT
===========================================================================

Generated: 2026-05-06 18:54:19
Project Directory: /Users/taros/Documents/DE424MP
Dataset Directory: /Users/taros/Documents/DE424MP/Datasets-20260506

===========================================================================
OUTPUT FILES
===========================================================================

Dataset1 (5×5, 10 steps):
  ✓ Marginal: /Users/taros/Documents/DE424MP/out_d1_marginal.txt
    Size: 40 bytes, Lines: 10
  ✓ MAP:      /Users/taros/Documents/DE424MP/out_d1_map.txt
    Size: 40 bytes, Lines: 10
  ✓ Marginal Accuracy: 100.0%
  ✓ MAP Accuracy:      100.0%

Dataset2 (20×20, 20 steps):
  ✓ Marginal: /Users/taros/Documents/DE424MP/out_d2_marginal.txt
    Size: 120 bytes, Lines: 20
  ✓ MAP:      /Users/taros/Documents/DE424MP/out_d2_map.txt
    Size: 120 bytes, Lines: 20
    [...]
```

---

### Meta Targets

#### `make all`

Complete pipeline: build → test-all → run → accuracy → summary

```bash
make all
```

This is the recommended target for full project validation.

#### `make info`

Displays system information and configuration:

```bash
make info
```

**Output:**

```
[INFO] System Information
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Project Directory: /Users/taros/Documents/DE424MP
EMDW Build Dir:    /Users/taros/devel/emdw/build
Wumpus Binary:     /Users/taros/devel/emdw/build/src/bin/wumpus
Dataset Directory: /Users/taros/Documents/DE424MP/Datasets-20260506
Ground Truth:      /Users/taros/Documents/DE424MP/Ground truth for dataset1-20260506/wumpus_trajectory.txt

Dataset Locations:
  D1: /Users/taros/Documents/DE424MP/Datasets-20260506/dataset1
  D2: /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2
  D3: /Users/taros/Documents/DE424MP/Datasets-20260506/dataset3
  D4: /Users/taros/Documents/DE424MP/Datasets-20260506/dataset4
  D5: /Users/taros/Documents/DE424MP/Datasets-20260506/dataset5

✓ Wumpus executable found
✓ Dataset directory found
✓ Ground truth file found
```

---

## Configuration Variables

### Overridable Make Variables

You can override default paths using environment variables:

```bash
# Use custom EMDW build directory
make build EMDW_BUILD=/path/to/emdw/build

# Use custom project directory
make run PROJECT_DIR=/path/to/wumpus

# Use custom dataset directory
make run DATASET_DIR=/path/to/datasets
```

### Key Paths (Set in Makefile)

```make
EMDW_BUILD    := $(HOME)/devel/emdw/build
PROJECT_DIR   := (auto-detected from Makefile location)
DATASET_DIR   := $(PROJECT_DIR)/Datasets-20260506
GROUND_TRUTH  := $(PROJECT_DIR)/Ground truth for dataset1-20260506/wumpus_trajectory.txt
WUMPUS_BIN    := $(EMDW_BUILD)/src/bin/wumpus
```

---

## Output Files Generated

### By `make run`

Each dataset generates two trajectory files:

- `out_d1_marginal.txt`, `out_d1_map.txt`
- `out_d2_marginal.txt`, `out_d2_map.txt`
- `out_d3_marginal.txt`, `out_d3_map.txt`
- `out_d4_marginal.txt`, `out_d4_map.txt`
- `out_d5_marginal.txt`, `out_d5_map.txt`

**Format:** Each line is a coordinate `x y` (space-separated, zero-indexed)

### By `make summary`

Generates comprehensive report to stdout (can be redirected to file):

```bash
make summary > SUMMARY.txt
```

### By `make accuracy`

Displays accuracy metrics to stdout

---

## Example Workflows

### Scenario 1: Full Project Validation

```bash
make all 2>&1 | tee project_run.log
```

Builds, tests, runs all datasets, computes accuracy, generates summary. Output saved to log file.

### Scenario 2: Quick Test Run (Small Dataset Only)

```bash
make run-d1 accuracy
```

Runs only Dataset1 and checks accuracy vs ground truth (~1 second).

### Scenario 3: Performance Testing

```bash
make test-advanced
```

Runs scaling and performance tests to validate efficiency.

### Scenario 4: Manual Parameter Testing

Run executable directly with custom parameters:

```bash
/Users/taros/devel/emdw/build/src/bin/wumpus \
  /Users/taros/Documents/DE424MP/Datasets-20260506/dataset1 \
  1 \
  /Users/taros/Documents/DE424MP/test_output \
  --pw 0.9 --pc 0.1 --em 3
```

---

## Dataset Specifications

| Dataset | Grid Size | Timesteps | pw/pc             | Learning              | Comment                    |
| ------- | --------- | --------- | ----------------- | --------------------- | -------------------------- |
| 1       | 5×5       | 10        | Known (0.95/0.05) | None                  | Baseline with ground truth |
| 2       | 20×20     | 20        | Known (0.90/0.10) | None                  | Scale test: 400 cells      |
| 3       | 10×20     | 20        | Unknown           | pw/pc (6 iters)       | Parameter estimation       |
| 4       | 4×4       | 100       | Known (0.95/0.05) | Occupancy (6 iters)   | Occupancy learning         |
| 5       | 7×7       | 200       | Unknown           | pw/pc + occ (8 iters) | Joint learning             |

---

## Troubleshooting

### "Command not found: make"

Install GNU make:

```bash
brew install make        # macOS
apt-get install make     # Ubuntu/Debian
```

### "Wumpus executable not found"

Run `make info` to check paths, then rebuild:

```bash
make clean-build build
```

### "Dataset directory not found"

Verify the dataset folder exists:

```bash
ls -la ~/Documents/DE424MP/Datasets-20260506/
```

### "Ground truth file not found"

Check the exact path:

```bash
ls -la "~/Documents/DE424MP/Ground truth for dataset1-20260506/"
```

### Python scripts not found

Verify scripts directory exists:

```bash
ls -la ~/Documents/DE424MP/scripts/
```

---

## Advanced Usage

### Capturing Full Output to File

```bash
make all > FULL_RUN_$(date +%Y%m%d_%H%M%S).log 2>&1
```

### Running Tests Only (No Data Execution)

```bash
make build test-all
```

### Running Only Large Datasets (D2-D5)

```bash
make run-d2 run-d3 run-d4 run-d5
```

### Cleaning Up and Starting Fresh

```bash
make clean-build clean build run accuracy summary
```

---

## File Structure

```
PROJECT_DIR/
├── Makefile                 # This file
├── scripts/
│   ├── compute_accuracy.py  # Accuracy computation helper
│   └── generate_summary.py  # Summary report generator
├── src/
│   ├── wumpus.cc
│   └── wumpus_model.cc
├── include/
│   ├── wumpus.hpp
│   └── wumpus_model.hpp
├── tests_basic/             # Unit test sources
├── tests_advanced/          # Integration test sources
├── Datasets-20260506/       # Input data
│   ├── dataset1/
│   ├── dataset2/
│   ├── dataset3/
│   ├── dataset4/
│   └── dataset5/
├── Ground truth for dataset1-20260506/
│   └── wumpus_trajectory.txt # Reference trajectory
└── out_d1_*.txt, out_d2_*.txt, ...  # Generated outputs
```

---

## See Also

- [README.md](README.md) - Project overview
- [RUN_AND_TESTS.md](RUN_AND_TESTS.md) - Detailed execution guide
- [mini_project.pdf](mini_project.pdf) - Project specification
