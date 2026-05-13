Wumpus Tracker - DE424 Mini Project

This project tracks a latent Wumpus in a grid world using noisy, cluttered binary detections (mini_project.pdf). At each time step, every grid cell emits a 0/1 detection. The Wumpus moves one cell per step (up/down/left/right) but fails to move if the target is outside the grid. The task is to infer the Wumpus position at each time step and output a trajectory of x y coordinates that matches the ground-truth file format described in the project brief.

Datasets:
- dataset1: 5x5 grid, 10 steps, pw=0.95, pc=0.05
- dataset2: 20x20 grid, 20 steps, pw=0.95, pc=0.05
- dataset3: 10x20 grid, 20 steps, pw/pc unknown (learned via EM)

TRANSITION MODEL
The Wumpus picks a direction uniformly from 4 (up, down, left, right):
- If the direction is out-of-bounds, the Wumpus stays put.
  P(X_t=j | X_{t-1}=i) = 0.25  for each valid (in-bounds) neighbour j
  P(X_t=i | X_{t-1}=i) += 0.25 for each out-of-bounds direction from i

EMISSION MODEL
Each cell k independently emits a binary detection:
  P(z_k=1 | X_t=s) = pw  if k == s (true cell)
  P(z_k=1 | X_t=s) = pc  if k != s (clutter)

INFERENCE
The HMM is assembled as an emdw factor graph:
- Unary prior factor p(X_0) over RV X_0 (uniform).
- Unary emission factors p(Z_t | X_t) at each timestep.
- Pairwise transition factors p(X_t | X_{t-1}) linking adjacent states.

Inference via loopy belief propagation on the Bethe cluster graph.

MAP trajectory computed via Viterbi dynamic programming.

EM PARAMETER LEARNING (Dataset 3)
E-step: run BP to compute posteriors gamma[t][s] = P(X_t=s | Z_1:T)
M-step: update pw and pc from expected sufficient statistics:
  pw = sum_t sum_s gamma[t][s] * Z_t^s / sum_t sum_s gamma[t][s]
  pc = sum_t sum_s gamma[t][s] * sum_{k!=s} Z_t^k /
       ( sum_t sum_s gamma[t][s] * (N-1) )

BUILDING
From emdw/build/:
  cmake ../; make -j7 wumpus

RUNNING
  src/bin/wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]

Examples:
- dataset1: src/bin/wumpus /path/to/dataset1 1 out_d1
- dataset2: src/bin/wumpus /path/to/dataset2 2 out_d2
- dataset3: src/bin/wumpus /path/to/dataset3 3 out_d3 --em 10

OUTPUT FILES
Each run produces:
  out_dN_marginal.txt  (T lines: x y per timestep, argmax marginals)
  out_dN_map.txt       (T lines: x y per timestep, Viterbi MAP path)

TESTS
Basic:
  test_parser.cc:     grid file parsing
  test_emission.cc:   emission model correctness
  test_transition.cc: boundary transition probabilities
  test_emdw_bp.cc:    BP factor construction and convergence

Advanced:
  test_e2e_sim.cc:        end-to-end accuracy on simulated data
  test_em_extremes.cc:    EM stability on degenerate observations
  test_dataset2_scale.cc: performance/memory on 20x20 grid (dataset2)

MAKEFILE TARGETS
  make build         Build the executable
  make test          Run basic unit tests
  make test-advanced Run advanced tests
  make run           Run all datasets (1-3)
  make run-d1        Run dataset1
  make run-d2        Run dataset2
  make run-d3        Run dataset3
  make accuracy      Compare dataset1 output to ground truth
  make clean         Remove generated files
