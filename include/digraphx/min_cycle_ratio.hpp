#pragma once

#include <algorithm>

#include "parametric.hpp"  // import max_parametric

/*!
 * @file min_cycle_ratio.hpp
 * @brief Minimum cycle ratio algorithms for directed graphs
 *
 * This module provides algorithms for finding the minimum cycle ratio in
 * directed graphs, which is a fundamental problem in graph theory with
 * applications in performance analysis, scheduling, and discrete event systems.
 *
 * The cycle ratio of a cycle is defined as:
 * ```
 * ratio(cycle) = sum(costs) / sum(times)
 * ```
 *
 * Problem formulation:
 * ```
 *  min  ratio(cycle)
 *  s.t. cycle is a directed cycle in G(V, E)
 * ```
 *
 * Key applications:
 * - Performance analysis of digital circuits
 * - Scheduling and resource allocation
 * - Network flow optimization
 * - Timing analysis in real-time systems
 * - Economic equilibrium problems
 *
 * Example cycle ratio calculation:
 * ```
 *    a ----2----> b
 *    |           |
 *   1|           |3
 *    |           |
 *    v     4     v
 *    c ---------> d
 *
 *    For cycle a->b->d->c->a with costs [2, 3, 4, 1] and times [1, 1, 1, 1]:
 *    ratio = (2 + 3 + 4 + 1) / (1 + 1 + 1 + 1) = 10 / 4 = 2.5
 * ```
 *
 * Algorithm approach:
 * - Transforms to parametric problem: max r s.t. cost(e) - r * time(e) >= 0
 * - Uses Howard's method for negative cycle detection
 * - Iteratively adjusts parameter r until convergence
 *
 * Performance characteristics:
 * - Time complexity: O(V * E * log(C)) where C is the ratio range
 * - Space complexity: O(V + E)
 * - Efficient for sparse graphs with tight cycles
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing the ratio/cost values
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

/*!
 * @brief Minimum Cycle Ratio Solver
 *
 * This class provides algorithms for solving the minimum cycle ratio (MCR) problem
 * in directed graphs. The MCR problem seeks to find the cycle with the minimum
 * ratio of total cost to total time, which is crucial for analyzing performance
 * characteristics of discrete event systems.
 *
 * Problem definition:
 * ```
 *  min  (sum(costs) / sum(times))
 *  s.t. cycle is a directed cycle in G(V, E)
 * ```
 *
 * Algorithm approach:
 * The solver transforms the MCR problem into a parametric problem:
 * ```
 *  max  r
 *  s.t. dist[v] - dist[u] >= cost(u,v) - r * time(u,v)
 *       for all edges (u,v) in G
 * ```
 *
 * Key insights:
 * - A cycle has ratio ≤ r iff cost - r*time has negative cycle
 * - Binary search on r with negative cycle detection
 * - Uses Howard's method for efficient cycle detection
 * - Converges to optimal minimum cycle ratio
 *
 * Applications:
 * - Digital circuit performance analysis
 * - Real-time system scheduling
 * - Communication network optimization
 * - Manufacturing system analysis
 * - Economic equilibrium computation
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing ratio values (typically double)
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
 * @brief Free function for minimum cost-to-time cycle ratio problem
 *
 * This function provides a functional interface for solving the minimum cycle
 * ratio problem without requiring explicit class instantiation. It solves the
 * parametric network problem:
 *
 * ```
 *     max  r
 *     s.t. dist[vtx] - dist[utx] ≥ cost(utx, vtx) - r * time(utx, vtx)
 *          ∀ edge(utx, vtx) ∈ G(V, E)
 * ```
 *
 * The function uses the same algorithmic approach as MinCycleRatioSolver but
 * with a more flexible callback-based interface. This is particularly useful
 * when:
 * - Edge data is stored in custom formats
 * - Cost and time extraction requires complex logic
 * - You prefer functional programming style
 * - Integration with existing callback-based code
 *
 * Algorithm details:
 * 1. Transforms MCR to parametric problem
 * 2. Uses binary search on parameter r
 * 3. Employs Howard's method for negative cycle detection
 * 4. Iteratively refines the minimum ratio estimate
 *
 * Usage example:
 * ```cpp
 * // Define cost and time extraction functions
 * auto get_cost = [](const Edge& e) { return e["cost"]; };
 * auto get_time = [](const Edge& e) { return e["time"]; };
 *
 * // Initialize distances
 * std::unordered_map<Node, double> dist;
 * for (const auto& [node, _] : graph) dist[node] = 0.0;
 *
 * // Find minimum cycle ratio
 * double ratio = 0.0;  // initial estimate
 * auto cycle = min_cycle_ratio(graph, ratio, get_cost, get_time, dist, 0.0);
 * ```
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing ratio values
 * @tparam Fn1 Type of cost extraction function
 * @tparam Fn2 Type of time extraction function
 * @tparam Mapping Type of distance mapping (node -> distance)
 * @tparam Domain Type of the domain for distance calculations
 * @param[in] gra The directed graph to analyze
 * @param[in,out] r0 Initial and final minimum ratio estimate
 * @param[in] get_cost Function to extract cost from an edge
 * @param[in] get_time Function to extract time from an edge
 * @param[in,out] dist Distance mapping updated during execution
 * @param[in] dummy Parameter for type deduction
 * @return Cycle The cycle achieving the minimum ratio
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
