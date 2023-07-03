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
   * provided `DiGraph` object. The `gra` member variable is a constant
   * reference to a `DiGraph` object, which means it cannot be modified after it
   * is initialized.
   *
   * @param gra
   */
  CycleRatioAPI(const DiGraph &gra) : gra(gra) {}

  /**
   * @brief distance between two end points of an edge
   *
   * The `distance` function takes a reference to a `Ratio` object named `ratio`
   * and a constant reference to an `Edge` object named `edge` as  parameters.
   * It calculates the distance between two vertices in the graph based on the
   * cost and time values associated with the edge  connecting them.
   *
   * @param ratio
   * @param edge
   * @return Ratio
   */
  auto distance(Ratio &ratio, const Edge &edge) const -> Ratio {
    return Ratio(edge.at("cost")) - ratio * edge.at("time");
  }

  /**
   * @brief zero_cancel function
   *
   * The `zero_cancel` function calculates the ratio of the total cost to the
   * total time for a given cycle.
   *
   * @param cycle
   * @return Ratio
   */
  auto zero_cancel(const Cycle &cycle) const -> Ratio {
    Ratio total_cost = 0;
    Ratio total_time = 0;
    for (const auto &edge : cycle) {
      total_cost += edge.at("cost");
      total_time += edge.at("time");
    }
    // Ratio total_cost =
    //     std::accumulate(cycle.begin(), cycle.end(), Ratio(0),
    //                     [](const Edge &edge) { return edge.at("cost"); });
    // Ratio total_time =
    //     std::accumulate(cycle.begin(), cycle.end(), Ratio(0),
    //                     [](const Edge &edge) { return edge.at("time"); });

    return Ratio(total_cost) / total_time;
  }
};

/**
 * @brief Minimum Cycle Ratio Solver
 *
 * The minimum cycle ratio (MCR) problem is a fundamental problem in the
 * analysis of directed graphs. Given a directed graph, the MCR problem seeks to
 * find the cycle with the minimum ratio of the sum of edge weights to the
 * number of edges in the cycle. In other words, the MCR problem seeks to find
 * the "tightest" cycle in the graph, where the tightness of a cycle is measured
 * by the ratio of the total weight of the cycle to its length.
 *
 * The MCR problem has many applications in the analysis of discrete event
 * systems, such as digital circuits and communication networks. It is closely
 * related to other problems in graph theory, such as the shortest path problem
 * and the maximum flow problem. Efficient algorithms for solving the MCR
 * problem are therefore of great practical importance.
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

  auto calc_ratio = [&](const Cycle &cycle) -> Ratio {
    auto total_cost = cost_T(0);
    auto total_time = time_T(0);
    for (auto &&edge : cycle) {
      total_cost += get_cost(edge);
      total_time += get_time(edge);
    }
    // cost_T total_cost = std::accumulate(
    //     cycle.cbegin(), cycle.cend(), cost_T(0),
    //     [&get_cost](const Edge &edge) { return get_cost(edge); });
    // time_T total_time = std::accumulate(
    //     cycle.cbegin(), cycle.cend(), time_T(0),
    //     [&get_time](const Edge &edge) { return get_time(edge); });
    return Ratio(std::move(total_cost)) / std::move(total_time);
  };

  auto calc_weight = [&get_cost, &get_time](Ratio &ratio,
                                            const Edge &edge) -> Ratio {
    return get_cost(edge) - ratio * get_time(edge);
  };

  return max_parametric(gra, r0, std::move(calc_weight), std::move(calc_ratio),
                        dist, dummy);
}
