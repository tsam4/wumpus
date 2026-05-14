// advanced test: dataset2 scale + timing (per mini_project.pdf)

#include "wumpus_model.hpp"

#include "emdw.hpp"
#include "discretetable.hpp"
#include "clustergraph.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <sys/resource.h>
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

int main(int argc, char** argv) {
  // per pdf: dataset2 is 20x20, 20 steps, pw=0.9 pc=0.1
  std::string dataset_dir = (argc > 1) ? argv[1] : "Datasets-20260506/dataset2";
  auto obs = load_dataset(dataset_dir);
  assert(!obs.empty());
  assert(obs[0].rows == 20 && obs[0].cols == 20);
  assert((int)obs.size() == 20);

  double pw = 0.9;
  double pc = 0.1;

  int rows = obs[0].rows;
  int cols = obs[0].cols;
  int n = rows * cols;
  int t_steps = (int)obs.size();

  rcptr<std::vector<int>> state_dom(new std::vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  std::vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  auto adj = build_transition_adj(rows, cols);

  std::vector<rcptr<Factor>> factors;
  std::vector<double> prior(n, 1.0);
  factors.push_back(make_unary(x_ids[0], state_dom, prior));

  std::vector<std::vector<double>> emis_log;
  emis_log.reserve(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    auto logp = compute_emission_log(obs[t], pw, pc);
    auto probs = scale_from_log(logp);
    factors.push_back(make_unary(x_ids[t], state_dom, probs));
    emis_log.push_back(logp);
  }

  for (int t = 1; t < t_steps; ++t) {
    factors.push_back(make_trans(x_ids[t], x_ids[t - 1], state_dom, adj));
  }

  auto t0 = std::chrono::steady_clock::now();
  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  std::map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  loopyBP_CG(cg, msgs, msgQ, 0.0);
  auto t1 = std::chrono::steady_clock::now();

  std::vector<double> prior_log(n, 0.0);
  for (int s = 0; s < n; ++s) prior_log[s] = 0.0;
  auto map_path = viterbi_path(adj, emis_log, prior_log);
  auto t2 = std::chrono::steady_clock::now();

  assert((int)map_path.size() == t_steps);

  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  auto bp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  auto map_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

  // print timing + mem for report
  std::cout << "dataset2 bp_ms=" << bp_ms << " map_ms=" << map_ms
            << " maxrss=" << usage.ru_maxrss << "\n";

  return 0;
}
