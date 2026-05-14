// wumpus_model.cc -- BP inference helpers, emission/transition models, file I/O

#include "wumpus_model.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "clustergraph.hpp"
#include "discretetable.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

using namespace std;
using namespace emdw;
namespace fs = std::filesystem;

// type aliases
using T  = int;
using DT = DiscreteTable<T>;

// ============================================================================
// Factor helpers
// ============================================================================

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

static double prob_from_factor(const rcptr<Factor>& f, RVIdType var, int val) {
  const DT* dt = dynamic_cast<const DT*>(f.get());
  if (!dt) return 0.0;
  return dt->potentialAt({var}, {T(val)});
}

// ============================================================================
// Numerical stability
// ============================================================================

// clamp to (1e-6, 1-1e-6) -- prevents log(0) in emission factors
// and underflow in EM parameter updates
double clamp_prob(double p) {
  if (p < 1e-6)       return 1e-6;
  if (p > 1.0 - 1e-6) return 1.0 - 1e-6;
  return p;
}

// ============================================================================
// File I/O
// ============================================================================

// parse a single data_file*.txt into a Grid
// cells stored row-major: cells[y*cols + x]
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
    if ((int)row.size() != c) return false;  // col count mismatch
  }

  grid->rows = r;
  grid->cols = c;
  grid->cells.assign(r * c, 0);
  for (int y = 0; y < r; ++y)
    for (int x = 0; x < c; ++x)
      grid->cells[y * c + x] = rows[y][x];

  return true;
}

// find all data_file*.txt in dir, sorted (chronological order)
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

// load all observation grids from a dataset directory, indexed by timestep
vector<Grid> load_dataset(const string& dataset_dir) {
  vector<string> files = list_data_files(dataset_dir);
  vector<Grid> obs;
  Grid ref;
  for (const auto& path : files) {
    Grid g;
    if (!read_grid_file(path, &g))
      throw string("failed to read: ") + path;
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
// Transition model
// ============================================================================

// valid 4-neighbors (up/down/left/right) for each cell
vector<vector<int>> build_neighbors(int rows, int cols) {
  int n = rows * cols;
  vector<vector<int>> nbrs(n);
  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      int idx = y * cols + x;
      if (y > 0)          nbrs[idx].push_back((y - 1) * cols + x);  // up
      if (y + 1 < rows)   nbrs[idx].push_back((y + 1) * cols + x);  // down
      if (x > 0)          nbrs[idx].push_back(y * cols + (x - 1));  // left
      if (x + 1 < cols)   nbrs[idx].push_back(y * cols + (x + 1));  // right
    }
  }
  return nbrs;
}

// transition probs: random walk with failure (per mini_project.pdf)
// wumpus picks a direction uniformly; if out-of-bounds, stays put
// returns adj[i] = {(j, p_j)} where p_j = P(X_t=j | X_{t-1}=i)
vector<vector<pair<int, double>>> build_transition_adj(int rows, int cols) {
  int n = rows * cols;
  vector<vector<pair<int, double>>> adj(n);

  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      int idx   = y * cols + x;
      double stay = 0.0;  // probability mass from failed moves

      // up
      if (y == 0)          stay += 0.25;
      else adj[idx].push_back({(y-1)*cols + x, 0.25});
      // down
      if (y + 1 >= rows)   stay += 0.25;
      else adj[idx].push_back({(y+1)*cols + x, 0.25});
      // left
      if (x == 0)          stay += 0.25;
      else adj[idx].push_back({y*cols + (x-1), 0.25});
      // right
      if (x + 1 >= cols)   stay += 0.25;
      else adj[idx].push_back({y*cols + (x+1), 0.25});

      if (stay > 0.0) adj[idx].push_back({idx, stay});  // self-loop
    }
  }
  return adj;
}

// ============================================================================
// Emission model
// ============================================================================

// log p(Z_t | X_t=s) for each state s
// true cell s: detected w/ prob pw, missed w/ 1-pw
// clutter cell k!=s: detected w/ prob pc, true negative w/ 1-pc
// returns logp[s] for s in [0, n-1]
vector<double> compute_emission_log(const Grid& obs, double pw, double pc) {
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
      if (k == s)
        lp += (z == 1) ? lpw : l1pw;   // true cell
      else
        lp += (z == 1) ? lpc : l1pc;   // clutter
    }
    logp[s] = lp;
  }
  return logp;
}

// log-sum-exp trick: shift by max before exp() to avoid underflow
// keeps relative probs intact, stable for very negative log values
vector<double> scale_from_log(const vector<double>& logp) {
  double max_log = *max_element(logp.begin(), logp.end());
  vector<double> probs(logp.size(), 0.0);
  for (int i = 0; i < (int)logp.size(); ++i)
    probs[i] = exp(logp[i] - max_log);
  return probs;
}

// ============================================================================
// Belief propagation
// ============================================================================

// run sum-product BP on the HMM factor graph and return marginals
// gamma[t][s] = P(X_t=s | Z_1:T)
BPInferenceResult run_bp_inference(
    const vector<Grid>& obs,
    const vector<RVIdType>& x_ids,
    const rcptr<vector<int>>& state_dom,
    double pw,
    double pc) {
  int t_steps = (int)obs.size();
  int n       = (int)state_dom->size();

  vector<rcptr<Factor>> factors;

  // uniform prior over X_0
  vector<double> prior(n, 1.0);
  factors.push_back(make_unary_factor(x_ids[0], state_dom, prior));

  // emission factor for each timestep
  for (int t = 0; t < t_steps; ++t) {
    vector<double> probs = scale_from_log(compute_emission_log(obs[t], pw, pc));
    factors.push_back(make_unary_factor(x_ids[t], state_dom, probs));
  }

  // transition factor for each t > 0
  int rows = obs[0].rows;
  int cols = obs[0].cols;
  auto adj = build_transition_adj(rows, cols);
  for (int t = 1; t < t_steps; ++t)
    factors.push_back(make_transition_factor(x_ids[t], x_ids[t-1], state_dom, adj));

  // build Bethe cluster graph and run LBP
  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  loopyBP_CG(cg, msgs, msgQ, 0.0);

  // extract posteriors
  vector<vector<double>> gamma(t_steps, vector<double>(n, 0.0));
  for (int t = 0; t < t_steps; ++t) {
    rcptr<Factor> bel = queryLBP_CG(cg, msgs, {x_ids[t]})->normalize();
    for (int s = 0; s < n; ++s)
      gamma[t][s] = prob_from_factor(bel, x_ids[t], s);
  }

  return BPInferenceResult{gamma};
}
