#pragma once

#include <vector>

#include "neg_cycle.hpp"  // import NegCycleFinder

/*!
 * @file parametric.hpp
 * @brief Maximum parametric network problem solver
 *
 * This module provides algorithms for solving parametric network optimization
 * problems where edge costs depend on a single parameter. The goal is to find
 * the maximum parameter value such that the system of distance constraints
 * remains feasible.
 *
 * Problem formulation:
 * \code
 *  max  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       for all edges e(u, v) in G(V, E)
 * \endcode
 *
 * Key applications:
 * - Maximum cycle ratio problems
 * - Performance analysis of discrete event systems
 * - Scheduling and timing analysis
 * - Network flow optimization with parameter-dependent costs
 *
 * Example parametric network:
 * \code
 *    a --c(5,r)--> b
 *    |             |
 * c(2,r)       c(3,r)
 *    |             |
 *    v             v
 *    d --c(4,r)--> e
 *
 * Where c(i,r) represents cost depending on parameter r
 * \endcode
 *
 * Algorithm approach:
 * 1. Start with initial parameter value r₀
 * 2. Use negative cycle detection to find infeasibility
 * 3. Reduce parameter based on violating cycles
 * 4. Iterate until no more violations found
 *
 * Performance characteristics:
 * - Time complexity depends on negative cycle detection
 * - Space complexity: O(V + E) for distance storage
 * - Convergence typically in few iterations
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam ParametricAPI Interface for distance calculations
 */
template <typename DiGraph, typename ParametricAPI> class MaxParametricSolver {
  public:
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;

  private:
    NegCycleFinder<DiGraph> _ncf;
    ParametricAPI &_omega;

  public:
    /**
     * @brief Construct a Maximum Parametric Solver
     *
     * Creates a new solver instance that will find the maximum parameter value
     * for which the distance constraints are satisfiable. The solver uses the
     * provided ParametricAPI to calculate edge distances as functions of the
     * parameter.
     *
     * @param[in] gra The directed graph on which to solve the parametric problem
     * @param[in] omega API object providing distance calculation methods
     */
    MaxParametricSolver(const DiGraph &gra, ParametricAPI &omega) : _ncf{gra}, _omega{omega} {}

    /**
     * @brief Execute the maximum parametric algorithm
     *
     * This method implements the core algorithm for finding the maximum parameter
     * value r such that the distance constraints remain feasible. It uses an
     * iterative approach that repeatedly finds negative cycles and adjusts the
     * parameter based on the most violating cycles.
     *
     * Algorithm steps:
     * 1. Initialize with current parameter value r_opt
     * 2. Find negative cycles using Howard's method with current r
     * 3. For each violating cycle, calculate the parameter that would eliminate it
     * 4. Update r_opt to the minimum of these values
     * 5. Repeat until no more violations found
     *
     * The method returns the cycle that determines the final optimal parameter
     * value, which is useful for sensitivity analysis and debugging.
     *
     * @tparam Ratio Type representing the parameter value
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Domain Type of the domain for distance calculations
     * @param[in,out] r_opt Initial and final optimal parameter value
     * @param[in,out] dist Distance mapping that gets updated during execution
     * @param[in] domain Type deduction parameter for distance domain (typically default-constructed)
     * @return Cycle The critical cycle that determines the optimal parameter
     */
    template <typename Ratio, typename Mapping, typename Domain>
    auto run(Ratio &r_opt, Mapping &dist, Domain domain) {
        (void)domain;  // Mark as used to avoid compiler warning
        
        auto get_weight = [this,
                           &r_opt](const Edge &edge) -> Domain {  // note!!!
            return Domain(this->_omega.distance(r_opt, edge));
        };

        auto r_min = r_opt;
        auto c_min = Cycle{};
        auto c_opt = Cycle{};

        while (true) {
            for (const auto &ci : this->_ncf.howard(dist, std::move(get_weight))) {
                auto ri = this->_omega.zero_cancel(ci);
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
 * @brief Free function for solving maximum parametric network problems
 *
 * This function provides a functional interface for solving parametric network
 * problems without requiring explicit class instantiation. It solves the same
 * problem as MaxParametricSolver but with a more flexible callback-based
 * approach.
 *
 * Problem formulation:
 * \code
 *  max  r
 *  s.t. dist[v] - dist[u] <= distance(e, r)
 *       for all edges e(u, v) in G(V, E)
 * \endcode
 *
 * This approach is useful when:
 * - You prefer functional programming style
 * - You need custom distance calculation logic
 * - You want to avoid class instantiation overhead
 * - You're working with existing callback-based code
 *
 * The function internally creates a NegCycleFinder and applies the same
 * iterative algorithm as the class-based version.
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Ratio Type representing the parameter value
 * @tparam Fn1 Type of the distance calculation function
 * @tparam Fn2 Type of the zero cancellation function
 * @tparam Mapping Type of the distance mapping (node -> distance)
 * @tparam Domain Type of the domain for distance calculations
 * @param[in] gra The directed graph to analyze
 * @param[in,out] r_opt Initial and optimal parameter value
 * @param[in] distance Function calculating edge distance as function of r
 * @param[in] zero_cancel Function calculating parameter from a cycle
 * @param[in,out] dist Distance mapping that gets updated
 * @param[in] domain Type deduction parameter for distance domain
 * @return Cycle The critical cycle that determines the optimal parameter
 */
template <typename DiGraph, typename T, typename Fn1, typename Fn2, typename Mapping, typename D>
auto max_parametric(const DiGraph &gra, T &r_opt, Fn1 &&distance, Fn2 &&zero_cancel, Mapping &dist,
                    D domain) {
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;

    (void)domain;  // Mark as used to avoid compiler warning

    auto get_weight = [&distance, &r_opt](const Edge &edge) -> D {  // note!!!
        return static_cast<D>(distance(r_opt, edge));
    };

    auto ncf = NegCycleFinder<DiGraph>(gra);
    auto r_min = r_opt;
    auto c_min = Cycle{};
    auto c_opt = Cycle{};  // should initial outside

    while (true) {
        for (const auto &ci : ncf.howard(dist, std::move(get_weight))) {
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
