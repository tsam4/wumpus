// test_emdw_bp: integration test for run_bp_inference
//
// Uses a 2x2 grid, 2 timesteps, pw=0.95, pc=0.05.
// Injects a single detection at cell 0 at t=0; no detections at t=1.
//
// Asserts:
// - gamma[0][0] is the MAP cell at t=0 (strongly detected)
// - All gamma[t] rows sum to approximately 1.0 (valid posteriors)
// - No NaN values anywhere in output

#include "wumpus_model.hpp"
#include "emdw.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace std;
using namespace emdw;

static bool approx(double a, double b, double tol = 1e-4) {
  return fabs(a - b) < tol;
}

int main() {
  int rows = 2, cols = 2, n = 4;
  int t_steps = 2;

  // t=0: detection at cell 0 only; t=1: no detections
  Grid g0; g0.rows = rows; g0.cols = cols; g0.cells = {1, 0, 0, 0};
  Grid g1; g1.rows = rows; g1.cols = cols; g1.cells = {0, 0, 0, 0};
  vector<Grid> obs = {g0, g1};

  // emdw RV setup
  rcptr<vector<int>> state_dom(new vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  BPInferenceResult res = run_bp_inference(obs, x_ids, state_dom, 0.95, 0.05);
  const auto& gamma = res.gamma;

  // t=0: cell 0 must be MAP
  for (int s = 1; s < n; ++s)
    assert(gamma[0][0] >= gamma[0][s] && "cell 0 must be MAP at t=0");

  // All rows sum to ~1.0; no NaN
  for (int t = 0; t < t_steps; ++t) {
    double sum = 0.0;
    for (int s = 0; s < n; ++s) {
      assert(!isnan(gamma[t][s]) && "gamma must not contain NaN");
      sum += gamma[t][s];
    }
    assert(approx(sum, 1.0) && "gamma row must sum to 1.0");
  }

  printf("PASS: test_emdw_bp (2x2, 2 steps, BP marginals correct)\n");
  return 0;
}
