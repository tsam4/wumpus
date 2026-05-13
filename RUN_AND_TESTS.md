# Running and Testing the Wumpus Tracker

## CLI Reference

```
wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]
```

| Argument      | Description                                                       | Default     |
|---------------|-------------------------------------------------------------------|-------------|
| `dataset_dir` | Path to the dataset directory                                     | (required)  |
| `dataset_id`  | 1–3; selects known parameters or enables EM mode                  | (required)  |
| `out_prefix`  | Output file prefix (produces `_marginal.txt`, `_map.txt`)         | (required)  |
| `--pw v`      | Override detection probability pw                                 | From dataset|
| `--pc v`      | Override clutter probability pc                                   | From dataset|
| `--em n`      | Number of EM iterations for parameter learning                    | 0           |

---

## Running Each Dataset

#### Dataset 1: Known Parameters (5×5 grid, 10 steps)

```bash
src/bin/wumpus /path/to/Datasets/dataset1 1 out_d1
```

- pw = 0.95, pc = 0.05 (hardcoded)
- No learning required
- Output: `out_d1_marginal.txt`, `out_d1_map.txt`

#### Dataset 2: Known Parameters (20×20 grid, 20 steps)

```bash
src/bin/wumpus /path/to/Datasets/dataset2 2 out_d2
```

- pw = 0.95, pc = 0.05 (hardcoded)
- Output: `out_d2_marginal.txt`, `out_d2_map.txt`

#### Dataset 3: Unknown pw/pc (10×20 grid, 20 steps)

```bash
src/bin/wumpus /path/to/Datasets/dataset3 3 out_d3 --em 10
```

- pw/pc unknown; learned via EM (10 iterations recommended)
- Output: `out_d3_marginal.txt`, `out_d3_map.txt`

---

## Makefile Targets

```bash
make build          # Build wumpus executable
make test           # Run basic unit tests
make test-advanced  # Run advanced integration tests
make test-all       # Run all tests
make run            # Run datasets 1–3
make run-d1         # Run dataset 1 only
make run-d2         # Run dataset 2 only
make run-d3         # Run dataset 3 only
make accuracy       # Compare dataset1 output against ground truth
make summary        # Print run summary
make all            # Full pipeline: build + test + run + accuracy + summary
make clean          # Remove generated files and outputs
```

---

## Output Files

```
out_d1_marginal.txt    (10 lines)
out_d1_map.txt
out_d2_marginal.txt    (20 lines)
out_d2_map.txt
out_d3_marginal.txt    (20 lines)
out_d3_map.txt
```

Each file contains one `x y` coordinate pair per line.

---

## Unit Tests

### Basic Tests (`make test`)

- **test_parser.cc**: Grid file I/O — loads `data_file*.txt`, validates dimensions and values
- **test_emission.cc**: Emission model — verifies log-likelihoods for known pw/pc
- **test_transition.cc**: Transition model — verifies boundary probabilities (corner, edge, interior cells)
- **test_emdw_bp.cc**: BP factor construction — builds and runs LBP on a small 2-timestep chain

### Advanced Tests (`make test-advanced`)

- **test_e2e_sim.cc**: End-to-end accuracy — simulates a Wumpus trajectory, runs inference, checks accuracy
- **test_em_extremes.cc**: EM stability — verifies convergence on degenerate grids (all-0 or all-1 detections)
- **test_dataset2_scale.cc**: Performance test — runs inference on a 20×20 grid, checks memory usage (~15.5 MB)

### Adding Tests to CMakeLists.txt

```cmake
# Add symlink for test source
ln -s /path/to/tests/test_mytest.cc $EMDW_ROOT/src/bin/test_mytest.cc

# In emdw/src/bin/CMakeLists.txt, add:
add_executable(test_mytest test_mytest.cc wumpus_model.cc)
target_include_directories(test_mytest PRIVATE /path/to/include)
target_link_libraries(test_mytest emdw)
```

### Running Tests via CMake

```bash
cd $EMDW_ROOT/build
cmake ../; make -j7 \
         test_parser test_emission test_transition test_emdw_bp \
         test_dataset2_scale test_em_extremes test_e2e_sim

src/bin/test_parser
src/bin/test_emission
src/bin/test_transition
src/bin/test_emdw_bp
src/bin/test_e2e_sim
src/bin/test_em_extremes
src/bin/test_dataset2_scale
```

### One-Line Test Suite

```bash
src/bin/test_parser && \
src/bin/test_emission && \
src/bin/test_transition && \
src/bin/test_emdw_bp && \
src/bin/test_e2e_sim && \
src/bin/test_em_extremes && \
src/bin/test_dataset2_scale && \
echo "All tests passed"
```

---

## Test Outputs

| Test                    | Input                                          | Pass Indicator                    |
|-------------------------|------------------------------------------------|-----------------------------------|
| **test_parser**         | `dataset1/data_file*.txt`                      | "PASS" + grid dimensions          |
| **test_emission**       | Synthetic grid, pw=0.95 pc=0.05                | Log-likelihoods within tolerance  |
| **test_transition**     | 5×5 grid boundary cases                        | Probabilities sum to 1.0          |
| **test_emdw_bp**        | 2-step HMM chain                               | Posteriors sum to 1.0             |
| **test_e2e_sim**        | Simulated trajectory                           | Accuracy > threshold              |
| **test_em_extremes**    | Degenerate grids                               | No NaN, params in valid range     |
| **test_dataset2_scale** | 20×20 grid, 20 steps                           | Memory ≤ 16 MB, runtime < 5 s    |

---

## Accuracy Check

After running dataset1:

```bash
python3 scripts/compute_accuracy.py \
  "/path/to/Ground truth/wumpus_trajectory.txt" \
  out_d1_marginal.txt \
  out_d1_map.txt
```

---

## Result Status

| Test                  | Status | Notes                                          |
|-----------------------|--------|------------------------------------------------|
| **Parser**            | ✅ Pass | File loading correct for all grid sizes        |
| **Emission model**    | ✅ Pass | Log-likelihoods correct                        |
| **Transition model**  | ✅ Pass | Boundary probabilities correct                 |
| **emdw BP**           | ✅ Pass | Posteriors normalise correctly                 |
| **E2E simulation**    | ✅ Pass | Accuracy above threshold                       |
| **EM extremes**       | ✅ Pass | Stable on degenerate inputs                    |
| **Dataset2 scale**    | ✅ Pass | Performance efficient for 20×20 grid           |
