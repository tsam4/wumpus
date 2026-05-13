// test_emission: unit tests for compute_emission_log and scale_from_log
//
// Covers (per mini_project.pdf emission model):
// - True cell receives the highest log-probability
// - scale_from_log produces values in (0, 1] with max element == 1.0
// - clamp_prob clips 0.0 and 1.0 to safe interior values
// - All-zero and all-one observation grids produce no NaN or -inf

#include "../../src/wumpus_model.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace std;

static bool approx(double a, double b, double tol = 1e-9) {
  return fabs(a - b) < tol;
}

void test_true_cell_highest() {
  // 2x2 grid, detection only at cell 0 => cell 0 must have highest log-prob
  Grid g;
  g.rows = 2; g.cols = 2;
  g.cells = {1, 0, 0, 0};
  auto logp = compute_emission_log(g, 0.95, 0.05);
  for (int s = 1; s < 4; ++s)
    assert(logp[0] > logp[s] && "cell 0 must have highest log-prob");
  printf("PASS: test_true_cell_highest\n");
}

void test_scale_from_log_range() {
  // scale_from_log must give values in (0,1] with max == 1.0
  Grid g;
  g.rows = 3; g.cols = 3;
  g.cells.assign(9, 0);
  g.cells[4] = 1;
  auto logp = compute_emission_log(g, 0.95, 0.05);
  auto probs = scale_from_log(logp);
  double maxp = 0.0;
  for (double p : probs) {
    assert(p > 0.0 && p <= 1.0 && "scaled probability out of (0,1]");
    if (p > maxp) maxp = p;
  }
  assert(approx(maxp, 1.0) && "max scaled probability must be 1.0");
  printf("PASS: test_scale_from_log_range\n");
}

void test_clamp_prob_boundaries() {
  assert(clamp_prob(0.0) >= 1e-6 && "0.0 must be clamped up");
  assert(clamp_prob(1.0) <= 1.0 - 1e-6 && "1.0 must be clamped down");
  assert(approx(clamp_prob(0.5), 0.5) && "interior value must be unchanged");
  printf("PASS: test_clamp_prob_boundaries\n");
}

void test_no_nan_all_zeros() {
  Grid g;
  g.rows = 2; g.cols = 2;
  g.cells = {0, 0, 0, 0};
  auto logp = compute_emission_log(g, 0.95, 0.05);
  for (double v : logp) {
    assert(!isnan(v) && "log-prob must not be NaN for all-zero obs");
    assert(!isinf(v) && "log-prob must not be inf for all-zero obs");
  }
  printf("PASS: test_no_nan_all_zeros\n");
}

void test_no_nan_all_ones() {
  Grid g;
  g.rows = 2; g.cols = 2;
  g.cells = {1, 1, 1, 1};
  auto logp = compute_emission_log(g, 0.95, 0.05);
  for (double v : logp) {
    assert(!isnan(v) && "log-prob must not be NaN for all-ones obs");
    assert(!isinf(v) && "log-prob must not be inf for all-ones obs");
  }
  printf("PASS: test_no_nan_all_ones\n");
}

int main() {
  test_true_cell_highest();
  test_scale_from_log_range();
  test_clamp_prob_boundaries();
  test_no_nan_all_zeros();
  test_no_nan_all_ones();
  printf("All emission tests passed.\n");
  return 0;
}
