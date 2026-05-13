# Run & Test Guide

Documentation of how to run the wumpus program, interpret outputs, execute tests, and review results from execution on 6 May 2026. All model assumptions and dataset parameters follow `mini_project.pdf`.

---

## 1. How to Run the Program

### Executable Location

The `wumpus` executable is built inside the emdw build directory.

### Command-Line Interface

```
wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n] [--occ prior]
```

### Required Arguments

| Argument      | Description                                                                              |
| ------------- | ---------------------------------------------------------------------------------------- |
| `dataset_dir` | Path to dataset folder containing `data_file*.txt` grids                                 |
| `dataset_id`  | 1–5; selects known parameters or enables EM/occupancy modes                              |
| `out_prefix`  | Prefix for output files (creates `<out_prefix>_marginal.txt` and `<out_prefix>_map.txt`) |

### Optional Arguments

| Argument      | Description                                            | Default         |
| ------------- | ------------------------------------------------------ | --------------- |
| `--pw v`      | Override detection probability for true cell           | From dataset_id |
| `--pc v`      | Override clutter probability                           | From dataset_id |
| `--em n`      | Run EM for n iterations (enables parameter estimation) | 0 (no EM)       |
| `--occ prior` | Override occupancy prior                               | From dataset_id |

### Example Commands (Executed)

#### Dataset 1: Known Parameters (5×5 grid, 10 steps)

```bash
src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset1 1 \
  /Users/taros/Documents/DE424MP/out_d1
```

- Parameters: `pw=0.95, pc=0.05` (fixed)
- Output: `out_d1_marginal.txt`, `out_d1_map.txt`

#### Dataset 2: Known Parameters (20×20 grid, 20 steps)

```bash
src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2 2 \
  /Users/taros/Documents/DE424MP/out_d2
```

- Parameters: `pw=0.90, pc=0.10` (fixed)
- Output: `out_d2_marginal.txt`, `out_d2_map.txt`

#### Dataset 3: Unknown Parameters (10×20 grid, 20 steps)

```bash
src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset3 3 \
  /Users/taros/Documents/DE424MP/out_d3 --em 6
```

- Parameters: `pw, pc` unknown; estimated via EM (6 iterations)
- Output: `out_d3_marginal.txt`, `out_d3_map.txt`

#### Dataset 4: Unknown Occupancy (4×4 grid, 100 steps)

```bash
src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset4 4 \
  /Users/taros/Documents/DE424MP/out_d4 --em 6
```

- Parameters: `pw=0.95, pc=0.05` (fixed)
- Occupancy: Prior 25%; unknown cells estimated via EM (6 iterations)
- Output: `out_d4_marginal.txt`, `out_d4_map.txt`

#### Dataset 5: Unknown Parameters & Occupancy (7×7 grid, 200 steps)

```bash
src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset5 5 \
  /Users/taros/Documents/DE424MP/out_d5 --em 8
```

- Parameters: `pw, pc` unknown; estimated via EM (8 iterations)
- Occupancy: Prior 10%; unknown cells estimated via EM (8 iterations)
- Output: `out_d5_marginal.txt`, `out_d5_map.txt`

---

## 2. File Roles

### Source Code

| File                  | Purpose                                                                                                |
| --------------------- | ------------------------------------------------------------------------------------------------------ |
| `src/wumpus.cc`       | Main program: dataset loading, factor construction, BP inference, EM parameter updates, output writing |
| `src/wumpus_model.cc` | Shared utilities: grid IO, transition model, emission likelihoods, Viterbi MAP algorithm               |

### Headers

| File                       | Purpose                                                    |
| -------------------------- | ---------------------------------------------------------- |
| `include/wumpus.hpp`       | Shared structs: `Grid`, `DatasetConfig`, `InferenceResult` |
| `include/wumpus_model.hpp` | Helper function declarations                               |

### Tests

| Directory         | Purpose                                             |
| ----------------- | --------------------------------------------------- |
| `tests_basic/`    | Minimal unit-level tests for core model constraints |
| `tests_advanced/` | Stress tests and edge-case validation               |

---

## 3. Output Files and Interpretation

### Output Format

Program outputs **two files per run:**

1. **`<out_prefix>_marginal.txt`**: Posterior marginal trajectory (sum-product BP)
2. **`<out_prefix>_map.txt`**: Maximum A Posteriori trajectory (Viterbi)

Each line contains a single trajectory step: `x y`

- `x` = column index (0 = leftmost)
- `y` = row index (0 = topmost)
- Zero-indexed coordinates with origin at top-left
- Matches ground-truth format in `mini_project.pdf`

### Interpreting Results

**Marginal trajectory (`*_marginal.txt`):**

- Represents the most likely position at each time step, independently
- Computed as $\arg\max_s p(X_t = s \mid Z_{1:T})$ using BP marginals
- Optimal for single-step accuracy

**MAP trajectory (`*_map.txt`):**

- Represents the most likely complete path through the entire sequence
- Computed using Viterbi algorithm (max-product for HMM chains)
- Optimal for joint sequence accuracy

### Generated Output Files (This Run)

```
out_d1_marginal.txt
out_d1_map.txt
out_d2_marginal.txt
out_d2_map.txt
out_d3_marginal.txt
out_d3_map.txt
out_d4_marginal.txt
out_d4_map.txt
out_d5_marginal.txt
out_d5_map.txt
```

---

## 4. How to Run Tests

### Test Categories

#### Basic Tests (Unit-level constraints)

- **test_transition.cc**: Transition model boundary and blocked-cell logic
- **test_emission.cc**: Emission likelihood for $p_w$ and $p_c$
- **test_parser.cc**: Grid file parsing and dataset loading
- **test_emdw_bp.cc**: Sanity check that BP executes

#### Advanced Tests (Stress & edge cases)

- **test_dataset2_scale.cc**: Performance metrics (timing, memory)
- **test_em_extremes.cc**: EM stability on extreme grids (all 0s, all 1s)
- **test_occ_prior_stress.cc**: Occupancy updates with heavy blocking
- **test_e2e_sim.cc**: End-to-end accuracy on deterministic mock scenario

### Build Commands

```bash
cd ~/devel/emdw/build

# Reconfigure and build all tests
cmake ../
make -j7 wumpus test_transition test_emission test_parser test_emdw_bp \
         test_dataset2_scale test_em_extremes test_occ_prior_stress test_e2e_sim
```

### Run Commands

```bash
# Run all basic tests
src/bin/test_transition
src/bin/test_emission
src/bin/test_parser
src/bin/test_emdw_bp

# Run all advanced tests
src/bin/test_dataset2_scale /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2
src/bin/test_em_extremes
src/bin/test_occ_prior_stress
src/bin/test_e2e_sim
```

### Or Run All at Once

```bash
cd ~/devel/emdw/build && \
src/bin/test_transition && \
src/bin/test_emission && \
src/bin/test_parser && \
src/bin/test_emdw_bp && \
src/bin/test_dataset2_scale /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2 && \
src/bin/test_em_extremes && \
src/bin/test_occ_prior_stress && \
src/bin/test_e2e_sim
```

### Interpreting Test Output

| Test                      | Output on Pass                                   | Output on Fail              |
| ------------------------- | ------------------------------------------------ | --------------------------- |
| **test_transition**       | (none)                                           | Assertion message + exit(1) |
| **test_emission**         | (none)                                           | Assertion message + exit(1) |
| **test_parser**           | (none)                                           | Assertion message + exit(1) |
| **test_emdw_bp**          | (none)                                           | Assertion message + exit(1) |
| **test_dataset2_scale**   | `dataset2 bp_ms=<ms> map_ms=<ms> maxrss=<bytes>` | Assertion message + exit(1) |
| **test_em_extremes**      | (none)                                           | Assertion message + exit(1) |
| **test_occ_prior_stress** | (none)                                           | Assertion message + exit(1) |
| **test_e2e_sim**          | (none)                                           | Assertion message + exit(1) |

**Exit codes:**

- `0` = Pass
- `1` = Fail (assertion or explicit error)

---

## 5. Test Results from This Run

### Build Result

✅ **Success** – No compiler warnings from test targets.

### Runtime Result

✅ **All tests exited with code 0** – All assertions passed.

### Performance Metrics

**test_dataset2_scale output:**

```
dataset2 bp_ms=153 map_ms=0 maxrss=16056320
```

- **bp_ms**: Belief propagation inference time = 153 ms
- **map_ms**: Viterbi MAP computation time = 0 ms (rounding to integer milliseconds)
- **maxrss**: Peak resident set size = 16,056,320 bytes ≈ 15.3 MB (macOS native units)

### Validation Against Ground Truth

**Dataset 1 accuracy:**

- `out_d1_map.txt` matches provided ground truth **exactly** (diff was empty)
- `out_d1_marginal.txt` matches provided ground truth **exactly**
- ✅ Trajectory parsing and coordinate convention validated

**Datasets 2–5 validation:**

- No ground truth files in workspace; only generation and format checked
- ✅ All output files generated without errors
- ✅ Output format (x y coordinates) validated

### Test Coverage Summary

| Category                | Result  | Details                                          |
| ----------------------- | ------- | ------------------------------------------------ |
| **Transition model**    | ✅ Pass | Boundary and blocked-cell logic correct          |
| **Emission model**      | ✅ Pass | $p_w$ and $p_c$ likelihood computation correct   |
| **Grid parsing**        | ✅ Pass | File IO and dataset loading robust               |
| **BP inference**        | ✅ Pass | emdw integration and marginal extraction working |
| **Performance**         | ✅ Pass | Dataset2 completes in ~153 ms, ~15 MB memory     |
| **EM stability**        | ✅ Pass | Parameter updates converge on extreme grids      |
| **Occupancy updates**   | ✅ Pass | Cell blocking correctly modeled and estimated    |
| **End-to-end accuracy** | ✅ Pass | Achieves ≥80% accuracy on deterministic scenario |

### Summary

All tests pass without warnings. Program outputs match ground truth for dataset1. EM iterations complete successfully on all datasets. Occupancy estimation converges despite heavy blocking constraints. Performance is efficient for all grid sizes tested.
