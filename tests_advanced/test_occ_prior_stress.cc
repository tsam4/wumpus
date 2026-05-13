// advanced test: occupied-cell prior stress (datasets 4 and 5, mini_project.pdf)

#include "wumpus_model.hpp"

#include "emdw.hpp"
#include "discretetable.hpp"
#include "clustergraph.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

#include <cassert>
#include <cmath>
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
  // small grid with heavy blocks
  int rows = 4;
  int cols = 4;
  int n = rows * cols;
  int t_steps = 6;

  std::vector<Grid> obs(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    obs[t].rows = rows;
    obs[t].cols = cols;
    obs[t].cells.assign(n, 0); // all zeros
  }

  // dataset4 prior per pdf
  double occ_prior = 0.25;
  std::vector<double> occ_probs(n, 0.9); // heavy blocking

  rcptr<std::vector<int>> state_dom(new std::vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  std::vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  auto adj = build_transition_adj(rows, cols, occ_probs, true);

  // check transition normalization
  for (int i = 0; i < n; ++i) {
    double sum = 0.0;
    for (const auto& kv : adj[i]) sum += kv.second;
    if (std::fabs(sum - 1.0) >= 1e-9) return 1;
  }

  // run bp with occ in emissions
  double pw = 0.95;
  double pc = 0.05;
  std::vector<rcptr<Factor>> factors;
  std::vector<double> prior(n, 1.0);
  for (int i = 0; i < n; ++i) prior[i] = clamp_prob(1.0 - occ_probs[i]);
  factors.push_back(make_unary(x_ids[0], state_dom, prior));

  for (int t = 0; t < t_steps; ++t) {
    auto logp = compute_emission_log(obs[t], pw, pc, occ_probs, true);
    auto probs = scale_from_log(logp);
    factors.push_back(make_unary(x_ids[t], state_dom, probs));
  }
  for (int t = 1; t < t_steps; ++t) {
    factors.push_back(make_trans(x_ids[t], x_ids[t - 1], state_dom, adj));
  }

  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  std::map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  loopyBP_CG(cg, msgs, msgQ, 0.0);

  // update occ probs once (as in main code)
  auto nbrs = build_neighbors(rows, cols);
  std::vector<std::vector<double>> gamma(t_steps, std::vector<double>(n, 0.0));
  for (int t = 0; t < t_steps; ++t) {
    rcptr<Factor> bel = queryLBP_CG(cg, msgs, {x_ids[t]})->normalize();
    const DiscreteTable<int>* dt = dynamic_cast<const DiscreteTable<int>*>(bel.get());
    assert(dt != nullptr);
    for (int s = 0; s < n; ++s) gamma[t][s] = dt->potentialAt({x_ids[t]}, {s});
  }

  std::vector<double> attempts(n, 0.0);
  std::vector<double> success(n, 0.0);
  for (int t = 1; t < t_steps; ++t) {
    rcptr<Factor> pair = queryLBP_CG(cg, msgs, {x_ids[t], x_ids[t - 1]})->normalize();
    const DiscreteTable<int>* dt = dynamic_cast<const DiscreteTable<int>*>(pair.get());
    assert(dt != nullptr);
    for (int prev = 0; prev < n; ++prev) {
      double p_prev = gamma[t - 1][prev];
      for (int nb : nbrs[prev]) {
        attempts[nb] += 0.25 * p_prev;
        double p_pair = dt->potentialAt({x_ids[t], x_ids[t - 1]}, {nb, prev});
        success[nb] += p_pair;
      }
    }
  }

  double prior_open = clamp_prob(1.0 - occ_prior);
  double prior_occ = clamp_prob(occ_prior);
  double strength = 2.0;
  double a = strength * prior_open;
  double b = strength * prior_occ;

  for (int j = 0; j < n; ++j) {
    if (attempts[j] <= 1e-9) {
      occ_probs[j] = occ_prior;
      continue;
    }
    double p_open = (success[j] + a) / (attempts[j] + a + b);
    occ_probs[j] = clamp_prob(1.0 - p_open);
    assert(occ_probs[j] > 0.0 && occ_probs[j] < 1.0);
  }

  // ensure update moves at least one cell away from the initial 0.9
  int changed = 0;
  for (double p : occ_probs) if (std::fabs(p - 0.9) > 1e-6) changed++;
  if (changed == 0) return 1;

  return 0;
}
