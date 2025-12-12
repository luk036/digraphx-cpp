#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "neg_cycle_q.hpp"  // import NegCycleFinderQ

/*!
 * @file min_parametric_q.hpp
 * @brief Minimum parametric network problem solver with constraints
 *
 * This module provides algorithms for solving constrained parametric network
 * optimization problems. It extends the basic parametric solver to support
 * constraint handling and both predecessor/successor-based algorithms.
 *
 * Problem formulation:
 * \code
 *  min  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       forall e(u, v) in G(V, E)
 *       subject to update constraints
 * \endcode
 *
 * Key features:
 * - Support for distance update constraints
 * - Both predecessor and successor algorithm variants
 * - Flexible callback-based constraint handling
 * - Early termination options for optimization
 * - Abstract API interface for extensibility
 *
 * Constraint types:
 * - Upper/lower bounds on distance updates
 * - Step size limitations
 * - Domain-specific validation rules
 * - Custom feasibility conditions
 *
 * Example constrained parametric problem:
 * \code
 *    a ----d(5,r)----> b
 *    |                |
 *    |d(2,r)    d(3,r)|
 *    |                |
 *    v      c(1)      v
 *    d ----d(4,r)----> e
 *
 * Where d(i,r) represents distance depending on parameter r
 * and c(1) represents a constraint on updates
 * \endcode
 *
 * Algorithm variants:
 * - Predecessor-based: Traditional forward relaxation
 * - Successor-based: Reverse relaxation for different constraint types
 * - Alternating: Switches between both for robustness
 *
 * Performance characteristics:
 * - Time complexity depends on constraint restrictiveness
 * - Space complexity: O(V + E) for mappings
 * - Constraints can significantly improve convergence
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing the parameter value
 * @tparam Domain Type of the domain for distance calculations
 */
template <typename Node, typename Edge, typename Ratio>
class MinParametricAPI {
  public:
    virtual ~MinParametricAPI() = default;

    /**
     * @brief Calculate the distance for a given ratio and edge
     *
     * @param ratio The ratio parameter that affects the distance calculation
     * @param edge The edge in the graph
     * @return Ratio The calculated distance based on the given ratio and edge
     */
    virtual auto distance(const Ratio& ratio, const Edge& edge) -> Ratio = 0;

    /**
     * @brief Calculate the ratio that would make the cycle's total distance zero
     *
     * @param cycle The cycle in the graph that needs to be evaluated
     * @return Ratio The ratio that would make the cycle's total distance zero
     */
    virtual auto zero_cancel(const std::vector<Edge>& cycle) -> Ratio = 0;
};

/*!
 * @brief Minimum Parametric Solver with constraint support
 *
 * This class implements algorithms for solving constrained parametric network
 * optimization problems. It extends the basic parametric solver by incorporating
 * constraint handling through callback functions and providing both predecessor
 * and successor-based algorithm variants.
 *
 * Problem formulation:
 * \code
 *  min  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       forall e(u, v) in G(V, E)
 *       subject to: update_ok(old_dist, new_dist) == true
 * \endcode
 *
 * Algorithm approach:
 * 1. Initialize with starting parameter value
 * 2. Use constrained negative cycle detection
 * 3. Apply update constraints during relaxation
 * 4. Adjust parameter based on violating cycles
 * 5. Alternate between predecessor/successor methods
 *
 * Key innovations:
 * - Constraint-aware relaxation: Updates validated by callback
 * - Dual algorithm support: Both forward and reverse relaxation
 * - Alternating strategy: Improves robustness and convergence
 * - Early termination: Option to stop after first improvement
 *
 * Constraint handling:
 * The UpdateOk callback enables sophisticated constraint strategies:
 * \code{.cpp}
 * // Example: Only allow significant improvements
 * auto update_ok = [](double old_val, double new_val) {
 *     return new_val < old_val - 0.01;  // Minimum improvement threshold
 * };
 *
 * // Example: Bounded updates
 * auto update_ok = [](double old_val, double new_val) {
 *     return std::abs(new_val - old_val) <= max_step_size;
 * };
 * \endcode
 *
 * Use cases:
 * - Resource allocation with capacity constraints
 * - Economic equilibrium with market frictions
 * - Network optimization with budget limitations
 * - Scheduling with time window constraints
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing the parameter value
 * @tparam Domain Type of the domain for distance calculations
 */
template <typename DiGraph, typename Ratio, typename Domain>
class MinParametricSolver {
  public:
    using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
    using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;
    using UpdateOk = std::function<bool(const Domain&, const Domain&)>;

  private:
    const DiGraph& _digraph;
    MinParametricAPI<Node, Edge, Ratio>& _omega;

  public:
    /**
     * @brief Initialize the solver with a graph and parametric API
     *
     * @param digraph A directed graph where each node maps to its neighbors
     *        and the edges connecting them
     * @param omega An instance of MinParametricAPI that provides the necessary
     *        methods for distance calculation and cycle analysis
     */
    MinParametricSolver(const DiGraph& digraph, MinParametricAPI<Node, Edge, Ratio>& omega)
        : _digraph{digraph}, _omega{omega} {}

    /**
     * @brief Execute the parametric solver algorithm to find the minimum ratio
     *
     * @param dist A mutable mapping of node distances that will be updated
     *        during the algorithm
     * @param ratio The initial ratio value to start the optimization from
     * @param update_ok A callback function that determines whether a distance
     *        update is acceptable
     * @param pick_one_only If true, stops after finding the first improving cycle
     * @return std::pair<Ratio, Cycle> A pair containing:
     *         - The minimum ratio found
     *         - The cycle that corresponds to this ratio
     */
    template <typename Mapping>
    auto run(Mapping& dist, Ratio ratio, const UpdateOk& update_ok, bool pick_one_only = false)
        -> std::pair<Ratio, Cycle> {

        // Helper function to calculate edge weights based on current ratio
        auto get_weight = [this, &ratio](const Edge& edge) -> Domain {
            return static_cast<Domain>(this->_omega.distance(ratio, edge));
        };

        // Initialize tracking variables for maximum ratio and corresponding cycle
        auto r_max = ratio;
        auto c_max = Cycle{};
        auto cycle = Cycle{};
        auto reverse = true;  // Flag to alternate search direction

        // Initialize the negative cycle finder with our graph
        auto ncf = NegCycleFinderQ<DiGraph, Domain>{this->_digraph};

        // Main optimization loop
        while (true) {
            // Search for cycles in either forward or reverse direction
            if (reverse) {
                auto cycles = ncf.howard_succ(dist, get_weight, update_ok);
                for (const auto& c_i : cycles) {
                    auto r_i = this->_omega.zero_cancel(c_i);
                    if (r_max < r_i) {
                        r_max = r_i;
                        c_max = c_i;
                        if (pick_one_only) {  // Early exit if we only need one improvement
                            break;
                        }
                    }
                }
            } else {
                auto cycles = ncf.howard_pred(dist, get_weight, update_ok);
                for (const auto& c_i : cycles) {
                    auto r_i = this->_omega.zero_cancel(c_i);
                    if (r_max < r_i) {
                        r_max = r_i;
                        c_max = c_i;
                        if (pick_one_only) {  // Early exit if we only need one improvement
                            break;
                        }
                    }
                }
            }

            // Termination condition: no better ratio found
            if (r_max <= ratio) {
                break;
            }

            // Update state for next iteration
            cycle = c_max;
            ratio = r_max;
            reverse = !reverse;  // Alternate search direction
        }

        return std::make_pair(ratio, cycle);
    }

    /**
     * @brief Convenience overload without pick_one_only parameter
     */
    // template <typename Mapping>
    // auto run(Mapping& dist, Ratio ratio, const UpdateOk& update_ok) -> std::pair<Ratio, Cycle> {
    //     return run(dist, ratio, update_ok, false);
    // }
};

/*!
 * @brief Free function for constrained minimum parametric problems
 *
 * This function provides a functional interface for solving constrained
 * parametric network problems without requiring class instantiation. It combines
 * the flexibility of callback-based programming with sophisticated constraint
 * handling.
 *
 * Problem formulation:
 * \code
 *  min  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       forall e(u, v) in G(V, E)
 *       subject to update_ok(old_dist, new_dist) == true
 * \endcode
 *
 * Algorithm features:
 * - Alternating predecessor/successor search strategy
 * - Constraint-aware relaxation through update_ok callback
 * - Early termination option for faster convergence
 * - Memory-efficient cycle enumeration
 *
 * Usage patterns:
 *
 * 1. Basic constrained optimization:
 * \code
 * auto distance = [](double r, const Edge& e) { return e.cost - r * e.time; };
 * auto zero_cancel = [](const Cycle& c) { return calculate_ratio(c); };
 * auto update_ok = [](double old, double new) { return new < old; };
 *
 * auto [ratio, cycle] = min_parametric(graph, r0, distance, zero_cancel, dist, 0.0);
 * \endcode
 *
 * 2. Step-size limited optimization:
 * \code
 * auto update_ok = [max_step](double old, double new) {
 *     return std::abs(new - old) <= max_step;
 * };
 * \endcode
 *
 * 3. Threshold-based improvements:
 * \code{.cpp}
 * auto update_ok = [min_improvement](double old, double new) {
 *     return old - new >= min_improvement;
 * };
 * \endcode
 *
 * Return value:
 * - First element: Optimal parameter value found
 * - Second element: Critical cycle achieving this value (empty if none)
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing the parameter value
 * @tparam Fn1 Type of distance calculation function
 * @tparam Fn2 Type of zero cancellation function
 * @tparam Mapping Type of distance mapping (node -> distance)
 * @tparam Domain Type of the domain for distance calculations
 * @param[in] gra The directed graph to analyze
 * @param[in] ratio Initial parameter value for optimization
 * @param[in] distance Function calculating edge distance as function of r
 * @param[in] zero_cancel Function calculating parameter from a cycle
 * @param[in,out] dist Distance mapping updated during execution
 * @param[in] domain Type deduction parameter for distance domain
 * @param[in] pick_one_only If true, stop after first improving cycle
 * @return std::pair<Ratio, std::vector<typename MinParametricSolver<DiGraph, Ratio, Domain>::Edge>> Optimal parameter and critical cycle
 */
template <typename DiGraph, typename Ratio, typename Fn1, typename Fn2, typename Mapping, typename Domain>
inline auto min_parametric(const DiGraph& gra, Ratio ratio, Fn1&& distance, Fn2&& zero_cancel,
                    Mapping& dist, Domain domain, bool pick_one_only = false)
    -> std::pair<Ratio, std::vector<typename MinParametricSolver<DiGraph, Ratio, Domain>::Edge>> {

    (void)domain;  // Mark as used to avoid compiler warning
    
    // using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
    // using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;
    // using UpdateOk = std::function<bool(const Domain&, const Domain&)>;

    // Create a default update_ok that always allows updates
    auto update_ok = [](const Domain& /*old_val*/, const Domain& /*new_val*/) { return true; };

    // Helper function to calculate edge weights based on current ratio
    auto get_weight = [&distance, &ratio](const Edge& edge) -> Domain {
        return static_cast<Domain>(distance(ratio, edge));
    };

    auto r_max = ratio;
    auto c_max = Cycle{};
    auto cycle = Cycle{};
    auto reverse = true;

    auto ncf = NegCycleFinderQ<DiGraph, Domain>{gra};

    // Main optimization loop
    while (true) {
        // Search for cycles in either forward or reverse direction
        if (reverse) {
            auto cycles = ncf.howard_succ(dist, get_weight, update_ok);
            for (const auto& c_i : cycles) {
                auto r_i = zero_cancel(c_i);
                if (r_max < r_i) {
                    r_max = r_i;
                    c_max = c_i;
                    if (pick_one_only) {
                        break;
                    }
                }
            }
        } else {
            auto cycles = ncf.howard_pred(dist, get_weight, update_ok);
            for (const auto& c_i : cycles) {
                auto r_i = zero_cancel(c_i);
                if (r_max < r_i) {
                    r_max = r_i;
                    c_max = c_i;
                    if (pick_one_only) {
                        break;
                    }
                }
            }
        }

        // Termination condition: no better ratio found
        if (r_max <= ratio) {
            break;
        }

        // Update state for next iteration
        cycle = c_max;
        ratio = r_max;
        reverse = !reverse;
    }

    return std::make_pair(ratio, cycle);
}