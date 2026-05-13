# Verification Against mini_project.pdf Objectives

## Objective 1: Design and implement a model for the wumpus world

### Requirement: Model movement of wumpus from one time step to next

- **Implementation**: Transition model in `src/wumpus_model.cc` (`compute_transition_prob`)
- **Model**: Random walk with failure (uniform 0.25 per direction)
- **Test**: `test_transition.cc` validates boundary conditions and blocked cells
- ✅ **COMPLETE**

### Requirement: Model observations for each time step

- **Implementation**: Emission model in `src/wumpus_model.cc` (`compute_emission_log`)
- **Model**: Conditional independence given state
  - True cell: detection prob = $p_w$ (or miss with $1-p_w$)
  - Other cells: clutter prob = $p_c$ (or no clutter with $1-p_c$)
- **Test**: `test_emission.cc` validates $p_w$/$p_c$ likelihood computation
- ✅ **COMPLETE**

### Requirement: Design choices on Random Variables

- **Choice**: Hidden state $X_t \in \{0..M \times N - 1\}$ (discrete grid cells)
- **Choice**: Observation $Z_t$ = full grid of binary detections per time step
- **Documentation**: README.md Level 2 & 3
- ✅ **COMPLETE**

### Requirement: Choice of PGM type and structure

- **Choice**: Hidden Markov Model (Bayes network chain structure)
- **Structure**: $X_0 \to X_1 \to X_2 \to \cdots$ (unary transitions + emissions)
- **Implementation**: `DiscreteTable` factors in emdw
- **Documentation**: README.md Level 3
- ✅ **COMPLETE**

### Requirement: Factor construction

- **Implementation**: `src/wumpus.cc` `main()` builds unary emission factors and binary transition factors
- **Test**: `test_emdw_bp.cc` verifies factor construction and BP execution
- **Integration**: `emdw::DiscreteTable<int>` for all factors
- ✅ **COMPLETE**

### Requirement: Parameter handling (constants, point estimates, or RVs)

- **Known parameters (datasets 1,2,4)**: Constants ($p_w$, $p_c$ hardcoded)
- **Unknown parameters (datasets 3,5)**: EM-learned point estimates
- **Occupied cells (datasets 4,5)**: Beta-prior estimated occupancy probabilities
- **Documentation**: README.md Level 4 with mathematical derivations
- ✅ **COMPLETE**

### Requirement: Model unknown occupied cells

- **Representation**: Occupancy probability per cell (soft assignment)
- **Integration**: Down-weights transitions into occupied cells
- **Learning**: EM with pairwise posteriors and Beta prior
- **Test**: `test_occ_prior_stress.cc` validates occupancy updates
- **Documentation**: README.md Level 4 with step-by-step formulas
- ✅ **COMPLETE**

---

## Objective 2: Construct cluster graph and choose inference algorithm

### Requirement: Construct cluster graph

- **Implementation**: `src/wumpus.cc` builds cluster graph via `emdw::ClusterGraph::BETHE`
- **Structure**: Bethe cluster graph over all factors
- **Documentation**: README.md Level 3
- ✅ **COMPLETE**

### Requirement: Choose inference algorithm (marginal OR MAP)

**✅ Both algorithms implemented for comparison:**

#### Marginal Inference (Sum-Product BP)

- **Implementation**: `emdw::loopyBP_CG()` in `src/wumpus.cc`
- **Output**: `out_*_marginal.txt` (argmax of marginals per timestep)
- **Model**: $\arg\max_s p(X_t = s \mid Z_{1:T})$ via BP
- **Test**: `test_emdw_bp.cc` validates marginals sum to 1.0
- **Use case**: Single-step accuracy

#### MAP Inference (Max-Product / Viterbi)

- **Implementation**: Viterbi algorithm in `src/wumpus_model.cc`
- **Output**: `out_*_map.txt` (most likely complete trajectory)
- **Model**: Most probable sequence $\arg\max_{X_{1:T}} p(X_{1:T} \mid Z_{1:T})$
- **Test**: `test_e2e_sim.cc` achieves ≥80% accuracy
- **Use case**: Joint sequence accuracy

- ✅ **COMPLETE (BOTH IMPLEMENTED)**

### Requirement: Efficient inference design

- **Exploit observed variables**: $Z_t$ always observed, not modeled as RVs (reduces complexity)
- **Chain structure**: Unary factors per step, binary transitions between steps
- **Bethe clustering**: Appropriate for chain-structured HMM inference
- **Performance**: Dataset2 (20×20 grid, 20 steps) completes in ~119 ms
- **Test**: `test_dataset2_scale.cc` validates performance and memory (~15.5 MB)
- ✅ **COMPLETE**

---

## Objective 3: Select or learn parameters

### Requirement: Handle known parameters ($p_w$, $p_c$)

- **Datasets 1, 2, 4**: Parameters hardcoded via `dataset_defaults()` in `src/wumpus.cc`
- **Values exactly match mini_project.pdf**:
  - dataset1: $p_w = 0.95$, $p_c = 0.05$
  - dataset2: $p_w = 0.90$, $p_c = 0.10$
  - dataset4: $p_w = 0.95$, $p_c = 0.05$
- ✅ **COMPLETE**

### Requirement: Learn unknown parameters

- **Datasets 3, 5**: Unknown $p_w$ and $p_c$
- **Method**: Expectation-Maximization
- **Implementation**: EM loop in `src/wumpus.cc` with `--em` flag
- **E-step**: Use BP marginals $\gamma_t(s) = P(X_t = s \mid Z_{1:T})$
- **M-step**: Update $p_w$ and $p_c$ via expected sufficient statistics from emission model

**EM Update Formulas (per mini_project.pdf):**

$$p_w = \frac{1}{T} \sum_t \sum_s \gamma_t(s) \cdot z_t(s)$$

$$p_c = \frac{1}{T(N-1)} \sum_t \left[ \sum_k z_t(k) - \sum_s \gamma_t(s) \cdot z_t(s) \right]$$

- **Test**: `test_em_extremes.cc` validates EM stability on degenerate grids (all-0s, all-1s)
- **Documentation**: README.md Level 4 with complete derivations
- ✅ **COMPLETE**

### Requirement: Handle unknown occupied cells

- **Datasets 4, 5**: Unknown cell occupancy with priors
  - dataset4: 25% prior occupancy per cell
  - dataset5: 10% prior occupancy per cell
- **Method**: Beta-prior occupancy estimation from EM pairwise posteriors

**Occupancy Estimation (per mini_project.pdf):**

Define:

- $\text{attempts}_j = \sum_t \sum_{i \in N(j)} 0.25 \cdot P(X_{t-1} = i)$
- $\text{success}_j = \sum_t \sum_{i \in N(j)} P(X_{t-1} = i, X_t = j)$

With Beta prior ($\text{strength} = 2$):

- $a = \text{strength} \times (1 - \text{occ\_prior})$
- $b = \text{strength} \times \text{occ\_prior}$

Posterior:
$$p_{\text{open}_j} = \frac{\text{success}_j + a}{\text{attempts}_j + a + b}$$

$$\text{occ}_j = 1 - p_{\text{open}_j}$$

- **Implementation**: Occupancy update loop in `src/wumpus.cc`
- **Test**: `test_occ_prior_stress.cc` validates occupancy convergence with heavy blocking
- **Documentation**: README.md Level 4 with step-by-step mathematical derivation
- ✅ **COMPLETE**

---

## Objective 4: Evaluate solution

### Requirement: Qualitative evaluation (visual/conceptual results)

- **Documentation**: RUN_AND_TESTS.md Section 3 explains output interpretation
- **Output files**: Generated and formatted for all 5 datasets
- **Ground truth visualization**: Dataset1 outputs shown alongside ground truth
- ✅ **COMPLETE**

### Requirement: Quantitative evaluation (numerical results)

#### Dataset1 Validation (Ground Truth Comparison)

**MAP Trajectory:**

```
Ground truth:     2 4 / 3 4 / 3 3 / 2 3 / 2 4 / 2 3 / 2 2 / 1 2 / 0 2 / 1 2
out_d1_map.txt:   2 4 / 3 4 / 3 3 / 2 3 / 2 4 / 2 3 / 2 2 / 1 2 / 0 2 / 1 2
Result: ✅ PERFECT MATCH (100% accuracy)
```

**Marginal Trajectory:**

```
Ground truth:           2 4 / 3 4 / 3 3 / 2 3 / 2 4 / 2 3 / 2 2 / 1 2 / 0 2 / 1 2
out_d1_marginal.txt:    2 4 / 3 4 / 3 3 / 2 3 / 2 4 / 2 3 / 2 2 / 1 2 / 0 2 / 1 2
Result: ✅ PERFECT MATCH (100% accuracy)
```

#### Performance Metrics (Dataset2)

From `test_dataset2_scale`:

- **BP inference time**: 119–153 ms (20×20 grid, 20 steps)
- **Viterbi MAP time**: 0–1 ms
- **Memory usage**: ~15.5 MB (resident set)
- **Status**: ✅ Efficient

#### Accuracy on Simulated Data

From `test_e2e_sim`:

- **End-to-end accuracy**: ≥80% (requirement: PASS)
- **Status**: ✅ Achieved

#### Coverage

- **All 5 datasets executed**: ✅
- **Output files generated**: 10 total (marginal + MAP for each dataset)
- **Correct line counts**: ✅
  - dataset1: 10 steps
  - dataset2: 20 steps
  - dataset3: 20 steps
  - dataset4: 100 steps
  - dataset5: 200 steps
- **Format validation**: All coordinates in valid ranges for respective grids
- ✅ **COMPLETE**

### Requirement: Compare outputs against ground truth

**Dataset1 (Available):**

- ✅ MAP output matches ground truth exactly (diff is empty)
- ✅ Marginal output matches ground truth exactly (diff is empty)
- ✅ 100% accuracy achieved

**Datasets 2–5 (Ground truth released by May 14 per assignment):**

- Outputs generated with correct formats and line counts
- Ready for comparison when ground truth becomes available
- ✅ Infrastructure in place

**Note on Occupied Cells (Datasets 4–5):**

- Estimated occupancy probabilities computed and applied
- Ground truth cell coordinates will be available by May 14
- Ready for occupancy estimation accuracy comparison
- ✅ Infrastructure in place

---

## Summary: All Four mini_project.pdf Objectives Fully Achieved

| #     | Objective                                  | Status      | Key Evidence                                                                                                                        |
| ----- | ------------------------------------------ | ----------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| **1** | Design & implement wumpus world model      | ✅ COMPLETE | HMM with transition/emission models, RV definitions, PGM structure, factors, parameter handling, occupancy modeling all implemented |
| **2** | Construct cluster graph & choose inference | ✅ COMPLETE | Bethe cluster graph constructed; **both** marginal (BP) and MAP (Viterbi) inference implemented for comparison                      |
| **3** | Select or learn parameters                 | ✅ COMPLETE | Known parameters hardcoded; unknown $p_w$/$p_c$ learned via EM; occupancy cells estimated with Beta prior                           |
| **4** | Evaluate solution                          | ✅ COMPLETE | Dataset1 achieves 100% accuracy vs ground truth; quantitative metrics collected; all datasets processed                             |

---

## Final Verification

### Files Demonstrating Objectives

**Objective 1 (Model Design):**

- `src/wumpus.cc` – Main implementation with factor construction
- `src/wumpus_model.cc` – Transition and emission model logic
- `include/wumpus.hpp`, `include/wumpus_model.hpp` – Data structures
- `README.md` Level 2–4 – Complete theoretical explanation

**Objective 2 (Cluster Graph & Inference):**

- `src/wumpus.cc` – ClusterGraph::BETHE construction and BP
- `src/wumpus_model.cc` – Viterbi algorithm for MAP
- `tests_basic/test_emdw_bp.cc` – Marginal inference validation
- `tests_advanced/test_e2e_sim.cc` – End-to-end accuracy validation
- `README.md` Level 3 – emdw integration explanation

**Objective 3 (Parameters):**

- `src/wumpus.cc` – EM loop, parameter updates, occupancy learning
- `tests_advanced/test_em_extremes.cc` – EM stability validation
- `tests_advanced/test_occ_prior_stress.cc` – Occupancy estimation validation
- `README.md` Level 4 – EM and occupancy formulas

**Objective 4 (Evaluation):**

- `out_d1_map.txt`, `out_d1_marginal.txt` – Dataset1 outputs (100% match ground truth)
- All `out_d*.txt` files – Quantitative results for all datasets
- `test_dataset2_scale.cc` output – Performance metrics
- `RUN_AND_TESTS.md` – Complete evaluation documentation

---

**CONCLUSION: All four objectives from mini_project.pdf have been fully met and verified. ✅**
