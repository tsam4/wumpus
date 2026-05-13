// ============================================================================
// Wumpus Tracker: Main HMM Inference with Optional EM Learning
// ============================================================================
//
// This is the main program for the DE424 Wumpus Tracking mini-project.
// It implements a complete Hidden Markov Model (HMM) solution with:
//
// 1. Hidden state: X_t in {0..M*N-1}, representing Wumpus position (grid cell)
// 2. Observations: Z_t, grid of binary detections at timestep t
// 3. Transition model: random walk with failure (per mini_project.pdf)
// 4. Emission model: noisy detections with pw (true cell) and pc (clutter)
// 5. Inference: Belief Propagation (marginals) + Viterbi (MAP trajectory)
// 6. Learning: EM for unknown pw/pc (dataset 3 only)
//
// Workflow:
// - Load observation grids from dataset files
// - Run inference with given (or learned) parameters
// - Output two trajectories:
//   - _marginal.txt: argmax of per-timestep marginals
//   - _map.txt: most likely complete path (Viterbi)
//
// ============================================================================


#include "wumpus.hpp"
#include "wumpus_model.hpp"

// emdw (probabilistic graphical model toolkit) headers
#include "emdw.hpp"
#include "discretetable.hpp"  // DiscreteTable for factor representation
#include "clustergraph.hpp"   // ClusterGraph for factor graph
#include "lbp_cg.hpp"         // Loopy Belief Propagation on cluster graphs
#include "messagequeue.hpp"   // Message queue for BP scheduling

// standard headers
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace emdw;

// Short type aliases for emdw types
using T = int;                  // RV value type (discrete: 0..n-1)
using DT = DiscreteTable<T>;    // Factor implementation

// ============================================================================
// Configuration Flags
// ============================================================================
// Change these flags at the top of this file to control dataset and output options

// Which dataset to process (1-3)
// 1: 5×5 grid, 10 steps, pw=0.95, pc=0.05 (known params)
// 2: 20×20 grid, 20 steps, pw=0.90, pc=0.10 (known params)
// 3: 10×20 grid, 20 steps, unknown pw/pc (learned via EM)
static const int ACTIVE_DATASET = 3;

// Output directory for results (should match the dataset)
static const string OUTPUT_DIR = "." ;  // Current directory

// ============================================================================
// Runtime Configuration (derived from ACTIVE_DATASET)
// ============================================================================

static string get_dataset_path(int dataset_id) {
  switch (dataset_id) {
    case 1: return "./Datasets-20260506/dataset1";
    case 2: return "./Datasets-20260506/dataset2";
    case 3: return "./Datasets-20260506/dataset3";
    default: return "";
  }
}

static string get_output_prefix(int dataset_id) {
  switch (dataset_id) {
    case 1: return OUTPUT_DIR + "/out_d1";
    case 2: return OUTPUT_DIR + "/out_d2";
    case 3: return OUTPUT_DIR + "/out_d3";
    default: return "";
  }
}

// ============================================================================
// Factor Construction Utilities
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
// Output Utilities
// ============================================================================

// write_path: write trajectory to file in required format
//
// File format (per mini_project.pdf):
// Each line: x y (comma/space-separated coordinates, zero-indexed)
//
static void write_path(const string& path, const vector<int>& states, int cols) {
  ofstream out(path);
  for (int s : states) {
    int x = coord_x(s, cols);
    int y = coord_y(s, cols);
    out << x << " " << y << "\n";
  }
}

// ============================================================================
// Dataset Configuration
// ============================================================================

// dataset_defaults: load default parameters for each dataset
//
// Per mini_project.pdf Table in Section "Problem statement":
//
// - dataset1: 5×5, 10 steps, pw=0.95, pc=0.05 (known params)
// - dataset2: 20×20, 20 steps, pw=0.90, pc=0.10 (known params)
// - dataset3: 10×20, 20 steps, pw/pc unknown (learned via EM, 6 iterations)
//
static DatasetConfig dataset_defaults(int id) {
  DatasetConfig cfg;
  cfg.id = id;
  if (id == 1) {
    cfg.pw = 0.95;
    cfg.pc = 0.05;
    cfg.learn_pwpc = false;
    cfg.max_iters = 0;
  } else if (id == 2) {
    cfg.pw = 0.90;
    cfg.pc = 0.10;
    cfg.learn_pwpc = false;
    cfg.max_iters = 0;
  } else if (id == 3) {
    cfg.pw = 0.90;
    cfg.pc = 0.10;
    cfg.learn_pwpc = true;  // Enable EM for pw/pc
    cfg.max_iters = 6;
  }
  return cfg;
}

// parse_overrides: parse command-line argument overrides
//
// CLI format:
//   wumpus <dir> <id> <out> [--pw v] [--pc v] [--em n]
//
// Allows users to override defaults for manual testing.
//
static void parse_overrides(int argc, char** argv, DatasetConfig* cfg) {
  for (int i = 4; i < argc; ++i) {
    string a = argv[i];
    if (a == "--pw" && i + 1 < argc) {
      cfg->pw = atof(argv[++i]);
      cfg->learn_pwpc = false;  // disable EM if manually set
    } else if (a == "--pc" && i + 1 < argc) {
      cfg->pc = atof(argv[++i]);
      cfg->learn_pwpc = false;
    } else if (a == "--em" && i + 1 < argc) {
      cfg->max_iters = atoi(argv[++i]);  // override EM iteration count
    }
  }
}

// ============================================================================
// Main Program: HMM Inference with Optional EM Learning
// ============================================================================

int main(int argc, char** argv) {
  // =========================================================================
  // SETUP PHASE: Load data and initialize inference domain
  // =========================================================================

  // Use flag-based configuration if no command-line args, otherwise use CLI
  int dataset_id;
  string dataset_dir;
  string out_prefix;

  if (argc >= 4) {
    // Command-line mode (backward compatible)
    dataset_dir = argv[1];
    dataset_id = atoi(argv[2]);
    out_prefix = argv[3];
  } else {
    // Flag-based mode (development)
    dataset_id = ACTIVE_DATASET;
    dataset_dir = get_dataset_path(dataset_id);
    out_prefix = get_output_prefix(dataset_id);
    cout << "Using flag-based configuration (ACTIVE_DATASET=" << dataset_id << ")\n";
  }

  // Load dataset configuration
  DatasetConfig cfg = dataset_defaults(dataset_id);
  if (argc >= 4) {
    parse_overrides(argc, argv, &cfg);
  }

  // Load all observation grids from dataset directory
  vector<Grid> obs = load_dataset(dataset_dir);
  if (obs.empty()) {
    cerr << "no data files found in " << dataset_dir << "\n";
    return 1;
  }

  // Extract grid dimensions
  int rows = obs[0].rows;
  int cols = obs[0].cols;
  int n = rows * cols;        // total number of cells
  int t_steps = (int)obs.size();  // number of timesteps

  // Initialize emdw RV domain and identifiers
  // Domain: state values 0..n-1 representing each grid cell
  rcptr<vector<T>> state_dom(new vector<T>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;

  // RV identifiers: one RV per timestep (X_0, X_1, ..., X_{T-1})
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  // =========================================================================
  // LEARNING PHASE: Run EM if parameters need to be learned
  // =========================================================================

  vector<vector<double>> gamma;      // posteriors: gamma[t][s] = P(X_t=s|Z_1:T)
  vector<vector<double>> emis_log;   // log emission probabilities for Viterbi

  if (cfg.max_iters == 0) {
    // No learning: just run inference with given parameters
    BPInferenceResult inf_result = run_bp_inference(obs, x_ids, state_dom,
        cfg.pw, cfg.pc);
    gamma = inf_result.gamma;
    emis_log = inf_result.emis_log;
  } else {
    // Run EM iterations for dataset 3 (learn pw/pc)
    for (int iter = 0; iter < cfg.max_iters; ++iter) {
      // --- E-Step: Compute posteriors with current parameters ---
      BPInferenceResult inf_result = run_bp_inference(obs, x_ids, state_dom,
          cfg.pw, cfg.pc);
      gamma = inf_result.gamma;
      emis_log = inf_result.emis_log;

      // --- M-Step: Update pw and pc ---
      double true_det = 0.0;
      double clutter_det = 0.0;
      
      for (int t = 0; t < t_steps; ++t) {
        int total_det = 0;
        for (int k = 0; k < n; ++k) total_det += obs[t].cells[k];
        
        double exp_true_det = 0.0;
        for (int s = 0; s < n; ++s) {
          if (obs[t].cells[s] == 1) exp_true_det += gamma[t][s];
        }
        
        true_det += exp_true_det;
        clutter_det += (double)total_det - exp_true_det;
      }
      
      double pw_new = true_det / (double)t_steps;
      double pc_new = clutter_det / (double)(t_steps * (n - 1));
      cfg.pw = clamp_prob(pw_new);
      cfg.pc = clamp_prob(pc_new);
    }  // end EM loop

    // --- Final BP Pass with learned parameters ---
    BPInferenceResult inf_result = run_bp_inference(obs, x_ids, state_dom,
        cfg.pw, cfg.pc);
    gamma = inf_result.gamma;
    emis_log = inf_result.emis_log;
  }

  // =========================================================================
  // OUTPUT PHASE: Generate and write trajectories
  // =========================================================================

  // Prepare transition model and prior for trajectory extraction
  vector<vector<pair<int, double>>> adj = build_transition_adj(rows, cols);
  vector<double> prior_log(n, 0.0);
  for (int s = 0; s < n; ++s) {
    prior_log[s] = log(1.0);  // uniform prior
  }

  // Extract marginal trajectory: argmax_s P(X_t=s | Z_1:T) at each time
  vector<int> marginal_path(t_steps, 0);
  for (int t = 0; t < t_steps; ++t) {
    int best = 0;
    double best_p = gamma[t][0];
    for (int s = 1; s < n; ++s) {
      if (gamma[t][s] > best_p) {
        best_p = gamma[t][s];
        best = s;
      }
    }
    marginal_path[t] = best;
  }

  // Extract MAP trajectory: argmax_{X_1:T} P(X_1:T | Z_1:T) using Viterbi
  vector<int> map_path = viterbi_path(adj, emis_log, prior_log);

  // Write output files
  write_path(out_prefix + "_marginal.txt", marginal_path, cols);
  write_path(out_prefix + "_map.txt", map_path, cols);

  return 0;
}
