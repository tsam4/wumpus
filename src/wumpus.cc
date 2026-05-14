// ============================================================================
// wumpus.cc -- HMM marginal inference via belief propagation
//
// main program for DE424 wumpus tracking mini-project.
// hidden state X_t in {0..M*N-1} = wumpus grid cell at time t.
// observations Z_t = binary detection grid (noisy).
// inference: sum-product BP over factor graph to get marginals P(X_t | Z_1:T).
// learning: EM to estimate pw/pc when unknown (dataset 3 only).
//
// usage: wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]
// ============================================================================

#include "wumpus.hpp"
#include "wumpus_model.hpp"

// emdw probabilistic toolkit
#include "emdw.hpp"
#include "discretetable.hpp"  // factor representation
#include "clustergraph.hpp"   // factor graph structure
#include "lbp_cg.hpp"         // loopy BP on cluster graphs
#include "messagequeue.hpp"   // message scheduling

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace emdw;

// type aliases for emdw
using T  = int;               // discrete RV value type
using DT = DiscreteTable<T>;  // factor table type

// ============================================================================
// Output
// ============================================================================

// write trajectory to file -- one "x y" pair per line, zero-indexed
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

// default pw/pc/em settings per dataset (from mini_project.pdf)
static DatasetConfig dataset_defaults(int id) {
  DatasetConfig cfg;
  cfg.id = id;
  if (id == 1) {
    cfg.pw = 0.95;
    cfg.pc = 0.05;
    cfg.learn_pwpc = false;
    cfg.max_iters  = 0;
  } else if (id == 2) {
    cfg.pw = 0.90;
    cfg.pc = 0.10;
    cfg.learn_pwpc = false;
    cfg.max_iters  = 0;
  } else if (id == 3) {
    cfg.pw = 0.90;  // initial guess before EM
    cfg.pc = 0.10;
    cfg.learn_pwpc = true;
    cfg.max_iters  = 6;
  }
  return cfg;
}

// optional CLI overrides: --pw, --pc, --em
static void parse_overrides(int argc, char** argv, DatasetConfig* cfg) {
  for (int i = 4; i < argc; ++i) {
    string a = argv[i];
    if (a == "--pw" && i + 1 < argc) {
      cfg->pw = atof(argv[++i]);
      cfg->learn_pwpc = false;  // manual pw disables EM
    } else if (a == "--pc" && i + 1 < argc) {
      cfg->pc = atof(argv[++i]);
      cfg->learn_pwpc = false;
    } else if (a == "--em" && i + 1 < argc) {
      cfg->max_iters = atoi(argv[++i]);
    }
  }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
  if (argc < 4) {
    cerr << "usage: wumpus <dataset_dir> <dataset_id> <out_prefix> [--pw v] [--pc v] [--em n]\n";
    return 1;
  }

  string dataset_dir = argv[1];
  int    dataset_id  = atoi(argv[2]);
  string out_prefix  = argv[3];

  DatasetConfig cfg = dataset_defaults(dataset_id);
  parse_overrides(argc, argv, &cfg);

  vector<Grid> obs = load_dataset(dataset_dir);
  if (obs.empty()) {
    cerr << "no data files found in " << dataset_dir << "\n";
    return 1;
  }

  int rows    = obs[0].rows;
  int cols    = obs[0].cols;
  int n       = rows * cols;       // total cells
  int t_steps = (int)obs.size();

  // build emdw RV domain: cell indices 0..n-1
  rcptr<vector<T>> state_dom(new vector<T>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;

  // one RV id per timestep
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  // gamma[t][s] = P(X_t=s | Z_1:T)
  vector<vector<double>> gamma;

  if (cfg.max_iters == 0) {
    // known params -- single BP pass
    gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;
  } else {
    // EM: E-step = BP, M-step = update pw/pc from expected counts
    for (int iter = 0; iter < cfg.max_iters; ++iter) {
      // E-step
      gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;

      // M-step
      double true_det   = 0.0;
      double clutter_det = 0.0;

      for (int t = 0; t < t_steps; ++t) {
        int total_det = 0;
        for (int k = 0; k < n; ++k) total_det += obs[t].cells[k];

        // expected detections at wumpus cell
        double exp_true_det = 0.0;
        for (int s = 0; s < n; ++s) {
          if (obs[t].cells[s] == 1) exp_true_det += gamma[t][s];
        }

        true_det    += exp_true_det;
        clutter_det += (double)total_det - exp_true_det;
      }

      cfg.pw = clamp_prob(true_det / (double)t_steps);
      cfg.pc = clamp_prob(clutter_det / (double)(t_steps * (n - 1)));
    }

    // final pass with learned params
    gamma = run_bp_inference(obs, x_ids, state_dom, cfg.pw, cfg.pc).gamma;
  }

  // argmax marginals -> trajectory
  vector<int> marginal_path(t_steps, 0);
  for (int t = 0; t < t_steps; ++t) {
    int    best   = 0;
    double best_p = gamma[t][0];
    for (int s = 1; s < n; ++s) {
      if (gamma[t][s] > best_p) {
        best_p = gamma[t][s];
        best   = s;
      }
    }
    marginal_path[t] = best;
  }

  write_path(out_prefix + "_marginal.txt", marginal_path, cols);

  return 0;
}
