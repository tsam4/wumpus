// advanced test: EM extremes for pw/pc (datasets 3 and 5, mini_project.pdf)

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

static void run_em(const std::vector<Grid>& obs, double* pw, double* pc, int iters) {
  int rows = obs[0].rows;
  int cols = obs[0].cols;
  int n = rows * cols;
  int t_steps = (int)obs.size();

  rcptr<std::vector<int>> state_dom(new std::vector<int>(n));
  for (int i = 0; i < n; ++i) (*state_dom)[i] = i;
  std::vector<RVIdType> x_ids(t_steps);
  for (int t = 0; t < t_steps; ++t) x_ids[t] = RVIdType(t);

  std::vector<double> occ(n, 0.0);
  auto adj = build_transition_adj(rows, cols, occ, false);

  std::vector<std::vector<double>> gamma;

  for (int iter = 0; iter < iters; ++iter) {
    std::vector<rcptr<Factor>> factors;
    std::vector<double> prior(n, 1.0);
    factors.push_back(make_unary(x_ids[0], state_dom, prior));

    for (int t = 0; t < t_steps; ++t) {
      auto logp = compute_emission_log(obs[t], *pw, *pc, occ, false);
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

    gamma.assign(t_steps, std::vector<double>(n, 0.0));
    for (int t = 0; t < t_steps; ++t) {
      rcptr<Factor> bel = queryLBP_CG(cg, msgs, {x_ids[t]})->normalize();
      const DiscreteTable<int>* dt = dynamic_cast<const DiscreteTable<int>*>(bel.get());
      assert(dt != nullptr);
      for (int s = 0; s < n; ++s) {
        gamma[t][s] = dt->potentialAt({x_ids[t]}, {s});
      }
    }

    // m-step pw/pc
    double true_det = 0.0;
    double clutter_det = 0.0;
    for (int t = 0; t < t_steps; ++t) {
      int total_det = 0;
      for (int k = 0; k < n; ++k) total_det += obs[t].cells[k];
      double exp_true_det = 0.0;
      for (int s = 0; s < n; ++s) {
        if (obs[t].cells[s] == 1) exp_true_det += gamma[t][s];
      }
      true_det += exp_true_det;
      clutter_det += (double)total_det - exp_true_det;
    }

    double pw_new = true_det / (double)t_steps;
    double pc_new = clutter_det / (double)(t_steps * (n - 1));
    *pw = clamp_prob(pw_new);
    *pc = clamp_prob(pc_new);
  }
}

int main() {
  // tiny grid, adversarial obs
  int rows = 3;
  int cols = 3;
  int n = rows * cols;
  int t_steps = 5;

  std::vector<Grid> all_ones(t_steps);
  std::vector<Grid> all_zeros(t_steps);
  for (int t = 0; t < t_steps; ++t) {
    all_ones[t].rows = rows;
    all_ones[t].cols = cols;
    all_ones[t].cells.assign(n, 1);

    all_zeros[t].rows = rows;
    all_zeros[t].cols = cols;
    all_zeros[t].cells.assign(n, 0);
  }

  double pw = 0.9;
  double pc = 0.1;
  run_em(all_ones, &pw, &pc, 3);
  assert(pw > 0.0 && pw < 1.0);
  assert(pc > 0.0 && pc < 1.0);

  pw = 0.9;
  pc = 0.1;
  run_em(all_zeros, &pw, &pc, 3);
  assert(pw > 0.0 && pw < 1.0);
  assert(pc > 0.0 && pc < 1.0);

  return 0;
}
