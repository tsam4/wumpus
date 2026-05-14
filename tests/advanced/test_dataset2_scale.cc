// test_dataset2_scale: scalability test matching dataset2 spec (20x20, 20 steps)
//
// Constructs a 20x20 grid with 20 timesteps entirely in memory (no file I/O).
// Runs full BP inference and verifies:
// - No crash or assertion failure
// - No NaN in any output marginal
//
// Set env var SKIP_SLOW=1 to skip this test in constrained CI environments.

#include "wumpus_model.hpp"
#include "emdw.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;
using namespace emdw;

int main() {
  const char* skip = getenv("SKIP_SLOW");
  if (skip && strcmp(skip, "1") == 0) {
    printf("SKIP: test_dataset2_scale (SKIP_SLOW=1)\n");
    return 0;
  }

  int rows = 20, cols = 20, n = 400;
  int t_steps = 20;
  double pw = 0.90, pc = 0.10;

  // Synthetic observations: one detection per timestep, cycling through cells
  vector<Grid> obs(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    obs[t].rows = rows;
    obs[t].cols = cols;
    obs[t].cells.assign(n, 0);
    obs[t].cells[t % n] = 1;
  }

  rcptr<vector<int>> state_dom(new vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  auto res = run_bp_inference(obs, x_ids, state_dom, pw, pc);
  const auto& gamma = res.gamma;

  for (int t = 0; t < t_steps; ++t)
    for (int s = 0; s < n; ++s)
      assert(!isnan(gamma[t][s]) && "gamma must not contain NaN");

  printf("PASS: test_dataset2_scale (20x20, 20 steps, no crash, no NaN)\n");
  return 0;
}
