# Wumpus Tracker — DE424 Mini Project

Tracks a latent Wumpus through a grid world using noisy binary detections and Loopy Belief Propagation. Parameters are known for datasets 1–2; dataset 3 learns them via EM.

---

## Quick Start

```bash
# Build, run all three datasets, and visualize
make all
```

Or step by step:

```bash
make build        # compile wumpus + all test binaries
make test         # run the four unit tests
make run          # run datasets 1–3 and generate visualizations
make accuracy     # compare output against ground truth
```

Run a single dataset:

```bash
make run-d1       # dataset 1 only (no visualizer)
make run-d2
make run-d3
```

Visualize existing output without re-running:

```bash
make visualize-d1
make visualize-d2
make visualize-d3
```

Verbose run with trajectory preview:

```bash
make demo
```

All outputs (`.txt` trajectories, `.gif` animations) are written to `results/`.

---

## Datasets

| Dataset  | Grid  | Steps | pw   | pc   | Notes                          |
|----------|-------|-------|------|------|--------------------------------|
| dataset1 | 5×5   | 10    | 0.95 | 0.05 | Known params                   |
| dataset2 | 20×20 | 20    | 0.90 | 0.10 | Known params, larger grid      |
| dataset3 | 10×20 | 20    | ?    | ?    | Unknown pw/pc — learned via EM |

Dataset files live in `Datasets-20260506/dataset{1,2,3}/`. Each contains `data_file*.txt` grids (zero-indexed, one cell per line).

---

## Model

### HMM

The Wumpus position $X_t$ is the hidden state. It moves one cell per step (up/down/left/right); if the target is out of bounds the Wumpus stays put.

**Transition:**
$$P(X_t = j \mid X_{t-1} = i) = 0.25 \quad \text{for each valid neighbour}$$
$$P(X_t = i \mid X_{t-1} = i) \mathrel{+}= 0.25 \quad \text{per out-of-bounds direction from } i$$

**Emission:** each cell $k$ independently emits a binary observation:
$$P(Z_t^k = 1 \mid X_t = s) = \begin{cases} p_w & k = s \\ p_c & k \neq s \end{cases}$$

### Inference

The HMM is assembled as a Bethe cluster graph using [EMDW](https://github.com/emdw-team/emdw). Inference is **Loopy Belief Propagation** (sum-product). The output trajectory is the per-timestep argmax of the BP marginals.

### EM (Dataset 3)

For dataset 3, $p_w$ and $p_c$ are estimated by alternating:

- **E-step:** run BP to get posteriors $\gamma_t(s) = P(X_t = s \mid Z_{1:T})$
- **M-step:** update parameters from $\gamma_t$ and the observations

Default: 6 EM iterations.

---

## Repository Structure

```
wumpus/
├── src/
│   ├── wumpus.cc           # entry point: CLI, EM loop, output
│   └── wumpus_model.cc     # I/O, transition, emission, BP helpers
├── include/
│   ├── wumpus.hpp          # shared types (DatasetConfig, BPInferenceResult)
│   └── wumpus_model.hpp    # public interface for wumpus_model.cc
├── tests/
│   ├── test_parser.cc
│   ├── test_emission.cc
│   ├── test_transition.cc
│   └── test_emdw_bp.cc
├── scripts/
│   ├── visualize.py        # generates .gif animations from output files
│   └── compute_accuracy.py # compares marginal output to ground truth
├── emdw_de424/             # EMDW library (external dependency)
├── Makefile
├── CLAUDE.md               # AI assistant context file
└── mini_project.pdf        # project specification
```

---

## Tests

```bash
make test
```

Runs the four required tests. All should exit with code 0 and print `PASS`.

| Test | What it checks |
|---|---|
| `test_parser` | detection file parsing and row-major ordering |
| `test_emission` | emission factor correctness, log-space, clamping |
| `test_transition` | transition matrix: self-loops, row sums, edge/corner cells |
| `test_emdw_bp` | full BP integration on a 2×2 grid; marginals sum to 1, no NaN |

---

## Implementation Notes

- Emission likelihoods are computed in **log-space** then converted via the log-sum-exp trick before being passed to EMDW as factor potentials.
- `clamp_prob(p)` keeps all values in $(10^{-6},\, 1-10^{-6})$ to prevent $\log(0)$ during EM.
- Dataset files are sorted lexicographically to ensure correct temporal order.
