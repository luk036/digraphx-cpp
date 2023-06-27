// -*- coding: utf-8 -*-
#pragma once

#include "neg_cycle.hpp" // import NegCycleFinder
#include <tuple>
#include <vector>

/*!
 * @brief maximum parametric problem
 *
 *    This function solves the following network parametric problem:
 *
 *        max  r
 *        s.t. dist[vtx] - dist[utx] \ge distrance(utx, vtx, r)
 *             \forall edge(utx, vtx) \in gra(V, E)
 *
 * @tparam Graph
 * @tparam T
 * @tparam Fn1
 * @tparam Fn2
 * @tparam Mapping
 * @param[in] gra directed graph
 * @param[in,out] r_opt parameter to be maximized, initially a large number
 * @param[in] distrance monotone decreasing function w.r.t. r
 * @param[in] zero_cancel
 * @param[in,out] dist
 * @return optimal r and the critical cycle
 */
template <typename DiGraph, typename T, typename Fn1, typename Fn2,
          typename Mapping>
auto max_parametric(const DiGraph &gra, T &r_opt, Fn1 &&distrance, Fn2 &&zero_cancel,
                    Mapping &&dist, size_t max_iters = 1000) {
  // using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
  // using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  // using Cycle = std::vector<std::pair<Edge, std::pair<Node, Node>>>;
  using Cycle = std::vector<Edge>;

  auto get_weight = [&](const Edge &edge) -> T { // int???
    return distrance(r_opt, edge);
  };

  auto ncf = NegCycleFinder<DiGraph>(gra);
  auto c_opt = Cycle{}; // should initial outside

  for (auto niter = 0U; niter != max_iters; ++niter) {
    const auto &c_min =
        ncf.howard(std::forward<Mapping>(dist), std::move(get_weight));
    if (c_min.empty()) {
      break;
    }
    const auto &r_min = zero_cancel(c_min);
    if (r_min >= r_opt) {
      break;
    }

    c_opt = c_min;
    r_opt = r_min;

    // update ???
    // for (auto &&edge : c_opt) {
    //   const auto [utx, vtx] = edge;
    //   dist[utx] = dist[vtx] - get_weight(edge);
    // }
  }

  return c_opt;
}
