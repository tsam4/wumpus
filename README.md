# Wumpus Tracking with Probabilistic Inference

A comprehensive implementation of the Wumpus tracking problem from DE424MP, using Hidden Markov Models with belief propagation for inference and expectation-maximization for parameter estimation.

---

## Level 1: The Domain Definition

This project tracks a latent Wumpus in a grid world using noisy, cluttered binary detections (as specified in `mini_project.pdf`). At each time step, every grid cell emits a 0/1 detection. The Wumpus moves one cell per step (up/down/left/right) but can fail to move if the target is outside the grid or blocked (in datasets 4 and 5). The task is to infer the Wumpus position at each time step and output a trajectory of x, y coordinates that matches the ground-truth file format described in the project brief.

### Core Dataset Facts (from mini_project.pdf)

| Dataset  | Grid  | Steps | pw   | pc   | Notes                                |
| -------- | ----- | ----- | ---- | ---- | ------------------------------------ |
| dataset1 | 5×5   | 10    | 0.95 | 0.05 | Known parameters                     |
| dataset2 | 20×20 | 20    | 0.90 | 0.10 | Known parameters, larger scale       |
| dataset3 | 10×20 | 20    | ?    | ?    | Unknown pw/pc, use EM                |
| dataset4 | 4×4   | 100   | 0.95 | 0.05 | 25% of cells occupied, unknown       |
| dataset5 | 7×7   | 200   | ?    | ?    | 10% of cells occupied, unknown pw/pc |

### Coordinate Convention

Following `mini_project.pdf` Figure 1, output lines are formatted as `x y` for each time step. In code:

- Map grid coordinates to state index: `s = y*cols + x`
- `y` is the row index in the text file (top line is `y=0`)
- `x` is the column index (leftmost is `x=0`)
- Coordinates are zero-indexed with origin at top-left

---

## Level 2: The PGM Theoretical Blueprint

### Model Structure

**Hidden state:** $X_t \in \{0, 1, \ldots, M \cdot N - 1\}$, where each value represents a grid cell index $s = y \cdot \text{cols} + x$.

**Observations:** $Z_t$ is the complete grid of binary detections $z_k$ for all cells $k$.

### Transition Model: Random Walk with Failure

The Wumpus implements a random walk on the grid with failure modes:

1. **Basic Movement:** The Wumpus picks a direction uniformly from 4 (up, down, left, right)
2. **Failure Conditions:** The move fails if:
   - The target cell is outside the grid (boundary)
   - The target cell is blocked/occupied (in datasets 4 and 5)

For datasets **without occupancy**, the transition for each neighbor $j$ of state $i$ is:

$$P(X_t = j \mid X_{t-1} = i) = 0.25$$

for all valid neighbors, with self-transitions accumulating failed moves at boundaries.

For datasets **with occupancy**, let $N(i)$ be the valid 4-neighbors of cell $i$, and $\text{occ}_j$ be the occupancy probability of cell $j$:

$$P(X_t = j \mid X_{t-1} = i) = 0.25 \cdot (1 - \text{occ}_j) \quad \text{for } j \in N(i)$$

$$P(X_t = i \mid X_{t-1} = i) = 0.25 \cdot (\text{# out-of-bounds directions}) + \sum_{j \in N(i)} 0.25 \cdot \text{occ}_j$$

### Emission Model: Noisy Detection with Clutter

The observation grid is conditionally independent given $X_t$. If $X_t = c$ (the true state):

- **True cell:** emits $z_c = 1$ with probability $p_w$ (miss with probability $1 - p_w$)
- **Clutter cells:** each $k \neq c$ emits $z_k = 1$ with probability $p_c$ (true negative $1 - p_c$)

The likelihood of the entire grid is:

$$P(Z_t \mid X_t = c) = \left[ p_w^{[z_c=1]} (1-p_w)^{[z_c=0]} \right] \cdot \prod_{k \neq c} \left[ p_c^{[z_k=1]} (1-p_c)^{[z_k=0]} \right]$$

This likelihood is folded into a unary factor over $X_t$ by evaluating the product for each possible state $c$.

---

## Level 3: The emdw Framework Integration

### Implementation with Discrete Factors

Factors and random variables are implemented using `emdw::DiscreteTable<int>` (per `userguide.pdf`). For each time step $t$:

- **Unary prior factor** $p(X_0)$ over RV $X_0$: uniform initially, or weighted by open-cell prior when occupancy is modeled
- **Unary emission factor** $\psi_t(X_t)$: computed from observed grid $Z_t$ using the emission likelihood
- **Binary transition factor** $\phi_t(X_t, X_{t-1})$: sparse entries from the random-walk model

### Cluster Graph and Belief Propagation

Inference is performed using loopy belief propagation (BP):

1. **Cluster Graph Construction:** Build a Bethe cluster graph over all factors using `emdw::ClusterGraph::BETHE`
2. **Inference:** Call `emdw::loopyBP_CG()` for sum-product inference to compute marginals
3. **Marginalization:** Extract marginals with `emdw::queryLBP_CG()` and normalize to get $p(X_t \mid Z_{1:T})$
4. **Trajectory Selection:**
   - **Marginal trajectory:** `argmax_s p(X_t = s \mid Z_{1:T})` at each time $t$
   - **MAP trajectory:** Viterbi algorithm (max-product equivalent for HMM chains)

### MAP Computation via Viterbi

While emdw provides sum-product BP, it does not expose a max-product API in the provided headers. For Maximum A Posteriori (MAP) trajectory computation, we use the **Viterbi algorithm**, which is mathematically equivalent to max-product inference for chain-structured HMMs. Viterbi uses:

- The same transition probabilities as in the HMM
- The same emission log-likelihoods computed for the factors
- Dynamic programming for $O(T \cdot S^2)$ complexity (where $T$ is time steps, $S$ is state space)

---

## Level 4: Edge Case Logic & Parameter Estimation

### EM for Unknown Detection Parameters (Datasets 3 & 5)

**Datasets 3 and 5** have unknown $p_w$ and $p_c$ values. We use Expectation-Maximization:

**E-step:** Compute posterior occupancy using BP marginals:
$$\gamma_t(s) = P(X_t = s \mid Z_{1:T})$$

**M-step:** Update detection parameters using expected sufficient statistics. Let $z_t(k) \in \{0, 1\}$ be the observation at cell $k$, and $N = \text{rows} \times \text{cols}$:

- **Expected true detections:** $\sum_t \sum_s \gamma_t(s) \cdot z_t(s)$
- **Expected clutter detections:** $\sum_t \left[ \sum_k z_t(k) - \sum_s \gamma_t(s) \cdot z_t(s) \right]$

Parameter updates:

$$p_w = \frac{1}{T} \cdot \text{expected true detections}$$

$$p_c = \frac{1}{T \cdot (N-1)} \cdot \text{expected clutter detections}$$

**Numerical stability:** Clamp $p_w$ and $p_c$ to $(10^{-6}, 1 - 10^{-6})$ to avoid $\log(0)$ in emission factors.

### Occupancy Cell Estimation (Datasets 4 & 5)

**Datasets 4 and 5** have unknown occupied cells. Each cell $j$ has a prior occupancy from `mini_project.pdf`:

- **dataset4:** $P(\text{occ}_j = 1) = 0.25$
- **dataset5:** $P(\text{occ}_j = 1) = 0.10$

We estimate soft occupancy probability per cell using pairwise posteriors. Define:

- **Attempts:** $\text{attempts}_j = \sum_t \sum_{i \in N(j)} 0.25 \cdot P(X_{t-1} = i)$
- **Successes:** $\text{success}_j = \sum_t \sum_{i \in N(j)} P(X_{t-1} = i, X_t = j)$

Apply a **Beta prior** for regularization (strength = 2):

- $a = \text{strength} \times (1 - \text{occ\_prior})$
- $b = \text{strength} \times \text{occ\_prior}$

Posterior estimate:

$$p_{\text{open}_j} = \frac{\text{success}_j + a}{\text{attempts}_j + a + b}$$

$$\text{occ}_j = 1 - p_{\text{open}_j}$$

The occupancy estimate is used in two ways:

1. **Transition model:** Down-weights transitions into occupied cells
2. **State prior:** Acts as a prior on $X_t = j$ for open vs. blocked cells

---

## Level 5: Compilation and Execution Mechanics

### Prerequisites

Per `emdw_install.pdf` and `userguide.pdf`:

- Build emdw and dependencies (gLinear, prlite)
- Ensure `libemdw` is built
- Identify `EMDW_ROOT`:
  - If course provides `emdw_de424` at repo root: `EMDW_ROOT=/path/to/emdw_de424/devel/emdw`
  - Otherwise: use installed path (e.g., `~/devel/emdw`)

### Building the Executable

Build the wumpus executable inside emdw using the standard pattern:

**Step 1: Link sources into emdw/src/bin**

```bash
ln -s /Users/taros/Documents/DE424MP/src/wumpus.cc $EMDW_ROOT/src/bin/wumpus.cc
ln -s /Users/taros/Documents/DE424MP/src/wumpus_model.cc $EMDW_ROOT/src/bin/wumpus_model.cc
ln -s /Users/taros/Documents/DE424MP/include/wumpus.hpp $EMDW_ROOT/src/bin/wumpus.hpp
ln -s /Users/taros/Documents/DE424MP/include/wumpus_model.hpp $EMDW_ROOT/src/bin/wumpus_model.hpp
```

**Step 2: Update $EMDW_ROOT/src/bin/CMakeLists.txt**

Add the following lines:

```cmake
add_executable(wumpus wumpus.cc wumpus_model.cc)
target_link_libraries(wumpus emdw)
```

**Step 3: Build**

```bash
cd $EMDW_ROOT/build
cmake ../
make -j7 wumpus
```

### Running the Program

**Executable:** `wumpus` (built inside emdw)

**CLI:**

```
wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n] [--occ prior]
```

**Required arguments:**

- `dataset_dir`: path to dataset folder containing `data_file*.txt` grids
- `dataset_id`: 1–5 (selects known parameters or enables EM/occupancy mode)
- `out_prefix`: output file prefix

**Optional arguments:**

- `--pw v`: override detection probability for true cell (default from dataset_id)
- `--pc v`: override clutter probability (default from dataset_id)
- `--em n`: run EM for n iterations (enables parameter estimation for datasets 3, 5)
- `--occ prior`: override occupancy prior (default from dataset_id)

### Output Files

Program outputs two files per run:

- **`<out_prefix>_marginal.txt`:** Argmax of marginals $\arg\max_s p(X_t = s \mid Z_{1:T})$ at each time step
- **`<out_prefix>_map.txt`:** MAP path from Viterbi algorithm

**Format:** Each line is `x y` (x = column, y = row), zero-indexed, with origin at top-left, matching the ground-truth format in `mini_project.pdf`.

### Building Tests

Build tests using the same pattern inside emdw:

**Step 1: Link test sources and headers**

```bash
ln -s /Users/taros/Documents/DE424MP/tests_basic/test_transition.cc $EMDW_ROOT/src/bin/test_transition.cc
ln -s /Users/taros/Documents/DE424MP/tests_basic/test_emission.cc $EMDW_ROOT/src/bin/test_emission.cc
ln -s /Users/taros/Documents/DE424MP/tests_basic/test_parser.cc $EMDW_ROOT/src/bin/test_parser.cc
ln -s /Users/taros/Documents/DE424MP/tests_basic/test_emdw_bp.cc $EMDW_ROOT/src/bin/test_emdw_bp.cc
ln -s /Users/taros/Documents/DE424MP/tests_advanced/test_dataset2_scale.cc $EMDW_ROOT/src/bin/test_dataset2_scale.cc
ln -s /Users/taros/Documents/DE424MP/tests_advanced/test_em_extremes.cc $EMDW_ROOT/src/bin/test_em_extremes.cc
ln -s /Users/taros/Documents/DE424MP/tests_advanced/test_occ_prior_stress.cc $EMDW_ROOT/src/bin/test_occ_prior_stress.cc
ln -s /Users/taros/Documents/DE424MP/tests_advanced/test_e2e_sim.cc $EMDW_ROOT/src/bin/test_e2e_sim.cc
ln -s /Users/taros/Documents/DE424MP/src/wumpus_model.cc $EMDW_ROOT/src/bin/wumpus_model.cc
ln -s /Users/taros/Documents/DE424MP/include/wumpus_model.hpp $EMDW_ROOT/src/bin/wumpus_model.hpp
```

**Step 2: Add test targets to $EMDW_ROOT/src/bin/CMakeLists.txt**

```cmake
add_executable(test_transition test_transition.cc wumpus_model.cc)
add_executable(test_emission test_emission.cc wumpus_model.cc)
add_executable(test_parser test_parser.cc wumpus_model.cc)
add_executable(test_emdw_bp test_emdw_bp.cc)
add_executable(test_dataset2_scale test_dataset2_scale.cc wumpus_model.cc)
add_executable(test_em_extremes test_em_extremes.cc wumpus_model.cc)
add_executable(test_occ_prior_stress test_occ_prior_stress.cc wumpus_model.cc)
add_executable(test_e2e_sim test_e2e_sim.cc wumpus_model.cc)

target_link_libraries(test_transition emdw)
target_link_libraries(test_emission emdw)
target_link_libraries(test_parser emdw)
target_link_libraries(test_emdw_bp emdw)
target_link_libraries(test_dataset2_scale emdw)
target_link_libraries(test_em_extremes emdw)
target_link_libraries(test_occ_prior_stress emdw)
target_link_libraries(test_e2e_sim emdw)
```

**Step 3: Build and run tests**

```bash
cd $EMDW_ROOT/build
cmake ../
make -j7 test_transition test_emission test_parser test_emdw_bp \
         test_dataset2_scale test_em_extremes test_occ_prior_stress test_e2e_sim

src/bin/test_transition
src/bin/test_emission
src/bin/test_parser
src/bin/test_emdw_bp
src/bin/test_dataset2_scale /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2
src/bin/test_em_extremes
src/bin/test_occ_prior_stress
src/bin/test_e2e_sim
```

### Test Descriptions

**Basic Tests (unit-level constraints):**

- **test_transition.cc:** Verifies boundary and blocked-cell transition probabilities
- **test_emission.cc:** Validates correct $p_w$/$p_c$ likelihood computation for mock grid
- **test_parser.cc:** Tests grid file parsing and dataset loading
- **test_emdw_bp.cc:** Sanity check that BP runs without crashing

**Advanced Tests (stress and edge cases):**

- **test_dataset2_scale.cc:** Dataset2 timing and memory profiling
- **test_em_extremes.cc:** EM stability on all-1s and all-0s grids
- **test_occ_prior_stress.cc:** Heavy blocking with 25% prior occupancy
- **test_e2e_sim.cc:** End-to-end accuracy on small simulated path (≥80% required)
