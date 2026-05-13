// wumpus shared model helpers

#include "wumpus_model.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

// emdw headers for BP inference
#include "clustergraph.hpp"
#include "discretetable.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

using namespace std;
using namespace emdw;
namespace fs = std::filesystem;

// Type aliases for emdw
using T = int;
using DT = DiscreteTable<T>;

// ============================================================================
// Factor Construction Utilities (for BP inference)
// ============================================================================

// Helper to create unary factors
static rcptr<Factor> make_unary_factor(
    RVIdType var,
    const rcptr<vector<T>>& dom,
    const vector<double>& probs) {
  map<vector<T>, FProb> sparse;
  for (int i = 0; i < (int)probs.size(); ++i) {
    if (probs[i] <= 0.0) continue;
    sparse[{T(i)}] = probs[i];
  }
  return uniqptr<DT>(new DT({var}, {dom}, 0.0, sparse));
}

// Helper to create transition factors
static rcptr<Factor> make_transition_factor(
    RVIdType next_var,
    RVIdType prev_var,
    const rcptr<vector<T>>& dom,
    const vector<vector<pair<int, double>>>& adj) {
  map<vector<T>, FProb> sparse;
  for (int prev = 0; prev < (int)adj.size(); ++prev) {
    for (const auto& kv : adj[prev]) {
      int next = kv.first;
      double p = kv.second;
      if (p <= 0.0) continue;
      sparse[{T(next), T(prev)}] = p;
    }
  }
  return uniqptr<DT>(new DT({next_var, prev_var}, {dom, dom}, 0.0, sparse));
}

// Helper to extract marginal probability from a factor
static double prob_from_factor(const rcptr<Factor>& f, RVIdType var, int val) {
  const DT* dt = dynamic_cast<const DT*>(f.get());
  if (!dt) return 0.0;
  return dt->potentialAt({var}, {T(val)});
}

// ============================================================================
// Numerical Stability
// ============================================================================
// Clamp probability to safe open interval (1e-6, 1-1e-6)
//
// Why this matters:
// - log(0) is undefined and would produce -inf or NaN
// - When we compute log(pw), log(1-pw), log(pc), log(1-pc) in emission
//   factors, edge-case probabilities (0 or 1) would cause failures
// - Also prevents extremely small values from underflowing in exp()
// - Critical for EM parameter updates where pw/pc might converge toward 0
//
// Range: (1e-6, 1-1e-6) is safe for double-precision logarithm computation
//
double clamp_prob(double p) {
  if (p < 1e-6) return 1e-6;
  if (p > 1.0 - 1e-6) return 1.0 - 1e-6;
  return p;
}

// ============================================================================
// File I/O Helpers
// ============================================================================

// read_grid_file: parse a single data_file*.txt into a Grid struct
//
// File format (per mini_project.pdf):
// Each line contains space-separated integers (0 or 1) representing
// one row of detections at a single timestep. Example (5x5 grid):
//   1 0 0 0 0
//   0 0 0 0 0
//   0 0 1 0 0
//   0 0 0 0 0
//   0 0 1 0 0
//
// Validation:
// - All rows must have the same number of columns
// - File must be non-empty
// - Cells are stored in row-major order: cells[y*cols + x]
//
bool read_grid_file(const string& path, Grid* grid) {
  ifstream in(path);
  if (!in.is_open()) return false;

  vector<vector<int>> rows;
  string line;
  while (getline(in, line)) {
    if (line.empty()) continue;
    istringstream iss(line);
    vector<int> row;
    int v = 0;
    while (iss >> v) row.push_back(v);
    if (!row.empty()) rows.push_back(row);
  }
  if (rows.empty()) return false;

  int r = (int)rows.size();
  int c = (int)rows[0].size();
  for (const auto& row : rows) {
    if ((int)row.size() != c) return false;  // size mismatch
  }

  grid->rows = r;
  grid->cols = c;
  grid->cells.assign(r * c, 0);
  for (int y = 0; y < r; ++y) {
    for (int x = 0; x < c; ++x) {
      grid->cells[y * c + x] = rows[y][x];
    }
  }
  return true;
}

// list_data_files: find all data_file*.txt files in dataset directory
//
// Assumes files are named data_fileNNN.txt where NNN is zero-padded index.
// Sorting ensures they are processed in chronological order.
//
// Example: data_file000.txt, data_file001.txt, ..., data_file019.txt
//
vector<string> list_data_files(const string& dataset_dir) {
  vector<string> files;
  for (const auto& entry : fs::directory_iterator(dataset_dir)) {
    if (!entry.is_regular_file()) continue;
    string name = entry.path().filename().string();
    if (name.rfind("data_file", 0) != 0) continue;
    if (entry.path().extension().string() != ".txt") continue;
    files.push_back(entry.path().string());
  }
  sort(files.begin(), files.end());
  return files;
}

// load_dataset: load all observation grids from a dataset directory
//
// Returns vector<Grid> indexed by timestep 0, 1, 2, ...
// Validates all grids have same dimensions (rectangular world)
// Throws exception if parsing fails
//
vector<Grid> load_dataset(const string& dataset_dir) {
  vector<string> files = list_data_files(dataset_dir);
  vector<Grid> obs;
  Grid ref;
  for (const auto& path : files) {
    Grid g;
    if (!read_grid_file(path, &g)) {
      throw string("failed to read: ") + path;
    }
    if (obs.empty()) {
      ref = g;
    } else if (g.rows != ref.rows || g.cols != ref.cols) {
      throw string("grid size mismatch in: ") + path;
    }
    obs.push_back(g);
  }
  return obs;
}

// ============================================================================
// Transition Model
// ============================================================================

// build_neighbors: compute valid 4-neighbors (up/down/left/right) for each cell
//
// For each cell (y,x), computes which cells it can move to:
// - Up:    (y-1, x) if y > 0
// - Down:  (y+1, x) if y+1 < rows
// - Left:  (y, x-1) if x > 0
// - Right: (y, x+1) if x+1 < cols
//
// Used to define movement constraints in transition model.
// Returns adjacency list: nbrs[s] = {s1, s2, ...} = neighbors of cell s
//
vector<vector<int>> build_neighbors(int rows, int cols) {
  int n = rows * cols;
  vector<vector<int>> nbrs(n);
  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      int idx = y * cols + x;
      if (y > 0) nbrs[idx].push_back((y - 1) * cols + x);       // up
      if (y + 1 < rows) nbrs[idx].push_back((y + 1) * cols + x); // down
      if (x > 0) nbrs[idx].push_back(y * cols + (x - 1));        // left
      if (x + 1 < cols) nbrs[idx].push_back(y * cols + (x + 1)); // right
    }
  }
  return nbrs;
}

// build_transition_adj: construct transition probabilities with occupancy
//
// Implements HMM transition model per mini_project.pdf:
//
// Random walk with failure mechanism:
// 1. Wumpus picks a direction uniformly: U ~ Uniform({up, down, left, right})
// 2. Attempts to move in direction U, landing in cell j
// 3. If j is out-of-bounds or occupied, move fails and wumpus stays put
//
// Probability calculation (per mini_project.pdf Section 1):
//
// For each direction (4 equiprobable choices):
// - If direction is out-of-bounds:
//     P(stay) += 0.25
// - If direction points to cell j:
//     P(move to j) += 0.25
//
// Returns sparse adjacency list:
//   adj[i] = {(j, p_j), ...} where p_j = P(X_t=j | X_{t-1}=i)
//   Self-loop (i,i) is included if P(stay) > 0
//
vector<vector<pair<int, double>>> build_transition_adj(
    int rows,
    int cols) {
  int n = rows * cols;
  vector<vector<pair<int, double>>> adj(n);

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      int idx = y * cols + x;
      double stay = 0.0;  // accumulate failed move attempts

      // up
      if (y == 0) {
        stay += 0.25;  // out of bounds, stay put
      } else {
        int j = (y - 1) * cols + x;
        adj[idx].push_back({j, 0.25});
      }
      // down
      if (y + 1 >= rows) {
        stay += 0.25;
      } else {
        int j = (y + 1) * cols + x;
        adj[idx].push_back({j, 0.25});
      }
      // left
      if (x == 0) {
        stay += 0.25;
      } else {
        int j = y * cols + (x - 1);
        adj[idx].push_back({j, 0.25});
      }
      // right
      if (x + 1 >= cols) {
        stay += 0.25;
      } else {
        int j = y * cols + (x + 1);
        adj[idx].push_back({j, 0.25});
      }

      if (stay > 0.0) adj[idx].push_back({idx, stay});  // self-loop for failed moves
    }
  }

  return adj;
}

// ============================================================================
// Emission Model
// ============================================================================

// compute_emission_log: compute log-likelihood of observations given state
//
// Implements emission model per mini_project.pdf Section 1:
//
// Given observation grid Z_t and proposed state X_t=s, compute
//   log p(Z_t | X_t=s)
//
// Key assumptions:
// 1. Detections are conditionally independent given state
// 2. True cell s generates detection with probability pw (miss with 1-pw)
// 3. Clutter cells k!=s generate detection with probability pc (TN with 1-pc)
//
// For each proposed state s:
//   log p(Z_t | X_t=s) = sum_k log P(z_k | X_t=s, k)
//
// Where:
//   if k == s:
//     log P(z_k=1 | s) = log(pw)
//     log P(z_k=0 | s) = log(1-pw)
//   else:
//     log P(z_k=1 | s) = log(pc)
//     log P(z_k=0 | s) = log(1-pc)
//
// All probabilities are clamped to avoid log(0) issues.
// Returns vector logp[s] for all states s in [0, n-1]
//
vector<double> compute_emission_log(
    const Grid& obs,
    double pw,
    double pc) {
  int n = obs.rows * obs.cols;
  vector<double> logp(n, 0.0);
  double lpw  = log(clamp_prob(pw));
  double l1pw = log(clamp_prob(1.0 - pw));
  double lpc  = log(clamp_prob(pc));
  double l1pc = log(clamp_prob(1.0 - pc));

  for (int s = 0; s < n; ++s) {
    double lp = 0.0;
    for (int k = 0; k < n; ++k) {
      int z = obs.cells[k];
      if (k == s) {
        // true cell: detection prob = pw, miss prob = 1-pw
        lp += (z == 1) ? lpw : l1pw;
      } else {
        // clutter: detection prob = pc, true negative prob = 1-pc
        lp += (z == 1) ? lpc : l1pc;
      }
    }
    logp[s] = lp;
  }
  return logp;
}

// scale_from_log: numerically stable conversion from log-space to probabilities
//
// Given log-probabilities logp[i], we want to convert back to probabilities
// for use in emdw::DiscreteTable factors.
//
// Naive approach: exp(logp[i]) can underflow to 0 for very negative values
//
// Stable approach (log-sum-exp trick):
// 1. Find max_log = max_i logp[i]
// 2. Compute probs[i] = exp(logp[i] - max_log)
// 3. This keeps exponent values in [-large, 0], avoiding underflow
// 4. Relative probabilities are preserved
//
// Example:
//   logp = [-100, -101, -102, -1000]
//   max_log = -100
//   probs = [exp(0), exp(-1), exp(-2), exp(-900)] = [1.0, ~0.368, ~0.135, ~0]
//
vector<double> scale_from_log(const vector<double>& logp) {
  double max_log = *max_element(logp.begin(), logp.end());
  vector<double> probs(logp.size(), 0.0);
  for (int i = 0; i < (int)logp.size(); ++i) {
    probs[i] = exp(logp[i] - max_log);
  }
  return probs;
}

// ============================================================================
// Belief Propagation Inference
// ============================================================================

// run_bp_inference: unified BP inference pipeline
//
// Encapsulates the BP setup -> run -> extraction pattern used in both
// EM iterations and the final inference pass.
//
// Workflow:
// 1. Construct factors: prior, emission (per timestep), transition (per timestep)
// 2. Build Bethe cluster graph from factors
// 3. Run loopy belief propagation (sum-product) to convergence
// 4. Extract marginal posteriors gamma[t][s] = P(X_t=s | Z_1:T)
//
BPInferenceResult run_bp_inference(
    const vector<Grid>& obs,
    const vector<RVIdType>& x_ids,
    const rcptr<vector<int>>& state_dom,
    double pw,
    double pc) {
  int t_steps = (int)obs.size();
  int n = (int)state_dom->size();

  // Build factors
  vector<rcptr<Factor>> factors;

  // Prior factor: P(X_0) = 1 (uniform)
  vector<double> prior(n, 1.0);
  factors.push_back(make_unary_factor(x_ids[0], state_dom, prior));

  // Emission factors: P(Z_t | X_t) for each timestep
  for (int t = 0; t < t_steps; ++t) {
    vector<double> logp = compute_emission_log(obs[t], pw, pc);
    vector<double> probs = scale_from_log(logp);
    factors.push_back(make_unary_factor(x_ids[t], state_dom, probs));
  }

  // Transition factors: P(X_t | X_{t-1}) for each timestep
  int rows = obs[0].rows;
  int cols = obs[0].cols;
  vector<vector<pair<int, double>>> adj = build_transition_adj(rows, cols);
  for (int t = 1; t < t_steps; ++t) {
    factors.push_back(make_transition_factor(x_ids[t], x_ids[t - 1], state_dom, adj));
  }

  // Run loopy belief propagation (sum-product)
  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  loopyBP_CG(cg, msgs, msgQ, 0.0);

  // Extract posteriors: gamma[t][s] = P(X_t=s | Z_1:T)
  vector<vector<double>> gamma(t_steps, vector<double>(n, 0.0));
  for (int t = 0; t < t_steps; ++t) {
    rcptr<Factor> bel = queryLBP_CG(cg, msgs, {x_ids[t]})->normalize();
    for (int s = 0; s < n; ++s) {
      gamma[t][s] = prob_from_factor(bel, x_ids[t], s);
    }
  }

  return BPInferenceResult{gamma};
}
