// basic test: emission likelihood per mini_project.pdf

#include "wumpus_model.hpp"

#include <cassert>
#include <cmath>
#include <vector>

static void expect_close(double a, double b, double eps) {
  assert(std::fabs(a - b) <= eps);
}

int main() {
  // 2x2 grid example
  Grid g;
  g.rows = 2;
  g.cols = 2;
  g.cells = {1, 0, 0, 1}; // row-major

  double pw = 0.9;
  double pc = 0.1;
  std::vector<double> occ(4, 0.0);

  // state s=0: z0 uses pw, others use pc
  auto logp = compute_emission_log(g, pw, pc, occ, false);
  double expected = std::log(pw) + std::log(1.0 - pc) + std::log(1.0 - pc) + std::log(pc);
  expect_close(logp[0], expected, 1e-12);

  // add occupancy prior (open prob) on state 0
  occ[0] = 0.25;
  auto logp_occ = compute_emission_log(g, pw, pc, occ, true);
  double expected_occ = expected + std::log(1.0 - occ[0]);
  expect_close(logp_occ[0], expected_occ, 1e-12);

  return 0;
}
