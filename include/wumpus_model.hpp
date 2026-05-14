#pragma once

#include "wumpus.hpp"
#include "emdw.hpp"

#include <string>
#include <utility>
#include <vector>

// declarations for all model-building and inference helpers.
// roughly in pipeline order:
//   coord utils -> file i/o -> transition model -> emission model -> bp

// ---------------------------------------------------------------------------
// coordinate helpers  (row-major: index = y*cols + x)
// ---------------------------------------------------------------------------

inline int coord_x(int index, int cols) { return index % cols; }
inline int coord_y(int index, int cols) { return index / cols; }
inline int coord_to_index(int x, int y, int cols) { return y * cols + x; }

// ---------------------------------------------------------------------------
// numerical stability
// ---------------------------------------------------------------------------

// clamp p to (1e-6, 1-1e-6) so log(p) never blows up during em or bp
double clamp_prob(double p);

// ---------------------------------------------------------------------------
// file i/o
// ---------------------------------------------------------------------------

// parse one data_file*.txt into a Grid; returns false on failure
bool read_grid_file(const std::string& path, Grid* grid);

// return sorted list of all data_file*.txt paths in a directory
std::vector<std::string> list_data_files(const std::string& dataset_dir);

// load all observation grids from a dataset directory (one per timestep)
std::vector<Grid> load_dataset(const std::string& dataset_dir);

// ---------------------------------------------------------------------------
// transition model  (random walk with boundary failure)
// ---------------------------------------------------------------------------

// build_neighbors: adjacency list nbrs[i] = {j, ...} for 4-connected grid
std::vector<std::vector<int>> build_neighbors(int rows, int cols);

// build_transition_adj: sparse P(X_t=j | X_{t-1}=i) for every cell i
//
// walk rule: pick direction uniformly from 4; if target is out-of-bounds
// stay put. so boundary cells accumulate self-transition mass.
// returns adj[i] = {(j, p), ...} where sum_j p = 1 for each i.
std::vector<std::vector<std::pair<int, double>>> build_transition_adj(
    int rows,
    int cols);

// ---------------------------------------------------------------------------
// emission model
// ---------------------------------------------------------------------------

// compute log p(Z_t | X_t=s) for every state s.
//
// for each cell k in the observation grid:
//   k == s  ->  log pw  if detected,  log(1-pw) otherwise
//   k != s  ->  log pc  if detected,  log(1-pc) otherwise
// returns logp[s] = sum over all cells k
std::vector<double> compute_emission_log(
    const Grid& obs,
    double pw,
    double pc);

// convert log-probs to scaled probs for use in emdw factor tables.
// subtracts max before exp to avoid underflow: exp(logp[i] - max_logp)
std::vector<double> scale_from_log(const std::vector<double>& logp);

// ---------------------------------------------------------------------------
// belief propagation inference
// ---------------------------------------------------------------------------

// posterior marginals from one bp pass
struct BPInferenceResult {
  std::vector<std::vector<double>> gamma;  // gamma[t][s] = P(X_t=s | Z_{1:T})
};

// run full bp pipeline: build factor graph -> loopy bp -> extract marginals
//
// obs:       observation grids (one per timestep)
// x_ids:     emdw rv ids for each hidden state variable X_t
// state_dom: domain [0, n_cells-1]
// pw, pc:    detection and clutter probabilities
BPInferenceResult run_bp_inference(
    const std::vector<Grid>& obs,
    const std::vector<emdw::RVIdType>& x_ids,
    const rcptr<std::vector<int>>& state_dom,
    double pw,
    double pc);
