// test_em_extremes: EM robustness under degenerate observation conditions
//
// Verifies:
// - clamp_prob(0.0) and clamp_prob(1.0) produce valid interior values
// - EM with all-zero observations completes without crash or NaN
// - EM with all-one observations completes without crash or NaN
// - pw/pc remain strictly in (0, 1) after each M-step

#include "wumpus_model.hpp"
#include "emdw.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace std;
using namespace emdw;

static void run_em_loop(const vector<Grid>& obs, double pw0, double pc0) {
  int n = obs[0].rows * obs[0].cols;
  int t_steps = (int)obs.size();

  rcptr<vector<int>> state_dom(new vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  double pw = pw0, pc = pc0;
  for (int iter = 0; iter < 3; ++iter) {
    // E-step
    auto res = run_bp_inference(obs, x_ids, state_dom, pw, pc);
    const auto& gamma = res.gamma;

    // M-step
    double true_det = 0.0, clutter_det = 0.0;
    for (int t = 0; t < t_steps; ++t) {
      int total_det = 0;
      for (int k = 0; k < n; ++k) total_det += obs[t].cells[k];
      double exp_true = 0.0;
      for (int s = 0; s < n; ++s)
        if (obs[t].cells[s] == 1) exp_true += gamma[t][s];
      true_det    += exp_true;
      clutter_det += (double)total_det - exp_true;
    }
    pw = clamp_prob(true_det / (double)t_steps);
    pc = clamp_prob(clutter_det / (double)(t_steps * (n - 1)));

    assert(!isnan(pw) && "pw must not be NaN after M-step");
    assert(!isnan(pc) && "pc must not be NaN after M-step");
    assert(pw > 0.0 && pw < 1.0 && "pw must be in (0,1) after M-step");
    assert(pc > 0.0 && pc < 1.0 && "pc must be in (0,1) after M-step");
  }
}

int main() {
  // clamp_prob boundary checks
  assert(clamp_prob(0.0) >= 1e-6  && "0.0 must clamp up");
  assert(clamp_prob(1.0) <= 1.0 - 1e-6 && "1.0 must clamp down");
  printf("PASS: clamp_prob boundaries\n");

  // All-zero observations
  {
    Grid g; g.rows = 3; g.cols = 3; g.cells.assign(9, 0);
    vector<Grid> obs(4, g);
    run_em_loop(obs, 0.9, 0.1);
    printf("PASS: EM with all-zero observations\n");
  }

  // All-one observations
  {
    Grid g; g.rows = 3; g.cols = 3; g.cells.assign(9, 1);
    vector<Grid> obs(4, g);
    run_em_loop(obs, 0.9, 0.1);
    printf("PASS: EM with all-one observations\n");
  }

  printf("All EM extreme tests passed.\n");
  return 0;
}
