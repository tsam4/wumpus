// ============================================================================
// Wumpus Tracker: HMM Marginal Inference via Belief Propagation
// ============================================================================
//
// This is the main program for the DE424 Wumpus Tracking mini-project.
// It implements a Hidden Markov Model (HMM) solution using:
//
// 1. Hidden state: X_t in {0..M*N-1}, representing Wumpus position (grid cell)
// 2. Observations: Z_t, grid of binary detections at timestep t
// 3. Transition model: random walk with failure (per mini_project.pdf)
// 4. Emission model: noisy detections with pw (true cell) and pc (clutter)
// 5. Inference: Belief Propagation (sum-product) to compute marginals
// 6. Learning: EM for unknown pw/pc (dataset 3 only)
//
// Workflow:
// - Load observation grids from dataset files
// - Run BP inference with given (or learned) parameters
// - Output one trajectory:
//   - _marginal.txt: argmax of per-timestep marginals P(X_t | Z_1:T)
//
// CLI usage:
//   wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]
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
// Output Utilities
// ============================================================================

// write_path: write trajectory to file in required format
//
// File format (per mini_project.pdf):
// Each line: x y (space-separated coordinates, zero-indexed)
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
// - dataset1: 5x5, 10 steps, pw=0.95, pc=0.05 (known params)
// - dataset2: 20x20, 20 steps, pw=0.90, pc=0.10 (known params)
// - dataset3: 10x20, 20 steps, pw/pc unknown (learned via EM, 6 iterations)
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
// Main Program: HMM Marginal Inference with Optional EM Learning
// ============================================================================

int main(int argc, char** argv) {
  // Require CLI arguments
  if (argc < 4) {
    cerr << "Usage: wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]\n";
    return 1;
  }

  // =========================================================================
  // SETUP PHASE: Load data and initialize inference domain
  // =========================================================================

  string dataset_dir = argv[1];
  int dataset_id     = atoi(argv[2]);
  string out_prefix  = argv[3];

  // Load dataset configuration with optional CLI overrides
  DatasetConfig cfg = dataset_defaults(dataset_id);
  parse_overrides(argc, argv, &cfg);

  // Load all observation grids from dataset directory
  vector<Grid> obs = load_dataset(dataset_dir);
  if (obs.empty()) {
    cerr << "no data files found in " << dataset_dir << "\n";
    return 1;
  }

  // Extract grid dimensions
  int rows = obs[0].rows;
  int cols = obs[0].cols;
  int n = rows * cols;             // total number of cells
  int t_steps = (int)obs.size();  // number of timesteps

  // Initialize emdw RV domain and identifiers
  // Domain: state values 0..n-1 representing each grid cell
  rcptr<vector<T>> state_dom(new vector<T>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;

  // RV identifiers: one RV per timestep (X_0, X_1, ..., X_{T-1})
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  // =========================================================================
  // INFERENCE PHASE: BP marginals (with optional EM parameter learning)
  // =========================================================================

  vector<vector<double>> gamma;  // gamma[t][s] = P(X_t=s | Z_1:T)

  if (cfg.max_iters == 0) {
    // No learning: run single BP pass with given parameters
    gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;
  } else {
    // EM: alternating E-step (BP) and M-step (parameter update)
    for (int iter = 0; iter < cfg.max_iters; ++iter) {
      // --- E-Step: compute posteriors with current parameters ---
      gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;

      // --- M-Step: update pw and pc from expected sufficient statistics ---
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

      cfg.pw = clamp_prob(true_det / (double)t_steps);
      cfg.pc = clamp_prob(clutter_det / (double)(t_steps * (n - 1)));
    }  // end EM loop

    // Final BP pass with learned parameters
    gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;
  }

  // =========================================================================
  // OUTPUT PHASE: argmax of marginals -> trajectory file
  // =========================================================================

  // Marginal trajectory: argmax_s P(X_t=s | Z_1:T) at each timestep
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

  write_path(out_prefix + "_marginal.txt", marginal_path, cols);

  return 0;
}
