#pragma once

#include <string>
#include <vector>

// ============================================================================
// Wumpus Tracking Data Structures
// ============================================================================
//
// This header defines the core data structures for the wumpus tracking
// problem from DE424 Mini Project. The wumpus is a latent agent that moves
// randomly on a grid, and we observe noisy binary detections at each cell
// to infer its position over time.
//
// Key concepts from mini_project.pdf:
// - The wumpus performs a random walk: at each step, it chooses a direction
//   uniformly from 4 (up, down, left, right) and attempts to move.
// - If the target cell is outside grid bounds or occupied (blocked), the
//   wumpus stays in its current cell.
// - Observations: each cell independently emits a binary detection
//   (1 or 0) with probabilities pw (true cell) and pc (clutter).
// - Goal: infer the wumpus position at each timestep from these detections.
//
// ============================================================================

// Grid: represents the observation at one timestep
//
// Stores a 2D grid of binary detections (0 or 1) where cells[i] indicates
// whether cell i generated a detection at this timestep.
// Cells are indexed as s = y*cols + x (row-major order).
//
// Example: in a 5x5 grid, cell (x=2, y=3) is at index 3*5 + 2 = 17.
struct Grid {
  int rows = 0;      // number of rows
  int cols = 0;      // number of columns
  std::vector<int> cells;  // flattened row-major grid: cells[y*cols+x]
};

// DatasetConfig: parameters and settings for a dataset
//
// Controls model parameters (pw, pc) and learning strategies.
// Each dataset (1-3) in mini_project.pdf has specific settings:
//
// - Datasets 1, 2: known pw/pc (constants)
// - Dataset 3: unknown pw/pc (learned via EM, learn_pwpc=true)
//
struct DatasetConfig {
  int id = 0;              // dataset ID (1-3), determines defaults
  double pw = 0.0;         // P(detection | wumpus at cell): detection prob
  double pc = 0.0;         // P(detection | wumpus not at cell): clutter prob
  bool learn_pwpc = false; // if true, use EM to learn pw/pc
  int max_iters = 0;       // number of EM iterations (0 = no learning)
};

// InferenceResult: output of the inference pipeline
//
// Stores both marginal and MAP trajectories, as well as learned parameters:
//
// - marginal_path: argmax of posteriors at each timestep (single-step optimal)
//   Computed as: path[t] = argmax_s P(X_t=s | Z_1:T) from BP
//
// - map_path: most likely complete trajectory (joint optimal)
//   Computed via Viterbi: argmax_{X_1:T} P(X_1:T | Z_1:T)
//
// - pw, pc: final detection parameters (may be learned or fixed)
//
struct InferenceResult {
  std::vector<int> marginal_path;   // argmax marginals per timestep
  std::vector<int> map_path;        // max-product (Viterbi) trajectory
  double pw = 0.0;                  // final or learned detection prob
  double pc = 0.0;                  // final or learned clutter prob
};
