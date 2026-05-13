// advanced test: end-to-end sim + accuracy (objective 4, mini_project.pdf)

#include "wumpus_model.hpp"

#include "emdw.hpp"
#include "discretetable.hpp"
#include "clustergraph.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

#include <cassert>
#include <map>
#include <vector>

using namespace emdw;

static rcptr<Factor> make_unary(
    RVIdType var,
    const rcptr<std::vector<int>>& dom,
    const std::vector<double>& probs) {
  std::map<std::vector<int>, FProb> sparse;
  for (int i = 0; i < (int)probs.size(); ++i) {
    if (probs[i] <= 0.0) continue;
    sparse[{i}] = probs[i];
  }
  return uniqptr<DiscreteTable<int>>(new DiscreteTable<int>({var}, {dom}, 0.0, sparse));
}

static rcptr<Factor> make_trans(
    RVIdType next_var,
    RVIdType prev_var,
    const rcptr<std::vector<int>>& dom,
    const std::vector<std::vector<std::pair<int, double>>>& adj) {
  std::map<std::vector<int>, FProb> sparse;
  for (int prev = 0; prev < (int)adj.size(); ++prev) {
    for (const auto& kv : adj[prev]) {
      if (kv.second <= 0.0) continue;
      sparse[{kv.first, prev}] = kv.second;
    }
  }
  return uniqptr<DiscreteTable<int>>(
      new DiscreteTable<int>({next_var, prev_var}, {dom, dom}, 0.0, sparse));
}

int main() {
  // simulate dataset1-style grid
  int rows = 5;
  int cols = 5;
  int n = rows * cols;
  int t_steps = 10;

  // ground truth path (simple right moves + clamp at edge)
  std::vector<int> gt(t_steps, 0);
  for (int t = 0; t < t_steps; ++t) {
    int x = std::min(t, cols - 1);
    gt[t] = x; // y=0 row
  }

  // deterministic obs: only true cell detects (no clutter)
  std::vector<Grid> obs(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    obs[t].rows = rows;
    obs[t].cols = cols;
    obs[t].cells.assign(n, 0);
    obs[t].cells[gt[t]] = 1;
  }

  double pw = 0.95; // per pdf dataset1
  double pc = 0.05;

  rcptr<std::vector<int>> state_dom(new std::vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  std::vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  std::vector<double> occ(n, 0.0);
  auto adj = build_transition_adj(rows, cols, occ, false);

  std::vector<rcptr<Factor>> factors;
  std::vector<double> prior(n, 1.0);
  factors.push_back(make_unary(x_ids[0], state_dom, prior));

  std::vector<std::vector<double>> emis_log;
  emis_log.reserve(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    auto logp = compute_emission_log(obs[t], pw, pc, occ, false);
    auto probs = scale_from_log(logp);
    factors.push_back(make_unary(x_ids[t], state_dom, probs));
    emis_log.push_back(logp);
  }
  for (int t = 1; t < t_steps; ++t) {
    factors.push_back(make_trans(x_ids[t], x_ids[t - 1], state_dom, adj));
  }

  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  std::map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  loopyBP_CG(cg, msgs, msgQ, 0.0);

  std::vector<double> prior_log(n, 0.0);
  auto map_path = viterbi_path(adj, emis_log, prior_log);
  assert((int)map_path.size() == t_steps);

  // accuracy metric (objective 4)
  int correct = 0;
  for (int t = 0; t < t_steps; ++t) {
    if (map_path[t] == gt[t]) correct++;
  }
  double acc = (double)correct / (double)t_steps;
  if (acc < 0.8) return 1;

  return 0;
}
