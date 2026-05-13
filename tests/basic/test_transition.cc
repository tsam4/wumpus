// test_transition: unit tests for build_transition_adj
//
// Covers (per mini_project.pdf transition model):
// - Corner cell self-loop probability: 2 out-of-bounds => stay = 0.5
// - Every row of the transition matrix sums to 1.0
// - Interior cell has no self-loop; all 4 neighbours at 0.25
// - Top-edge cell has 1 out-of-bounds => stay = 0.25

#include "../../src/wumpus_model.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace std;

static bool approx(double a, double b, double tol = 1e-9) {
  return fabs(a - b) < tol;
}

void test_corner_self_loop() {
  // 3x3 grid, cell 0 = top-left corner
  // out-of-bounds: up (y<0), left (x<0) => 2 fails => stay = 0.5
  auto adj = build_transition_adj(3, 3);
  int cell = 0;
  double self_prob = 0.0;
  int nbr_count = 0;
  for (const auto& kv : adj[cell]) {
    if (kv.first == cell) self_prob += kv.second;
    else ++nbr_count;
  }
  assert(approx(self_prob, 0.5) && "corner self-loop must be 0.5");
  assert(nbr_count == 2 && "corner must have exactly 2 valid neighbours");
  printf("PASS: test_corner_self_loop\n");
}

void test_row_sums() {
  // Every row of P(X_t | X_{t-1}) must sum to 1.0 for a 4x5 grid
  int rows = 4, cols = 5;
  auto adj = build_transition_adj(rows, cols);
  int n = rows * cols;
  for (int s = 0; s < n; ++s) {
    double sum = 0.0;
    for (const auto& kv : adj[s]) sum += kv.second;
    assert(approx(sum, 1.0) && "transition row must sum to 1.0");
  }
  printf("PASS: test_row_sums (4x5 grid)\n");
}

void test_interior_cell() {
  // 3x3 grid, cell 4 = centre: all 4 moves valid, no self-loop
  auto adj = build_transition_adj(3, 3);
  int cell = 4;
  for (const auto& kv : adj[cell]) {
    assert(kv.first != cell && "interior cell must have no self-loop");
    assert(approx(kv.second, 0.25) && "interior cell neighbours must be 0.25");
  }
  assert((int)adj[cell].size() == 4 && "interior cell must have exactly 4 transitions");
  printf("PASS: test_interior_cell\n");
}

void test_edge_cell() {
  // 3x3 grid, cell 1 = top-centre: up out-of-bounds => stay = 0.25
  auto adj = build_transition_adj(3, 3);
  int cell = 1;  // (y=0, x=1)
  double self_prob = 0.0;
  for (const auto& kv : adj[cell]) {
    if (kv.first == cell) self_prob += kv.second;
  }
  assert(approx(self_prob, 0.25) && "top-edge cell stay prob must be 0.25");
  printf("PASS: test_edge_cell\n");
}

int main() {
  test_corner_self_loop();
  test_row_sums();
  test_interior_cell();
  test_edge_cell();
  printf("All transition tests passed.\n");
  return 0;
}
