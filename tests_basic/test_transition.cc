// basic test: transition logic per mini_project.pdf

#include "wumpus_model.hpp"

#include <cassert>
#include <cmath>
#include <vector>

static double get_prob(
    const std::vector<std::vector<std::pair<int, double>>>& adj,
    int prev,
    int next) {
  for (const auto& kv : adj[prev]) {
    if (kv.first == next) return kv.second;
  }
  return 0.0;
}

static void expect_close(double a, double b, double eps) {
  assert(std::fabs(a - b) <= eps);
}

int main() {
  // per pdf: 4-neighbor random walk, 0.25 per direction
  int rows = 3;
  int cols = 3;
  std::vector<double> occ(rows * cols, 0.0);

  // boundary test: top-left corner
  auto adj = build_transition_adj(rows, cols, occ, false);
  int s = 0; // (0,0)
  expect_close(get_prob(adj, s, s), 0.50, 1e-12); // up+left stay
  expect_close(get_prob(adj, s, 1), 0.25, 1e-12); // right
  expect_close(get_prob(adj, s, 3), 0.25, 1e-12); // down

  // blocked-cell test: right neighbor fully blocked
  occ.assign(rows * cols, 0.0);
  occ[1] = 1.0; // cell (1,0) occupied
  auto adj_block = build_transition_adj(rows, cols, occ, true);
  expect_close(get_prob(adj_block, s, s), 0.75, 1e-12); // up+left+blocked right
  expect_close(get_prob(adj_block, s, 1), 0.0, 1e-12);  // no move into block
  expect_close(get_prob(adj_block, s, 3), 0.25, 1e-12); // down ok

  return 0;
}
