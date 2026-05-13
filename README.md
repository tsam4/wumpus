# Wumpus Tracker — DE424 Mini Project

## Overview

This project tracks a latent Wumpus in a grid world using noisy, cluttered
binary detections (as specified in `mini_project.pdf`). At each time step,
every grid cell emits a 0/1 detection. The Wumpus moves one cell per step
(up/down/left/right) but fails to move if the target is outside the grid.
The task is to infer the Wumpus position at each time step and output a
trajectory of x, y coordinates that matches the ground-truth file format.

## Datasets

| Dataset  | Grid  | Steps | pw   | pc   | Notes                          |
|----------|-------|-------|------|------|--------------------------------|
| dataset1 | 5×5   | 10    | 0.95 | 0.05 | Known params                   |
| dataset2 | 20×20 | 20    | 0.90 | 0.10 | Known params, larger grid      |
| dataset3 | 10×20 | 20    | ?    | ?    | Unknown pw/pc (learned via EM) |

## Model

### Hidden Markov Model (HMM)

The Wumpus position $X_t \in \{0, \ldots, R \cdot C - 1\}$ is the hidden
state. Cells are indexed in row-major order: index $= y \cdot C + x$.

**Transition model:** The Wumpus picks a direction uniformly from 4 (up,
down, left, right). The move fails if the target cell is out of bounds, and
the Wumpus stays in its current cell.

$$P(X_t = j \mid X_{t-1} = i) = 0.25 \quad \text{for each valid neighbour } j$$
$$P(X_t = i \mid X_{t-1} = i) \mathrel{+}= 0.25 \quad \text{for each out-of-bounds direction from } i$$

**Emission model:** Each cell $k$ independently generates a binary
observation $Z_t^k$:

$$P(Z_t^k = 1 \mid X_t = s) = \begin{cases} p_w & \text{if } k = s \\ p_c & \text{if } k \neq s \end{cases}$$

### Probabilistic Graphical Model (PGM)

The HMM is assembled as a Bethe cluster graph using [EMDW](https://github.com/emdw-team/emdw):

- **Unary prior factor** $p(X_0)$: uniform over all cells
- **Unary emission factors** $p(Z_t \mid X_t)$ at each timestep
- **Pairwise transition factors** $p(X_t \mid X_{t-1})$ linking adjacent timesteps

Inference uses **Loopy Belief Propagation (BP)** (sum-product) on the Bethe
cluster graph. This is the sole inference algorithm used — no Viterbi or
loopy belief update variants are included.

### EM for Parameter Learning (Dataset 3)

For dataset 3, $p_w$ and $p_c$ are unknown and estimated via EM:

**E-step:** Run BP to compute posteriors $\gamma_t(s) = P(X_t = s \mid Z_{1:T})$

**M-step:** Update parameters:

$$p_w = \frac{\sum_t \sum_s \gamma_t(s) \cdot Z_t^s}{\sum_t \sum_s \gamma_t(s)}$$

$$p_c = \frac{\sum_t \sum_s \gamma_t(s) \cdot \sum_{k \neq s} Z_t^k}{\sum_t \sum_s \gamma_t(s) \cdot (N - 1)}$$

where $N$ is the total number of cells.

The default is **6 EM iterations** for dataset 3.

### Output

One trajectory file is produced per run:

| File | Contents |
|------|----------|
| `<out_prefix>_marginal.txt` | `argmax_s P(X_t=s \| Z_{1:T})` per timestep |

File format: one line per timestep, `x y` (zero-indexed, space-separated).

## Repository Structure

```
wumpus/
├── src/
│   ├── wumpus.cc          # Main binary: CLI, dataset config, EM loop, output
│   ├── wumpus_model.cc    # All model logic: I/O, transition, emission, BP
│   ├── wumpus_model.hpp   # Public interface for wumpus_model.cc
│   └── wumpus.hpp         # Shared types (DatasetConfig, BPInferenceResult)
├── tests/
│   ├── basic/             # Required unit tests (per mini_project.pdf)
│   │   ├── test_transition.cc   # build_transition_adj correctness
│   │   ├── test_emission.cc     # compute_emission_log + scale_from_log
│   │   ├── test_parser.cc       # read_grid_file correctness
│   │   └── test_emdw_bp.cc      # Full BP integration on 2x2 grid
│   └── advanced/          # Extended tests
│       ├── test_em_extremes.cc  # EM robustness: degenerate observations
│       ├── test_e2e_sim.cc      # End-to-end accuracy on synthetic data
│       └── test_dataset2_scale.cc # 20x20 scalability (SKIP_SLOW=1 to skip)
├── emdw_de424/            # EMDW library (external dependency)
│   └── devel/emdw/
│       ├── CMakeLists.txt
│       └── src/
│           └── bin/
│               └── CMakeLists.txt  # Declares wumpus + all test executables
├── Makefile               # Convenience targets: tests_basic, tests_advanced
└── mini_project.pdf       # Project specification
```

## Quick Start

### 1. Build

```bash
# From inside the emdw build directory
cd emdw_de424/devel/emdw/build
cmake ../
make -j$(nproc) wumpus
```

Or via the Makefile from the repo root:

```bash
make wumpus
```

### 2. Run

```bash
# General usage
./wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]
```

| Argument      | Description                                          | Default      |
|---------------|------------------------------------------------------|--------------|
| `dataset_dir` | Path to the dataset directory                        | (required)   |
| `dataset_id`  | 1, 2, or 3; selects default parameters or EM mode   | (required)   |
| `out_prefix`  | Output file prefix (produces `_marginal.txt`)        | (required)   |
| `--pw v`      | Override detection probability                       | From dataset |
| `--pc v`      | Override clutter probability                         | From dataset |
| `--em n`      | Override number of EM iterations                     | From dataset |

**Dataset 1** (5×5, 10 steps, known params):
```bash
./wumpus /path/to/Datasets/dataset1 1 out_d1
# produces: out_d1_marginal.txt
```

**Dataset 2** (20×20, 20 steps, known params):
```bash
./wumpus /path/to/Datasets/dataset2 2 out_d2
# produces: out_d2_marginal.txt
```

**Dataset 3** (10×20, 20 steps, EM parameter learning):
```bash
./wumpus /path/to/Datasets/dataset3 3 out_d3
# produces: out_d3_marginal.txt
# runs 6 EM iterations by default; override with --em n
```

## Testing

### Basic tests (required per mini_project.pdf)

Builds and runs `test_transition`, `test_emission`, `test_parser`, `test_emdw_bp`:

```bash
make tests_basic
```

Or manually:
```bash
cd emdw_de424/devel/emdw/build
make -j$(nproc) test_transition test_emission test_parser test_emdw_bp
./src/bin/test_transition
./src/bin/test_emission
./src/bin/test_parser
./src/bin/test_emdw_bp
```

### Advanced tests

Builds and runs `test_em_extremes`, `test_e2e_sim`, `test_dataset2_scale`:

```bash
make tests_advanced
```

Skip the 20×20 scalability test in constrained environments:
```bash
SKIP_SLOW=1 make tests_advanced
```

### What each test covers

| Test | Suite | What it checks |
|------|-------|----------------|
| `test_transition` | basic | Corner self-loop=0.5, all rows sum to 1.0, interior cell, edge cell |
| `test_emission` | basic | True cell highest log-prob, `scale_from_log` range, `clamp_prob` |
| `test_parser` | basic | Valid file read, missing file, empty file, row-major ordering |
| `test_emdw_bp` | basic | 2×2 BP marginals correct, sum to 1.0, no NaN |
| `test_em_extremes` | advanced | EM stable on all-zero / all-one obs; pw, pc stay in (0,1) |
| `test_e2e_sim` | advanced | 3×3 synthetic: MAP ≥ 4/5 steps correct on clean data |
| `test_dataset2_scale` | advanced | 20×20 × 20 steps: no crash, no NaN (SKIP_SLOW=1 to skip) |

## Implementation Notes

### Inference algorithm

Only **Belief Propagation** (sum-product, `loopyBP_CG`) is used. The BP
posteriors $\gamma_t(s) = P(X_t = s \mid Z_{1:T})$ are computed after
convergence via `queryLBP_CG`, then normalised. The marginal trajectory
is the per-timestep argmax of these posteriors.

### Numerical stability

- Emission likelihoods are computed in **log-space** then converted back via
  the **log-sum-exp trick** (`scale_from_log`) before being handed to emdw
  as factor potentials.
- `clamp_prob(p)` keeps all probabilities in $(10^{-6},\, 1-10^{-6})$ to
  prevent $\log(0)$ during EM M-step updates.

### File naming convention

Dataset files must be named `data_fileNNN.txt` (zero-padded, e.g.
`data_file000.txt`) inside the dataset directory. They are sorted
alexicographically to ensure correct temporal order.
