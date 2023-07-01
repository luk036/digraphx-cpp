// -*- coding: utf-8 -*-
/* The line `// -*- coding: utf-8 -*-` is a special comment that specifies the
encoding of the source code file. In this case, it indicates that the file is
encoded using UTF-8. This is useful for ensuring that the file is interpreted
correctly by the compiler or interpreter. */

#pragma once

#include <algorithm>
#include <numeric>

#include "parametric.hpp" // import max_parametric

/**
 * @brief CycleRatioAPI
 *
 * The `CycleRatioAPI` class is a template class that provides an interface for
 * calculating distances and performing zero cancellation on cycles in a
 * directed graph. It takes two template parameters: `DiGraph`, which represents
 * the directed graph type, and `Ratio`, which represents the ratio type used
 * for calculations.
 *
 * @tparam DiGraph
 * @tparam Ratio
 */
template <typename DiGraph, typename Ratio> class CycleRatioAPI {
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;

  const DiGraph &gra;
  // The line `const DiGraph& gra;` is declaring a constant reference variable
  // named `gra` of type `DiGraph`. This variable  is used to store a reference
  // to an object of type `DiGraph`. The `const` qualifier indicates that the
  // reference is   constant, meaning that the object it refers to cannot be
  // modified through this reference.

public:
  /**
   * @brief Construct a new Cycle Ratio API object
   *
   * The `CycleRatioAPI` class has a constructor that takes a reference to a
   * `DiGraph` object as a parameter. This constructor is used to create a new
   * `CycleRatioAPI` object and initialize its `gra` member variable with the
   * provided `DiGraph` object. The `gra` member variable is a constant reference to
   * a `DiGraph` object, which means it cannot be modified after it is initialized.
   *
   * @param gra
   */
  CycleRatioAPI(const DiGraph &gra) : gra(gra) {}

  /**
   * @brief
   *
   * @param ratio
   * @param edge
   * @return Ratio
   */
  auto distance(Ratio &ratio, const Edge &edge) const -> Ratio {
    return Ratio(edge.at("cost")) - ratio * edge.at("time");
  }

  auto zero_cancel(const Cycle &cycle) const -> Ratio {
    Ratio total_cost = 0;
    Ratio total_time = 0;
    for (const auto &edge : cycle) {
      total_cost += edge.at("cost");
      total_time += edge.at("time");
    }
    return Ratio(total_cost) / total_time;
  }
};

/**
 * @brief MinCycleRatioSolver implementation
 *
 * @tparam DiGraph
 * @tparam Ratio
 */
template <typename DiGraph, typename Ratio> class MinCycleRatioSolver {
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;

  const DiGraph &gra;

public:
  /**
   * @brief Construct a new Min Cycle Ratio Solver object
   *
   * @param gra
   */
  MinCycleRatioSolver(const DiGraph &gra) : gra(gra) {}

  /**
   * @brief run
   *
   * @tparam Mapping
   * @param dist
   * @param r0
   * @return Cycle
   */
  template <typename Mapping, typename Domain>
  auto run(Ratio &r0, Mapping &dist, Domain dummy) -> Cycle {
    auto omega = CycleRatioAPI<DiGraph, Ratio>(gra);
    auto solver = MaxParametricSolver(gra, omega);
    return solver.run(dist, r0, std::move(dummy));
  }
};

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
template <typename DiGraph, typename Ratio, typename Fn1, typename Fn2,
          typename Mapping, typename Domain>
auto min_cycle_ratio(const DiGraph &gra, Ratio &r0, Fn1 &&get_cost,
                     Fn2 &&get_time, Mapping &dist, Domain dummy) {
  using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
  using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
  using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
  using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
  using Cycle = std::vector<Edge>;
  using cost_T = decltype(get_cost(std::declval<Edge>()));
  using time_T = decltype(get_time(std::declval<Edge>()));

  auto calc_ratio = [&get_cost, &get_time](const Cycle &cycle) -> Ratio {
    auto total_cost = cost_T(0);
    auto total_time = time_T(0);
    for (auto &&edge : cycle) {
      total_cost += get_cost(edge);
      total_time += get_time(edge);
    }
    return static_cast<Ratio>(std::move(total_cost)) / std::move(total_time);
  };

  auto calc_weight = [&get_cost, &get_time](Ratio &ratio,
                                            const Edge &edge) -> Ratio {
    return get_cost(edge) - ratio * get_time(edge);
  };

  return max_parametric(gra, r0, std::move(calc_weight), std::move(calc_ratio),
                        dist, dummy);
}
