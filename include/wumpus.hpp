#pragma once

#include <string>
#include <vector>

// core data structures for the wumpus tracking problem (de424 mini project)
//
// the wumpus does a random walk on a grid -- at each step it picks one of
// 4 directions uniformly and moves there, staying put if it hits a wall.
// each cell independently fires a binary detection: pw if the wumpus is
// there, pc (clutter) otherwise. goal: recover the wumpus position from
// the noisy observation grid at each timestep.

// 2d observation grid for one timestep.
// cells are stored row-major: cells[y*cols + x].
// e.g. in a 5x5 grid, (x=2, y=3) -> index 17
struct Grid {
  int rows = 0;
  int cols = 0;
  std::vector<int> cells;  // binary detections, flattened row-major
};

// per-dataset model parameters and learning settings.
// datasets 1 & 2 have known pw/pc; dataset 3 learns them via EM.
struct DatasetConfig {
  int id = 0;              // 1, 2, or 3
  double pw = 0.0;         // p(detection | wumpus at cell)
  double pc = 0.0;         // p(detection | wumpus not at cell) -- clutter rate
  bool learn_pwpc = false; // true -> run EM to estimate pw and pc
  int max_iters = 0;       // em iteration count (0 = skip learning)
};

// output from one full inference pass.
//
// marginal_path[t] = argmax_s P(X_t=s | Z_{1:T})  -- per-step bp estimate
// map_path[t]      = viterbi decode of the full joint trajectory
struct InferenceResult {
  std::vector<int> marginal_path;   // bp marginal argmax per timestep
  std::vector<int> map_path;        // viterbi (max-product) trajectory
  double pw = 0.0;                  // final pw (may have been learned)
  double pc = 0.0;                  // final pc (may have been learned)
};
