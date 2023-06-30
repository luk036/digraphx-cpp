// -*- coding: utf-8 -*-
#pragma once

#include "neg_cycle.hpp" // import NegCycleFinder
#include <tuple>
#include <vector>

template <typename DiGraph, typename ParametricAPI> class MaxParametricSolver {
public:
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;

private:
  const DiGraph &_gra;
  ParametricAPI &_omega;
  NegCycleFinder<DiGraph> &_ncf;

public:
  MaxParametricSolver(const DiGraph &gra, ParametricAPI &omega)
      : _gra{gra}, _omega{omega}, _ncf{gra} {}

  template <typename R, typename Mapping, typename D>
  auto run(R &r_opt, Mapping &&dist, D && /* dist type */) {
    // using D1 = decltype(*(dist.begin()).second);
    // using D = std::remove_cv_t<std::remove_reference_t<D1>>;

    auto get_weight = [&](const Edge &edge) -> D { // note!!!
      return static_cast<D>(this->_omega.distance(r_opt, edge));
    };

    auto r_min = r_opt;
    auto c_min = Cycle{};
    auto c_opt = Cycle{}; // should initial outside

    while (true) {
      for (auto ci :
           this->_ncf.howard(std::move(dist), std::move(get_weight))) {
        auto ri = zero_cancel(ci);
        if (r_min > ri) {
          r_min = ri;
          c_min = ci;
        }
      }
      if (r_min >= r_opt) {
        break;
      }

      c_opt = std::move(c_min);
      r_opt = std::move(r_min);
    }

    return c_opt;
  }
};

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
          typename Mapping, typename D>
auto max_parametric(const DiGraph &gra, T &r_opt, Fn1 &&distance,
                    Fn2 &&zero_cancel, Mapping &&dist, D /* dist type*/) {
  // using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
  // using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;
  // using D1 = decltype(*(dist.begin()).second);
  // using D = std::remove_cv_t<std::remove_reference_t<D1>>;

  auto get_weight = [&](const Edge &edge) -> D { // note!!!
    return static_cast<D>(distance(r_opt, edge));
  };

  auto ncf = NegCycleFinder<DiGraph>(gra);
  auto r_min = r_opt;
  auto c_min = Cycle{};
  auto c_opt = Cycle{}; // should initial outside

  while (true) {
    for (auto ci : ncf.howard(std::move(dist), std::move(get_weight))) {
      auto ri = zero_cancel(ci);
      if (r_min > ri) {
        r_min = ri;
        c_min = ci;
      }
    }
    if (r_min >= r_opt) {
      break;
    }

    c_opt = std::move(c_min);
    r_opt = std::move(r_min);
  }

  return c_opt;
}
