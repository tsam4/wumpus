// test_e2e_sim: end-to-end accuracy test on synthetic data
//
// Simulates a Wumpus on a 3x3 grid for 5 timesteps at known positions.
// Generates clean synthetic observations (only the true cell fires).
// Runs BP and checks MAP prediction matches ground truth in >= 4/5 steps.
//
// This is a soft accuracy check — it can fail if the chain of transitions
// makes the true cell ambiguous, but on clean data (pw=0.95, pc=0.05)
// with a single firing cell it should always pass.

#include "../../src/wumpus_model.hpp"
#include "emdw.hpp"
#include <cassert>
#include <cstdio>
#include <vector>

using namespace std;
using namespace emdw;

int main() {
  int rows = 3, cols = 3, n = 9;
  int t_steps = 5;
  double pw = 0.95, pc = 0.05;

  // Known ground-truth Wumpus positions (cell indices, row-major)
  vector<int> truth = {4, 4, 5, 5, 8};

  // Clean synthetic observations: only true cell fires
  vector<Grid> obs(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    obs[t].rows = rows;
    obs[t].cols = cols;
    obs[t].cells.assign(n, 0);
    obs[t].cells[truth[t]] = 1;
  }

  // emdw RV setup
  rcptr<vector<int>> state_dom(new vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  // Run BP
  auto res = run_bp_inference(obs, x_ids, state_dom, pw, pc);
  const auto& gamma = res.gamma;

  // Count MAP matches
  int correct = 0;
  for (int t = 0; t < t_steps; ++t) {
    int best = 0;
    for (int s = 1; s < n; ++s)
      if (gamma[t][s] > gamma[t][best]) best = s;
    if (best == truth[t]) ++correct;
  }

  printf("MAP accuracy: %d/%d\n", correct, t_steps);
  assert(correct >= 4 && "MAP accuracy must be >= 4/5 on clean synthetic data");
  printf("PASS: test_e2e_sim\n");
  return 0;
}
