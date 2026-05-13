# Makefile Guide

## Overview

The Makefile automates building, testing, running, and reporting for the Wumpus Tracker project. All dataset-related targets operate on datasets 1–3 only.

---

## Environment Variables

| Variable       | Default                          | Description                         |
|----------------|----------------------------------|-------------------------------------|
| `EMDW_BUILD`   | `~/devel/emdw/build`             | Path to emdw build directory        |
| `PROJECT_DIR`  | Auto-detected from Makefile path | Project root directory              |
| `DATASET_DIR`  | `$(PROJECT_DIR)/Datasets-20260506` | Path to datasets                  |
| `GROUND_TRUTH` | Auto-detected from PROJECT_DIR   | Ground truth trajectory for dataset1|
| `WUMPUS_BIN`   | `$(EMDW_BUILD)/src/bin/wumpus`   | Path to compiled wumpus binary      |

Override example:
```bash
make run EMDW_BUILD=/custom/path/to/emdw/build
```

---

## Targets

### build
```bash
make build
```
Compiles the `wumpus` executable via cmake + make.

### test / test-advanced / test-all
```bash
make test           # Basic unit tests
make test-advanced  # Integration tests
make test-all       # All tests
```

**Basic tests:** `test_parser`, `test_emission`, `test_transition`, `test_emdw_bp`

**Advanced tests:** `test_e2e_sim`, `test_em_extremes`, `test_dataset2_scale`

### run
```bash
make run
```
Runs all three datasets sequentially and generates output files.

### run-d1 / run-d2 / run-d3
```bash
make run-d1   # Dataset1: 5×5, 10 steps
make run-d2   # Dataset2: 20×20, 20 steps
make run-d3   # Dataset3: 10×20, 20 steps (EM)
```

### flag-d1 / flag-d2 / flag-d3
```bash
make flag-d1  # Run Dataset1 with timing and visualizer
make flag-d2  # Run Dataset2 with timing and visualizer
make flag-d3  # Run Dataset3 with timing and visualizer
```

These targets run the binary with output to stdout (useful for debugging) and automatically invoke the visualizer.

### visualize-d1 / visualize-d2 / visualize-d3
```bash
make visualize-d1   # Visualize dataset1 outputs
make visualize-d2   # Visualize dataset2 outputs
make visualize-d3   # Visualize dataset3 outputs
```

Runs `visualize.py` on existing output files without recompiling.

### accuracy
```bash
make accuracy
```
Runs dataset1 and compares the output against the ground truth file using `scripts/compute_accuracy.py`.

### summary
```bash
make summary
```
Prints a summary of runs: datasets executed, output files found, runtime.

### all
```bash
make all
```
Full pipeline: build → test → run → accuracy → summary.

### clean
```bash
make clean
```
Removes generated output files (`out_d*.txt`), logs, and summary files.

---

## Dataset Configurations

| ID | Grid  | Steps | pw   | pc   | Mode                     |
|----|-------|-------|------|------|--------------------------|
| 1  | 5×5   | 10    | 0.95 | 0.05 | Known parameters         |
| 2  | 20×20 | 20    | 0.95 | 0.05 | Known parameters         |
| 3  | 10×20 | 20    | ?    | ?    | EM parameter learning    |

---

## Common Workflows

### Full run from scratch
```bash
make all
```

### Run only dataset3 (EM learning)
```bash
make run-d3
```

### Debug a single dataset with verbose output
```bash
make flag-d1
```

### Run only datasets 2 and 3
```bash
make run-d2 run-d3
```

### Run all tests without building
```bash
make test-all
```

---

## Output Files

| File                  | Content                                  |
|-----------------------|------------------------------------------|
| `out_d1_marginal.txt` | Dataset1 argmax marginals (10 lines)     |
| `out_d1_map.txt`      | Dataset1 Viterbi MAP path (10 lines)     |
| `out_d2_marginal.txt` | Dataset2 argmax marginals (20 lines)     |
| `out_d2_map.txt`      | Dataset2 Viterbi MAP path (20 lines)     |
| `out_d3_marginal.txt` | Dataset3 argmax marginals (20 lines)     |
| `out_d3_map.txt`      | Dataset3 Viterbi MAP path (20 lines)     |

---

## Directory Layout

```
wumpus/
├── Makefile
├── src/
│   ├── wumpus.cc
│   └── wumpus_model.cc
├── include/
│   ├── wumpus.hpp
│   └── wumpus_model.hpp
├── tests_basic/
│   ├── test_parser.cc
│   ├── test_emission.cc
│   ├── test_transition.cc
│   └── test_emdw_bp.cc
├── tests_advanced/
│   ├── test_e2e_sim.cc
│   ├── test_em_extremes.cc
│   └── test_dataset2_scale.cc
├── scripts/
│   ├── compute_accuracy.py
│   └── generate_summary.py
├── visualize.py
├── Datasets-20260506/
│   ├── dataset1/
│   ├── dataset2/
│   └── dataset3/
└── Ground truth for dataset1-20260506/
    └── wumpus_trajectory.txt
```
