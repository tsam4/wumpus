# Mini Project Objectives

This document maps each requirement from `mini_project.pdf` to the deliverables in this repository.

---

## Objective 1: Robust Implementation

**Requirement:** Implement a wumpus tracker with HMM, BP inference, and Viterbi MAP, covering all model specifications from `mini_project.pdf`.

**Deliverables:**

| File | Role |
|------|------|
| `src/wumpus.cc` | Main tracker: dataset loading, EM loops, output writing |
| `src/wumpus_model.cc` | Shared utilities: transitions, emissions, Viterbi, grid I/O |
| `include/wumpus.hpp` | Core data structures (`Grid`, `DatasetConfig`, `InferenceResult`) |
| `include/wumpus_model.hpp` | Helper function declarations |

**Features:**
- Hidden Markov Model for state tracking (random walk with boundary failure)
- Sum-product BP inference via emdw for marginal trajectories
- Viterbi algorithm for MAP trajectories (max-product equivalent)
- EM algorithm for unknown detection parameters (pw, pc) — dataset 3
- Numerical stability: probability clamping, log-space emission computation

---

## Objective 2: Comprehensive Testing

**Requirement:** Two-tier test architecture (basic unit + advanced integration tests), grounded in `mini_project.pdf` specifications.

**Basic Tests (`tests_basic/`):**

| File | Tests |
|------|-------|
| `test_transition.cc` | Boundary conditions, uniform transitions on valid neighbors, accumulated self-transitions |
| `test_emission.cc` | pw/pc likelihood computation on mock 3×3 grid |
| `test_parser.cc` | Grid file parsing, dataset loading, field initialization |
| `test_emdw_bp.cc` | BP execution without crash, marginals sum to 1.0 |

**Advanced Tests (`tests_advanced/`):**

| File | Tests |
|------|-------|
| `test_dataset2_scale.cc` | Performance on 20×20 grid: BP time, Viterbi time, memory |
| `test_em_extremes.cc` | EM stability on degenerate grids (all-zero, all-one observations) |
| `test_e2e_sim.cc` | End-to-end accuracy ≥ 80% on simulated deterministic scenario |

---

## Objective 3: Complete Documentation

**Requirement:** README covering domain, theory, implementation, edge cases, and mechanics. Run & Test guide with execution, file roles, output interpretation, and validation.

**Deliverables:**

| File | Contents |
|------|----------|
| `README.md` | Domain definition, HMM theory, emdw integration, EM derivation, build & run reference |
| `RUN_AND_TESTS.md` | CLI reference for datasets 1–3, file roles, output format, test procedures, results |

---

## Objective 4: Full Execution and Validation

**Requirement:** Run all datasets with appropriate parameters, validate outputs against ground truth where available.

**Execution Summary:**

| Dataset | Grid | Steps | Command | Status |
|---------|------|-------|---------|--------|
| dataset1 | 5×5 | 10 | `wumpus dataset1 1 out_d1` | ✅ MAP + marginal match GT exactly |
| dataset2 | 20×20 | 20 | `wumpus dataset2 2 out_d2` | ✅ Output format valid (20 lines) |
| dataset3 | 10×20 | 20 | `wumpus dataset3 3 out_d3 --em 10` | ✅ EM converged, output valid |

**Generated Output Files:**

```
out_d1_map.txt        (10 lines) ← matches ground truth
out_d1_marginal.txt   (10 lines) ← matches ground truth
out_d2_map.txt        (20 lines)
out_d2_marginal.txt   (20 lines)
out_d3_map.txt        (20 lines) [EM]
out_d3_marginal.txt   (20 lines) [EM]
```

**Performance (dataset2, largest grid):**

```
BP inference time:  153 ms
Viterbi MAP time:   < 1 ms
Memory (maxrss):    16,056,320 bytes (~15.3 MB)
```
