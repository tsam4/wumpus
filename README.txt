Level 1: The Domain Definition
This project tracks a latent Wumpus in a grid world using noisy, cluttered binary detections (mini_project.pdf). At each time step, every grid cell emits a 0/1 detection. The Wumpus moves one cell per step (up/down/left/right) but can fail to move if the target is outside the grid or blocked (datasets 4 and 5). The task is to infer the Wumpus position at each time step and output a trajectory of x y coordinates that matches the ground-truth file format described in the project brief.

Core data facts from mini_project.pdf:
- dataset1: 5x5 grid, 10 steps, pw=0.95, pc=0.05
- dataset2: 20x20 grid, 20 steps, pw=0.90, pc=0.10
- dataset3: 10x20 grid, 20 steps, pw/pc unknown
- dataset4: 4x4 grid, 100 steps, pw=0.95, pc=0.05, unknown occupied cells, prior 25% occupied
- dataset5: 7x7 grid, 200 steps, pw/pc unknown, unknown occupied cells, prior 10% occupied

Coordinate convention (mini_project.pdf, Figure 1): output lines are "x y" for each time step. In code, we map (x,y) to index s = y*cols + x, where y is the row index in the text file (top line is y=0), and x is the column index (leftmost is x=0).


Level 2: The PGM Theoretical Blueprint
Hidden state: X_t in {0..(M*N-1)}, where each value is a grid cell index s = y*cols + x. Observations: Z_t is the full grid of binary detections z_k for all cells k.

Transition model (random walk with failure, mini_project.pdf):
Let N(i) be the 4-neighbors of cell i that are inside the grid. For datasets with occupancy, each neighbor j has an occupancy probability occ_j (unknown, to be inferred). The Wumpus picks a direction uniformly from 4, then:
- If the direction is out of bounds, it stays put.
- If the direction points to an occupied cell, it stays put.
Thus for each neighbor j in N(i):
  P(X_t=j | X_{t-1}=i) = 0.25 * (1 - occ_j)
And the self-transition accumulates all failed moves:
  P(X_t=i | X_{t-1}=i) = 0.25 * (# out-of-bounds directions)
                        + sum_{j in N(i)} 0.25 * occ_j
For datasets without occupancy, occ_j = 0 and the transition reduces to a uniform 0.25 over valid neighbors plus accumulated stay on borders.

Emission model (mini_project.pdf):
The detection grid is conditionally independent given X_t. If X_t=c:
- The true cell emits z_c=1 with pw (miss with 1-pw).
- Every other cell k!=c emits a clutter detection z_k=1 with pc (true negative 1-pc).
Thus the likelihood of the whole grid is:
  P(Z_t | X_t=c) = [ pw if z_c=1 else (1-pw) ]
                   * product_{k!=c} [ pc if z_k=1 else (1-pc) ]
We fold this into a unary factor over X_t by evaluating the product for each state c.


Level 3: The emdw Framework Integration
Factors and RVs are implemented using emdw::DiscreteTable<int> (userguide.pdf). For each time step t:
- Unary prior factor p(X_0) over RV X_0 (uniform, or weighted by open-cell prior when occupancy is modeled).
- Unary emission factor psi_t(X_t) computed from the observed grid Z_t using the likelihood above.
- Binary transition factor phi_t(X_t, X_{t-1}) with sparse entries from the random-walk model.

Cluster graph construction (userguide.pdf):
- We build a Bethe cluster graph: ClusterGraph::BETHE over all factors.
- We call loopyBP_CG for sum-product inference.
- We extract marginals with queryLBP_CG and normalize to get p(X_t | Z_1:T).

MAP trajectory:
- emdw provides loopyBP_CG (sum-product) and loopyBU_CG (update), but does not expose a max-product API in the provided headers. For MAP, we therefore run Viterbi using the same transition probabilities and emission log-likelihoods computed for the factors. This is the max-product equivalent for an HMM chain and uses exactly the model defined above.


Level 4: Edge Case Logic & Parameter Estimation
Datasets 3 and 5 (unknown pw, pc):
We use EM with sum-product inference (BP) for the E-step.
- E-step: compute gamma_t(s) = P(X_t=s | Z_1:T) from BP marginals.
- M-step: update pw and pc using expected counts from mini_project.pdf’s emission model.
Let z_t(k) in {0,1} be the observation at cell k, and N = rows*cols.
  expected true detections = sum_t sum_s gamma_t(s) * z_t(s)
  expected clutter detections = sum_t [ sum_k z_t(k) - sum_s gamma_t(s) * z_t(s) ]
Then:
  pw = (1/T) * expected true detections
  pc = (1/(T*(N-1))) * expected clutter detections
We clamp pw and pc to (1e-6, 1-1e-6) to avoid log(0) in the emission factors.

Datasets 4 and 5 (unknown occupied cells):
Each cell j has an occupancy prior from mini_project.pdf:
- dataset4: P(occ_j=1) = 0.25
- dataset5: P(occ_j=1) = 0.10
We estimate a soft occupancy probability per cell using pairwise posteriors.
Define attempts_j as the expected number of attempts to move into j:
  attempts_j = sum_t sum_{i in N(j)} 0.25 * P(X_{t-1}=i)
Define success_j as the expected number of successful moves into j:
  success_j = sum_t sum_{i in N(j)} P(X_{t-1}=i, X_t=j)
We apply a Beta prior (mean = 1 - occ_prior) with small strength to stabilize:
  p_open_j = (success_j + a) / (attempts_j + a + b)
  occ_j = 1 - p_open_j
where a = strength * (1 - occ_prior), b = strength * occ_prior, strength=2.
The occupancy estimate directly down-weights transitions into j and also acts as a prior on X_t=j.


Level 5: Compilation and Execution Mechanics
Pre-reqs (per emdw_install.pdf and userguide.pdf):
- Build emdw and dependencies (gLinear, prlite). Ensure libemdw is built.
- Identify EMDW_ROOT. If the course bundle provides emdw_de424 at repo root, use:
    EMDW_ROOT=/path/to/emdw_de424/devel/emdw
  Otherwise use the installed path (e.g., ~/devel/emdw).

Build the wumpus executable inside emdw (simple path used in userguide.pdf):
1) Link sources into emdw/src/bin (symlink or copy)
   ln -s /Users/taros/Documents/DE424MP/src/wumpus.cc $EMDW_ROOT/src/bin/wumpus.cc
   ln -s /Users/taros/Documents/DE424MP/src/wumpus_model.cc $EMDW_ROOT/src/bin/wumpus_model.cc
   ln -s /Users/taros/Documents/DE424MP/include/wumpus.hpp $EMDW_ROOT/src/bin/wumpus.hpp
   ln -s /Users/taros/Documents/DE424MP/include/wumpus_model.hpp $EMDW_ROOT/src/bin/wumpus_model.hpp
2) Edit $EMDW_ROOT/src/bin/CMakeLists.txt and add:
   add_executable(wumpus wumpus.cc wumpus_model.cc)
   target_link_libraries(wumpus emdw)
3) Build:
   cd $EMDW_ROOT/build
   cmake ../; make -j7 wumpus

Run tracking:
- dataset1: src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset1 1 out_d1
- dataset2: src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2 2 out_d2
- dataset3: src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset3 3 out_d3 --em 6
- dataset4: src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset4 4 out_d4 --em 6
- dataset5: src/bin/wumpus /Users/taros/Documents/DE424MP/Datasets-20260506/dataset5 5 out_d5 --em 8

Outputs:
- out_*_marginal.txt: argmax of marginals per time step (sum-product)
- out_*_map.txt: MAP trajectory via Viterbi (max-product equivalent)
Each line is "x y" as required in mini_project.pdf.

Testing (two tiers, per mini_project.pdf constraints):
- tests_basic/ contains minimal unit tests:
  - test_transition.cc: boundary and blocked-cell transition probabilities
  - test_emission.cc: correct pw/pc likelihood for a mock grid
  - test_parser.cc: grid file parsing and dataset load
  - test_emdw_bp.cc: sanity check that BP runs
- tests_advanced/ contains stress tests:
  - test_dataset2_scale.cc: dataset2 timing + memory printout
  - test_em_extremes.cc: EM stability on all-1s and all-0s grids
  - test_occ_prior_stress.cc: heavy blocking with 25% prior
  - test_e2e_sim.cc: end-to-end accuracy on a small simulated path

To build tests inside emdw (same pattern as wumpus):
1) Link test sources and headers into emdw/src/bin:
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
2) Add test targets to $EMDW_ROOT/src/bin/CMakeLists.txt:
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
3) Build and run tests:
   cd $EMDW_ROOT/build
   cmake ../; make -j7 test_transition test_emission test_parser test_emdw_bp \
                    test_dataset2_scale test_em_extremes test_occ_prior_stress test_e2e_sim
   src/bin/test_transition
   src/bin/test_emission
   src/bin/test_parser
   src/bin/test_emdw_bp
   src/bin/test_dataset2_scale /Users/taros/Documents/DE424MP/Datasets-20260506/dataset2
   src/bin/test_em_extremes
   src/bin/test_occ_prior_stress
   src/bin/test_e2e_sim
