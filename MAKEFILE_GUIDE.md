# Makefile Guide

This Makefile wraps common emdw build and run operations. All commands are designed to be run from the **project root** (the directory containing this file).

For full build details, see `README.md`. For run and test procedures, see `RUN_AND_TESTS.md`.

---

## Targets

### Build

```bash
make build
```
Builds the `wumpus` executable (and links against libemdw).

### Run Datasets

```bash
make run         # Runs datasets 1, 2, and 3
make run-d1      # Runs dataset 1 only
make run-d2      # Runs dataset 2 only
make run-d3      # Runs dataset 3 only (EM learning)
```

You must set `DATASET_DIR` to point to your datasets folder:
```bash
make run DATASET_DIR=/path/to/Datasets
```

### Test

```bash
make test           # Runs basic unit tests
make test-advanced  # Runs advanced integration + stress tests
```

### Accuracy Validation

```bash
make accuracy
```
Runs `scripts/compute_accuracy.py` to compare `out_d1_marginal.txt` and `out_d1_map.txt` against the ground truth from `mini_project.pdf`.

### Visualize

```bash
make visualize
```
Runs `visualize.py` to display a trajectory plot for the dataset1 output.

### Clean

```bash
make clean
```
Removes all generated output files (`out_d*.txt`, etc.).

---

## Variable Overrides

| Variable | Default | Description |
|---|---|---|
| `DATASET_DIR` | (required) | Path to your datasets folder |
| `EM_ITERS` | `10` | Number of EM iterations for dataset 3 |
| `EMDW_BUILD` | `~/devel/emdw/build` | Path to the emdw build directory |
