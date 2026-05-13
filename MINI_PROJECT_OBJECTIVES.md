# Mini Project Objectives

## Overview

Build a probabilistic tracker for a Wumpus agent moving through a grid world, using noisy binary detections as observations. The solution uses a Hidden Markov Model (HMM) implemented as an emdw factor graph with loopy belief propagation (LBP) inference.

---

## Objective 1: Model the Transition Dynamics

**Requirement:** Implement a random-walk transition model per `mini_project.pdf`.

- **Grid:** Row-major indexing — cell $(x, y)$ at index $y \cdot C + x$
- **Move rule:** Wumpus picks direction uniformly from 4; fails (stays put) if out-of-bounds
- **Transition probabilities:**
  - $P(X_t = j \mid X_{t-1} = i) = 0.25$ for each valid neighbour $j$
  - $P(X_t = i \mid X_{t-1} = i) = $ (number of out-of-bounds directions from $i$) / 4
- **Implementation:** `build_transition_adj()` in `src/wumpus_model.cc`
- **Test:** `test_transition.cc` validates boundary conditions

---

## Objective 2: Model the Emission Distribution

**Requirement:** Implement the noisy binary detection model.

- **True cell** $s$: $P(Z_t^s = 1 \mid X_t = s) = p_w$
- **Clutter cells** $k \neq s$: $P(Z_t^k = 1 \mid X_t = s) = p_c$
- **Log-likelihood** over all cells: $\log p(Z_t \mid X_t = s) = \sum_k \log P(Z_t^k \mid X_t = s)$
- **Numerical stability:** clamping via `clamp_prob()`, log-sum-exp via `scale_from_log()`
- **Implementation:** `compute_emission_log()` in `src/wumpus_model.cc`
- **Test:** `test_emission.cc`

---

## Objective 3: Select or Learn Parameters

**Requirement:** Handle both known and unknown model parameters.

**Known parameters (Datasets 1, 2):**
- dataset1: $p_w = 0.95$, $p_c = 0.05$
- dataset2: $p_w = 0.95$, $p_c = 0.05$

**Unknown parameters (Dataset 3):**
- $p_w$ and $p_c$ unknown; learned via EM

**EM Algorithm (Dataset 3):**

E-step — run BP, extract posteriors:
$$\gamma_t(s) = P(X_t = s \mid Z_{1:T})$$

M-step — update parameters:
$$p_w = \frac{\sum_t \sum_s \gamma_t(s) \cdot Z_t^s}{\sum_t \sum_s \gamma_t(s)}$$

$$p_c = \frac{\sum_t \sum_s \gamma_t(s) \cdot \sum_{k \neq s} Z_t^k}{\sum_t \sum_s \gamma_t(s) \cdot (N - 1)}$$

- **Implementation:** EM loop in `src/wumpus.cc`
- **Test:** `test_em_extremes.cc`

---

## Objective 4: Assemble and Run the PGM

**Requirement:** Build an HMM factor graph using emdw and run LBP inference.

**PGM components:**
- RV $X_t$ for each timestep: `emdw::RVIdType`, domain $\{0, \ldots, N-1\}$
- Unary prior factor $p(X_0)$: uniform `DiscreteTable<int>`
- Unary emission factors $p(Z_t \mid X_t)$: per-timestep `DiscreteTable<int>`
- Pairwise transition factors $p(X_t \mid X_{t-1})$: `DiscreteTable<int>`
- Cluster graph: Bethe structure via `ClusterGraph`
- Inference: `loopyBP_CG` + `queryLBP_CG`

**Implementation:** `run_bp_inference()` in `src/wumpus_model.cc`

**Test:** `test_emdw_bp.cc`

---

## Objective 5: Compute Trajectories and Output Results

**Requirement:** Produce two trajectory files per dataset.

- **Marginal trajectory:** $\hat{x}_t = \arg\max_s P(X_t = s \mid Z_{1:T})$ — per-timestep argmax of BP marginals
- **MAP trajectory:** $\arg\max_{X_{1:T}} P(X_{1:T} \mid Z_{1:T})$ — Viterbi dynamic programming

**Viterbi recurrence:**
$$\delta_t(s) = \max_x \left[\delta_{t-1}(x) + \log P(X_t = s \mid X_{t-1} = x)\right] + \log p(Z_t \mid X_t = s)$$

**Implementation:** `viterbi_path()` in `src/wumpus_model.cc`

**Output format:** One `x y` pair per line.

**Dataset configurations:**

| Dataset | Grid  | Steps | pw   | pc   | EM iters |
|---------|-------|-------|------|------|----------|
| 1       | 5×5   | 10    | 0.95 | 0.05 | 0        |
| 2       | 20×20 | 20    | 0.95 | 0.05 | 0        |
| 3       | 10×20 | 20    | ?    | ?    | 10       |

---

## Implementation Status

| Step | Task                                       | Status      | Notes                                                                                  |
|------|--------------------------------------------|-------------|----------------------------------------------------------------------------------------|
| 1    | Design & implement wumpus world model      | ✅ COMPLETE  | HMM with transition/emission models, RV definitions, PGM structure, factors all implemented |
| 2    | Implement inference algorithm (BP)         | ✅ COMPLETE  | LBP via emdw ClusterGraph; Viterbi for MAP                                            |
| 3    | Select or learn parameters                 | ✅ COMPLETE  | Known parameters hardcoded; unknown $p_w$/$p_c$ learned via EM                        |
| 4    | Run on datasets and produce outputs        | ✅ COMPLETE  | Datasets 1–3 produce valid output files                                               |
| 5    | Validate with tests and accuracy check     | ✅ COMPLETE  | All unit and integration tests pass; dataset1 accuracy verified                        |

---

## Key Source Files

- `src/wumpus.cc` — main entry point, argument parsing, EM loop, output writing
- `src/wumpus_model.cc` — model: transitions, emissions, BP wrapper, Viterbi
- `include/wumpus.hpp` — data structures: `Grid`, `DatasetConfig`, `InferenceResult`
- `include/wumpus_model.hpp` — function declarations
- `tests_basic/test_transition.cc` — transition boundary validation
- `tests_basic/test_emdw_bp.cc` — factor graph and BP validation
- `tests_advanced/test_e2e_sim.cc` — end-to-end accuracy validation
- `tests_advanced/test_dataset2_scale.cc` — performance validation
