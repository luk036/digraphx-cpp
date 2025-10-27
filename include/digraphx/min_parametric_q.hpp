#pragma once

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "neg_cycle_q.hpp"  // import NegCycleFinderQ

/**
 * @brief Minimum Parametric API Interface
 *
 * This abstract class defines the interface for minimum parametric problems.
 * Concrete implementations must provide distance and zero_cancel methods.
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

/**
 * @brief Minimum Parametric Solver with constraints
 *
 * This class solves the following parametric network problem:
 *
 *  min  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       forall e(u, v) in G(V, E)
 *
 * A parametric network problem refers to a type of optimization problem that
 * involves finding the optimal solution to a network flow problem as a
 * function of one single parameter.
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

/**
 * @brief Free function to solve minimum parametric problem
 *
 * This function solves the following parametric network problem:
 *
 *  min  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       forall e(u, v) in G(V, E)
 *
 * @tparam DiGraph The type of the directed graph
 * @tparam Ratio The type representing a ratio
 * @tparam Fn1 The type of the function to calculate the distance
 * @tparam Fn2 The type of the function to perform zero cancellation
 * @tparam Mapping The type of the mapping from vertices to their distances
 * @tparam Domain The type of the domain for distances
 *
 * @return std::pair<Ratio, Cycle> The optimal ratio and critical cycle
 */
template <typename DiGraph, typename Ratio, typename Fn1, typename Fn2, typename Mapping, typename Domain>
inline auto min_parametric(const DiGraph& gra, Ratio ratio, Fn1&& distance, Fn2&& zero_cancel,
                    Mapping& dist, Domain /* dist type */, bool pick_one_only = false)
    -> std::pair<Ratio, std::vector<typename MinParametricSolver<DiGraph, Ratio, Domain>::Edge>> {
    
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