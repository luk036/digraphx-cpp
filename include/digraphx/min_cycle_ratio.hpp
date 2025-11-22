#pragma once

#include <algorithm>

#include "parametric.hpp"  // import max_parametric

/**
 * @brief CycleRatioAPI
 *
 * The `CycleRatioAPI` class is a template class that provides an interface for
 * calculating distances and performing zero cancellation on cycles in a
 * directed graph. It takes two template parameters: `DiGraph`, which represents
 * the directed graph type, and `Ratio`, which represents the ratio type used
 * for calculations.
 *
 * Example of cycle ratio calculation:
 *
 * ```svgbob
 *    a ----2----> b
 *    |           |
 *    |           |
 *   1|           |3
 *    |           |
 *    v     4     v
 *    c ---------> d
 *
 *    For a cycle a->b->d->c->a with costs [2, 3, 4, 1] and times [1, 1, 1, 1]:
 *    ratio = (2 + 3 + 4 + 1) / (1 + 1 + 1 + 1) = 10 / 4 = 2.5
 * ```
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
    // named `gra` of type `DiGraph`. This variable  is used to store a
    // reference to an object of type `DiGraph`. The `const` qualifier indicates
    // that the reference is   constant, meaning that the object it refers to
    // cannot be modified through this reference.

  public:
    /**
     * @brief Construct a new Cycle Ratio API object
     *
     * The `CycleRatioAPI` class constructor takes a reference to a `DiGraph` object and initializes
     * its `gra` member variable.
     *
     * @param[in] gra The `gra` parameter is a reference to a `DiGraph` object. It is used to
     * initialize the `gra` member variable of the `CycleRatioAPI` class. The `gra` member variable
     * is a constant reference to a `DiGraph` object, which means it cannot be modified
     */
    explicit CycleRatioAPI(const DiGraph &gra) : gra(gra) {}

    /**
     * @brief distance between two end points of an edge
     *
     * The `distance` function calculates the distance between two vertices in a graph based on the
     * cost and time values associated with the edge connecting them.
     *
     * @param[in] ratio A reference to a `Ratio` object named `ratio`.
     * @param[in] edge The `edge` parameter is a constant reference to an `Edge` object. It
     * represents an edge in a graph connecting two vertices.
     *
     * @return a `Ratio` object.
     */
    auto distance(Ratio &ratio, const Edge &edge) const -> Ratio {
        return Ratio(edge.at("cost")) - ratio * edge.at("time");
    }

    /**
     * The `zero_cancel` function calculates the ratio of the total cost to the total time for a
     * given cycle.
     *
     * @param[in] cycle The `cycle` parameter is of type `Cycle`, which is likely a container or
     * data structure that represents a cycle in a graph. It is used to calculate the ratio of the
     * total cost to the total time for the given cycle.
     *
     * @return The `zero_cancel` function returns the ratio of the total cost to the total time for
     * a given cycle.
     */
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
     * This function constructs a new MinCycleRatioSolver object with a given DiGraph.
     *
     * @param[in] gra The parameter "gra" is of type DiGraph, which is a directed graph. It is used
     * to represent the graph on which the Min Cycle Ratio Solver operates.
     */
    explicit MinCycleRatioSolver(const DiGraph &gra) : gra(gra) {}

    /**
     * @brief run
     *
     * @tparam Mapping
     * @param[in] dist
     * @param[in] r0
     * @param[in] dummy
     * @return Cycle
     */
    template <typename Mapping, typename Domain> auto run(Ratio &r0, Mapping &dist, Domain dummy)
        -> Cycle {
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
 * @tparam DiGraph The type of the directed graph.
 * @tparam Ratio The type representing a ratio or a fraction.
 * @tparam Fn1 The type of the function to get the cost of an edge.
 * @tparam Fn2 The type of the function to get the time of an edge.
 * @tparam Mapping The type of the mapping from vertices to their distances.
 * @tparam Domain The type of the domain for distances.
 * @param[in] gra
 * @param[in,out] r0
 * @param[in] get_cost
 * @param[in] get_time
 * @param[in,out] dist
 * @param[in] dummy A placeholder parameter to deduce the `Domain` type.
 * @return auto
 */
template <typename DiGraph, typename Ratio, typename Fn1, typename Fn2, typename Mapping,
          typename Domain>
auto min_cycle_ratio(const DiGraph &gra, Ratio &r0, Fn1 &&get_cost, Fn2 &&get_time, Mapping &dist,
                     Domain dummy) {
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
        for (const auto &edge : cycle) {
            total_cost += get_cost(edge);
            total_time += get_time(edge);
        }
        return Ratio(std::move(total_cost)) / std::move(total_time);
    };

    auto calc_weight = [&get_cost, &get_time](const Ratio &ratio, const Edge &edge) -> Ratio {
        return get_cost(edge) - ratio * get_time(edge);
    };

    return max_parametric(gra, r0, std::move(calc_weight), std::move(calc_ratio), dist, dummy);
}
