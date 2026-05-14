// basic test: emdw bp runs without segfault (objective 2, mini_project.pdf)

#include "emdw.hpp"
#include "discretetable.hpp"
#include "clustergraph.hpp"
#include "lbp_cg.hpp"
#include "messagequeue.hpp"

#include <cassert>
#include <vector>

using namespace emdw;

int main() {
  typedef int T;
  typedef DiscreteTable<T> DT;
  double defProb = 0.0;

  rcptr<std::vector<T>> binDom(new std::vector<T>{0, 1});

  // p(x)
  rcptr<Factor> pX = uniqptr<DT>(new DT(
      {RVIdType(0)}, {binDom}, defProb, {{{0}, 0.6}, {{1}, 0.4}}));

  // p(y|x)
  rcptr<Factor> pYgX = uniqptr<DT>(new DT(
      {RVIdType(1), RVIdType(0)}, {binDom, binDom}, defProb,
      {{{0, 0}, 0.9}, {{1, 0}, 0.1}, {{0, 1}, 0.2}, {{1, 1}, 0.8}}));

  std::vector<rcptr<Factor>> factors = {pX, pYgX};

  ClusterGraph cg(ClusterGraph::BETHE, factors, {});
  std::map<Idx2, rcptr<Factor>> msgs;
  MessageQueue msgQ;
  unsigned nMsgs = loopyBP_CG(cg, msgs, msgQ, 0.0);
  if (nMsgs == 0) return 1;

  rcptr<Factor> bel = queryLBP_CG(cg, msgs, {RVIdType(0)})->normalize();
  const DT* dt = dynamic_cast<const DT*>(bel.get());
  assert(dt != nullptr);
  double p0 = dt->potentialAt({RVIdType(0)}, {T(0)});
  double p1 = dt->potentialAt({RVIdType(0)}, {T(1)});
  if (p0 <= 0.0 || p1 <= 0.0) return 1;

  return 0;
}
