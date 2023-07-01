#pragma once

#include "neg_cycle.hpp" // import NegCycleFinder
#include <tuple>
#include <vector>

/**
 * @brief This function solves the following network parametric problem:
 *
 *  max  r
 *  s.t. dist[v] - dist[u] <= distrance(e, r)
 *       \forall e(u, v) \in gra(V, E)
 * 
 * @tparam DiGraph 
 * @tparam ParametricAPI 
 */
template <typename DiGraph, typename ParametricAPI> class MaxParametricSolver {
public:
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;

private:
  // const DiGraph &_gra;
  NegCycleFinder<DiGraph> _ncf;
  ParametricAPI &_omega;

public:
  /**
   * @brief Construct a new Max Parametric Solver object
   * 
   * @param gra 
   * @param omega 
   */
  MaxParametricSolver(const DiGraph &gra, ParametricAPI &omega)
      :  _ncf{gra}, _omega{omega} {}

  /**
   * @brief Run 
   * 
   * @tparam Ratio 
   * @tparam Mapping 
   * @tparam Domain 
   */
  template <typename Ratio, typename Mapping, typename Domain>
  auto run(Ratio &r_opt, Mapping &dist, Domain /* dist type */) {
    auto get_weight = [this, &r_opt](const Edge &edge) -> Domain { // note!!!
      return Domain(this->_omega.distance(r_opt, edge));
    };

    auto r_min = r_opt;
    auto c_min = Cycle{};
    auto c_opt = Cycle{};

    while (true) {
      for (auto ci : this->_ncf.howard(dist, std::move(get_weight))) {
        auto ri = zero_cancel(ci);
        if (r_min > ri) {
          r_min = ri;
          c_min = ci;
        }
      }
      if (r_min >= r_opt) {
        break;
      }

      c_opt = c_min;
      r_opt = r_min;
    }

    return c_opt;
  }
};

/*!
 * @brief This function solves the following network parametric problem:
 *
 *  max  r
 *  s.t. dist[v] - dist[u] <= distrance(e, r)
 *       \forall e(u, v) \in gra(V, E)
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
                    Fn2 &&zero_cancel, Mapping &dist, D /* dist type*/) {
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;

  auto get_weight = [&distance, &r_opt](const Edge &edge) -> D { // note!!!
    return static_cast<D>(distance(r_opt, edge));
  };

  auto ncf = NegCycleFinder<DiGraph>(gra);
  auto r_min = r_opt;
  auto c_min = Cycle{};
  auto c_opt = Cycle{}; // should initial outside

  while (true) {
    for (auto ci : ncf.howard(dist, std::move(get_weight))) {
      auto ri = zero_cancel(ci);
      if (r_min > ri) {
        r_min = ri;
        c_min = ci;
      }
    }
    if (r_min >= r_opt) {
      break;
    }

    c_opt = c_min;
    r_opt = r_min;
  }

  return c_opt;
}
