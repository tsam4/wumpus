# Wumpus Tracker — DE424 Mini Project

## Overview

This project tracks a latent Wumpus in a grid world using noisy, cluttered binary detections (as specified in `mini_project.pdf`). At each time step, every grid cell emits a 0/1 detection. The Wumpus moves one cell per step (up/down/left/right) but fails to move if the target is outside the grid. The task is to infer the Wumpus position at each time step and output a trajectory of x, y coordinates that matches the ground-truth file format described in the project brief.

## Datasets

| Dataset  | Grid  | Steps | pw   | pc   | Notes                         |
|----------|-------|-------|------|------|-------------------------------|
| dataset1 | 5×5   | 10    | 0.95 | 0.05 | Known params                  |
| dataset2 | 20×20 | 20    | 0.95 | 0.05 | Known params, larger grid     |
| dataset3 | 10×20 | 20    | ?    | ?    | Unknown pw/pc (learned via EM)|

## Model

### Hidden Markov Model (HMM)

The Wumpus position $X_t \in \{0, \ldots, R \cdot C - 1\}$ is the hidden state. Cells are indexed in row-major order: index $= y \cdot C + x$.

**Transition model:** The Wumpus picks a direction uniformly from 4 (up, down, left, right). The move fails if the target cell is out of bounds, and the Wumpus stays in its current cell.

$$P(X_t = j \mid X_{t-1} = i) = 0.25 \quad \text{for each valid (in-bounds) neighbour } j$$

$$P(X_t = i \mid X_{t-1} = i) \mathrel{+}= 0.25 \quad \text{for each out-of-bounds direction from } i$$

**Emission model:** Each cell $k$ independently generates a binary observation $Z_t^k$:

$$P(Z_t^k = 1 \mid X_t = s) = \begin{cases} p_w & \text{if } k = s \\ p_c & \text{if } k \neq s \end{cases}$$

### Probabilistic Graphical Model (PGM)

The HMM is assembled using [EMDW](https://github.com/emdw-team/emdw) factor graph inference:

- **Unary prior factor** $p(X_0)$: uniform over all cells
- **Unary emission factors** $p(Z_t \mid X_t)$ at each timestep
- **Pairwise transition factors** $p(X_t \mid X_{t-1})$ linking adjacent timesteps

Inference uses loopy belief propagation (LBP) on the Bethe cluster graph.

### EM for Parameter Learning (Dataset 3)

For dataset 3, $p_w$ and $p_c$ are unknown and estimated via EM:

**E-step:** Run BP to compute posteriors $\gamma_t(s) = P(X_t = s \mid Z_{1:T})$

**M-step:** Update parameters:

$$p_w = \frac{\sum_t \sum_s \gamma_t(s) \cdot Z_t^s}{\sum_t \sum_s \gamma_t(s)}$$

$$p_c = \frac{\sum_t \sum_s \gamma_t(s) \cdot \sum_{k \neq s} Z_t^k}{\sum_t \sum_s \gamma_t(s) \cdot (N - 1)}$$

where $N$ is the total number of cells.

### MAP Trajectory (Viterbi)

The MAP trajectory is computed via Viterbi dynamic programming:

$$\delta_t(s) = \max_{x} \left[\delta_{t-1}(x) + \log P(X_t = s \mid X_{t-1} = x)\right] + \log p(Z_t \mid X_t = s)$$

## Quick Start

```bash
# Build (from emdw/build/)
cmake ../; make -j7 wumpus

# Run datasets
./wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]

# Dataset 1
src/bin/wumpus /path/to/Datasets/dataset1 1 out_d1

# Dataset 2
src/bin/wumpus /path/to/Datasets/dataset2 2 out_d2

# Dataset 3 (EM learning)
src/bin/wumpus /path/to/Datasets/dataset3 3 out_d3 --em 10
```

### CLI Arguments

| Argument      | Description                                               | Default      |
|---------------|-----------------------------------------------------------|--------------|
| `dataset_dir` | Path to the dataset directory                             | (required)   |
| `dataset_id`  | 1–3; selects known parameters or enables EM mode          | (required)   |
| `out_prefix`  | Output file prefix (`_marginal.txt`, `_map.txt`)          | (required)   |
| `--pw v`      | Override detection probability                            | From dataset |
| `--pc v`      | Override clutter probability                              | From dataset |
| `--em n`      | Number of EM iterations for parameter learning            | 0            |

## Output Format

Each run produces two files:

- `<out_prefix>_marginal.txt` — argmax of per-timestep marginals, one `x y` per line
- `<out_prefix>_map.txt` — Viterbi MAP trajectory, one `x y` per line

## Using the Makefile

```bash
make build          # Build wumpus executable
make run            # Run datasets 1–3
make run-d1         # Run dataset 1 only
make run-d2         # Run dataset 2 only
make run-d3         # Run dataset 3 only
make test           # Run unit tests
make accuracy       # Compare dataset1 output to ground truth
make clean          # Remove generated files
```

## Project Structure

```
wumpus/
├── src/
│   ├── wumpus.cc          # Main: argument parsing, EM loop, output
│   └── wumpus_model.cc    # Model: transitions, emissions, BP, Viterbi
├── include/
│   ├── wumpus.hpp         # Data structures (Grid, DatasetConfig, InferenceResult)
│   └── wumpus_model.hpp   # Function declarations
├── tests_basic/
│   ├── test_parser.cc     # Grid file I/O tests
│   ├── test_emission.cc   # Emission model tests
│   ├── test_transition.cc # Transition model tests
│   └── test_emdw_bp.cc    # BP inference integration tests
├── tests_advanced/
│   ├── test_e2e_sim.cc    # End-to-end simulated accuracy test
│   ├── test_em_extremes.cc# EM stability on degenerate inputs
│   └── test_dataset2_scale.cc # Performance/memory test (20×20)
├── scripts/
│   ├── compute_accuracy.py
│   └── generate_summary.py
├── visualize.py
├── Makefile
└── README.md
```

## Tests

**Basic unit tests** (`make test`):
- `test_parser.cc`: Grid file loading and parsing
- `test_emission.cc`: Emission log-likelihood correctness
- `test_transition.cc`: Boundary transition probabilities
- `test_emdw_bp.cc`: BP factor construction and convergence

**Advanced integration tests** (`make test-advanced`):
- `test_e2e_sim.cc`: End-to-end accuracy on simulated trajectories
- `test_em_extremes.cc`: EM stability with degenerate (all-zero, all-one) observations
- `test_dataset2_scale.cc`: Performance and memory on 20×20 grid
