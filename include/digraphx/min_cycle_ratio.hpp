// -*- coding: utf-8 -*-
#pragma once

#include <algorithm>
#include <numeric>

#include "parametric.hpp" // import max_parametric

/*!
 * @brief minimum cost-to-time cycle ratio problem
 *
 *    This function solves the following network parametric problem:
 *
 *        max  r
 *        s.t. dist[vtx] - dist[utx] \ge cost(utx, vtx) - r * time(utx, vtx)
 *             \forall edge(utx, vtx) \in gra(V, E)
 *
 * @tparam Graph
 * @tparam Fn1
 * @tparam Fn2
 * @tparam Mapping
 * @param[in] gra
 * @param[in,out] r0
 * @param[in] get_cost
 * @param[in] get_time
 * @param[in,out] dist
 * @return auto
 */
template <typename DiGraph, typename T, typename Fn1, typename Fn2,
          typename Mapping>
auto min_cycle_ratio(const DiGraph &gra, T &r0, Fn1 &&get_cost, Fn2 &&get_time,
                     Mapping &&dist, size_t max_iters = 1000) {
  // using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
  // using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  // using Cycle = std::vector<std::pair<Edge, std::pair<Node, Node>>>;
  using cost_T = decltype(get_cost(std::declval<Edge>()));
  using time_T = decltype(get_time(std::declval<Edge>()));

  auto calc_ratio = [&](const auto &C) -> T {
    auto total_cost = cost_T(0);
    auto total_time = time_T(0);
    for (auto &&edge : C) {
      total_cost += get_cost(edge);
      total_time += get_time(edge);
    }
    return T(total_cost) / total_time;
  };

  auto calc_weight = [&](const T &r, const Edge &edge) -> T {
    return get_cost(edge) - r * get_time(edge);
  };

  return max_parametric(gra, r0, std::move(calc_weight), std::move(calc_ratio),
                        std::forward<Mapping>(dist), max_iters);
}
