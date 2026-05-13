# Run + Test Guide

This file documents exactly how to run the program, how to interpret outputs, how to run the tests, and the observed results. All model assumptions and dataset parameters are taken from `mini_project.pdf`.

---

## 1) How to Run the Program

**Executable:** `wumpus` (built inside emdw build tree)

**CLI:**
```
wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]
```

**Required args:**
- `dataset_dir`: path to a dataset folder containing `data_file*.txt` grids
- `dataset_id`: 1..3 (selects known parameters or enables EM mode)
- `out_prefix`: output file prefix

**Optional args:**
- `--pw v`: override detection probability
- `--pc v`: override clutter probability
- `--em n`: run n EM iterations to learn pw/pc

**Examples:**

```bash
# Dataset 1 (known pw=0.95, pc=0.05)
src/bin/wumpus /path/to/Datasets/dataset1 1 out_d1

# Dataset 2 (known pw=0.95, pc=0.05)
src/bin/wumpus /path/to/Datasets/dataset2 2 out_d2

# Dataset 3 (unknown pw/pc — EM learning)
src/bin/wumpus /path/to/Datasets/dataset3 3 out_d3 --em 10
```

---

## 2) File Roles

| File | Role |
|------|------|
| `src/wumpus.cc` | Main: argument parsing, dataset loading, EM loop, output writing |
| `src/wumpus_model.cc` | Model: transitions, emissions, BP pipeline, Viterbi MAP |
| `include/wumpus.hpp` | Core data structures: `Grid`, `DatasetConfig`, `InferenceResult` |
| `include/wumpus_model.hpp` | Function declarations for model helpers |
| `tests_basic/test_transition.cc` | Unit test: transition model boundary conditions |
| `tests_basic/test_emission.cc` | Unit test: emission log-likelihood correctness |
| `tests_basic/test_parser.cc` | Unit test: grid file parsing and dataset loading |
| `tests_basic/test_emdw_bp.cc` | Unit test: BP factor construction + marginals sum to 1 |
| `tests_advanced/test_e2e_sim.cc` | Integration test: end-to-end accuracy on simulated data |
| `tests_advanced/test_em_extremes.cc` | Integration test: EM stability on degenerate observations |
| `tests_advanced/test_dataset2_scale.cc` | Performance test: timing + memory on 20×20 grid |

---

## 3) Output Files and Interpretation

Each run produces **two output files**:

| File | Contents |
|------|----------|
| `<out_prefix>_marginal.txt` | Per-timestep argmax of posterior marginals: `P(X_t = s | Z_1:T)` |
| `<out_prefix>_map.txt` | Viterbi MAP trajectory: `argmax P(X_1:T | Z_1:T)` |

Each line is `x y` (column, row), zero-indexed, top-left origin — matching the ground-truth format in `mini_project.pdf`.

**Outputs from the reference run:**

```
out_d1_marginal.txt   (10 lines) ← matches ground truth
out_d1_map.txt        (10 lines) ← matches ground truth
out_d2_marginal.txt   (20 lines)
out_d2_map.txt        (20 lines)
out_d3_marginal.txt   (20 lines) ← EM converged
out_d3_map.txt        (20 lines) ← EM converged
```

---

## 4) How to Run Tests

All tests are built inside the emdw build tree as separate executables.

**Build:**
```bash
cd ~/devel/emdw/build
cmake ../
make -j7 wumpus test_transition test_emission test_parser test_emdw_bp \
         test_dataset2_scale test_em_extremes test_e2e_sim
```

**Run basic tests:**
```bash
src/bin/test_transition
src/bin/test_emission
src/bin/test_parser
src/bin/test_emdw_bp
```

**Run advanced tests:**
```bash
src/bin/test_dataset2_scale /path/to/Datasets/dataset2
src/bin/test_em_extremes
src/bin/test_e2e_sim
```

**Interpreting test output:**
- Most tests produce no output if all assertions pass.
- `test_dataset2_scale` prints one line with runtime and memory:
  ```
  dataset2 bp_ms=<ms> map_ms=<ms> maxrss=<bytes>
  ```
  On macOS, `ru_maxrss` is in bytes. On Linux it is in kilobytes.

---

## 5) Test Results (Reference Run)

**Build result:** Success, no compiler warnings from test targets.

**Runtime result:** All tests exited with code 0.

```
observed output from test_dataset2_scale:
dataset2 bp_ms=153 map_ms=0 maxrss=16056320
```

**Dataset 1 validation:**

```
out_d1_map.txt      → matches ground truth exactly (diff empty)
out_d1_marginal.txt → matches ground truth exactly (diff empty)
```

**Notes:**
- Dataset 2 has no ground truth provided; runtime was checked only.
- Dataset 3 has no ground truth provided; output generation and EM convergence were checked only.
- `test_e2e_sim.cc` asserts accuracy ≥ 0.8 on a deterministic mock scenario; it passed.
