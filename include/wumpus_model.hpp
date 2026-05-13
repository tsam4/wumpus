#pragma once

#include "wumpus.hpp"
#include "emdw.hpp"

#include <string>
#include <utility>
#include <vector>

// ============================================================================
// Helper Function Declarations for Wumpus Model
// ============================================================================
//
// These functions implement the core model components:
// 1. Numerical stability (clamping probabilities to safe range)
// 2. Coordinate conversion (linear index <-> grid coordinates)
// 3. Grid file I/O and dataset loading
// 4. Transition model construction (random walk with boundary failure)
// 5. Emission model computation (noisy detections)
// 6. Belief Propagation inference (marginal posteriors)
//
// ============================================================================

// Coordinate Conversion
// =====================
// Convert between linear cell indices (0..rows*cols-1) and grid coordinates (x, y).
// Grid uses row-major order: index = y * cols + x
// where (x, y) represents column x and row y, both 0-indexed.

// coord_x: convert cell index to column coordinate
// Example: 5x5 grid, index=17 -> x=2 (since 17 = 3*5 + 2)
inline int coord_x(int index, int cols) { return index % cols; }

// coord_y: convert cell index to row coordinate
// Example: 5x5 grid, index=17 -> y=3
inline int coord_y(int index, int cols) { return index / cols; }

// coord_to_index: convert grid coordinates to cell index
// Example: 5x5 grid, x=2, y=3 -> index=17
inline int coord_to_index(int x, int y, int cols) { return y * cols + x; }

// Numerical Stability
// ===================
// Clamps probabilities to (1e-6, 1-1e-6) to avoid log(0) when computing
// emission factors in log-space. This is critical for numerical stability
// in the EM algorithm and BP inference.
//
// Usage: safely compute log(p) even for edge-case probabilities
double clamp_prob(double p);

// File I/O Helpers
// ================

// read_grid_file: parse a single data_file*.txt into a Grid
// Returns false if file cannot be opened or parsed
// Validates that all rows have the same number of columns
bool read_grid_file(const std::string& path, Grid* grid);

// list_data_files: find all data_file*.txt files in a directory
// Returns sorted list of full file paths
std::vector<std::string> list_data_files(const std::string& dataset_dir);

// load_dataset: load all observation grids from a dataset directory
// Throws exception if file parsing fails or grid sizes are inconsistent
// Returns vector of Grid objects, one per timestep, in sorted order
std::vector<Grid> load_dataset(const std::string& dataset_dir);

// Transition Model Construction
// ==============================

// build_neighbors: compute 4-neighbors (up/down/left/right) for each cell
// Returns adjacency list nbrs[i] = {j1, j2, ...} for cell i
// Used to define which moves are valid from each state
std::vector<std::vector<int>> build_neighbors(int rows, int cols);

// build_transition_adj: construct transition probabilities
//
// Implements random walk with failure (per mini_project.pdf):
// - Wumpus chooses direction uniformly from 4
// - If target is out-of-bounds, wumpus stays put
//
// Transition model:
//   P(X_t=j | X_{t-1}=i) = 0.25 for j in valid neighbors of i
//   P(X_t=i | X_{t-1}=i) = sum of failed attempts (out-of-bounds moves)
//
// Returns sparse adjacency list adj[i] = {(j, p), ...}
//   where p = P(X_t=j | X_{t-1}=i)
std::vector<std::vector<std::pair<int, double>>> build_transition_adj(
    int rows,
    int cols);

// Emission Model Computation
// ==========================

// compute_emission_log: compute log-likelihood of observations given state
//
// Implements emission model (per mini_project.pdf):
// Given observations Z_t (grid of detections) and proposed state X_t=s:
//   log p(Z_t | X_t=s) = sum_k log P(z_k | X_t, k)
//
// Where for each cell k:
//   if k == s (true cell):
//     P(z_k=1 | X_t=s) = pw   (detection probability)
//     P(z_k=0 | X_t=s) = 1-pw (miss probability)
//   else (clutter):
//     P(z_k=1 | X_t=s) = pc   (clutter detection probability)
//     P(z_k=0 | X_t=s) = 1-pc (true negative probability)
//
// Returns vector logp[s] = log p(Z_t | X_t=s) for all states s
std::vector<double> compute_emission_log(
    const Grid& obs,
    double pw,
    double pc);

// scale_from_log: numerically stable conversion from log-space to probs
//
// To avoid underflow, when we have log-probabilities logp[i], we compute:
//   probs[i] = exp(logp[i] - max_log)
//
// This subtracts the maximum log value before exponentiating, keeping
// exponent values in a reasonable range. After normalization, relative
// probabilities are preserved.
//
// Critical for BP factor construction: converts log-likelihoods into
// probability values that can be used in DiscreteTable factors
std::vector<double> scale_from_log(const std::vector<double>& logp);

// Belief Propagation Inference
// ============================

// BPInferenceResult: holds outputs from a single BP inference pass
struct BPInferenceResult {
  std::vector<std::vector<double>> gamma;  // gamma[t][s] = P(X_t=s | Z_1:T)
};

// run_bp_inference: execute full BP pipeline and extract marginal posteriors
//
// High-level wrapper around the BP inference loop:
// 1. Constructs factors (prior, emission, transition)
// 2. Builds Bethe cluster graph
// 3. Runs loopy belief propagation (sum-product)
// 4. Extracts marginal posteriors gamma[t][s] = P(X_t=s | Z_1:T)
//
// Inputs:
//   obs:       observation grids for each timestep
//   x_ids:     RV identifiers (one per timestep)
//   state_dom: domain of state values [0, n-1]
//   pw, pc:    detection and clutter probabilities
//
// Returns: BPInferenceResult with posterior marginals
BPInferenceResult run_bp_inference(
    const std::vector<Grid>& obs,
    const std::vector<emdw::RVIdType>& x_ids,
    const rcptr<std::vector<int>>& state_dom,
    double pw,
    double pc);
