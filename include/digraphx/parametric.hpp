#pragma once

#include <vector>

#include "neg_cycle.hpp"  // import NegCycleFinder

/**
 * @brief Maximum Parametric Solver
 *
 * This class solves the following parametric network problem:
 *
 *  max  r
 *  s.t. dist[v] - dist[u] <= distrance(e, r)
 *       for all edges e(u, v) in gra(V, E)
 *
 * A parametric network problem refers to a type of optimization problem that
 * involves finding the optimal solution to a network flow problem as a function
 * of one single parameter.
 *
 * ```svgbob
 *    Example parametric network:
 *    a ----c(5,r)----> b
 *    |                |
 *    |c(2,r)    c(3,r)|
 *    |                |
 *    v                v
 *    d ----c(4,r)----> e
 *
 *    Where c(i,r) represents cost depending on parameter r
 * ```
 *
 * @tparam DiGraph The type of the directed graph.
 * @tparam ParametricAPI The type of the parametric API.
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
     * The MaxParametricSolver constructor initializes the MaxParametricSolver object with a given
     * DiGraph and ParametricAPI.
     *
     * @param[in] gra The parameter "gra" is of type DiGraph and it represents a directed graph. It
     * is used as input for the constructor of the MaxParametricSolver class.
     * @param[in] omega omega is an object of type ParametricAPI.
     */
    MaxParametricSolver(const DiGraph &gra, ParametricAPI &omega) : _ncf{gra}, _omega{omega} {}

    /**
     * The function "run" iteratively finds the minimum weight cycle in a graph until the weight of
     * the current minimum cycle is greater than or equal to a given ratio.
     *
      * @tparam Ratio The type representing a ratio or a fraction.
      * @tparam Mapping The type of the mapping from vertices to their distances.
      * @tparam Domain The type of the domain for distances.
      * @param[in] r_opt r_opt is a reference to a variable of type Ratio.
      * @param[in] dist The `dist` parameter is a mapping that represents the distance between two
      * elements in a domain. It is used in the `get_weight` lambda function to calculate the weight
      * of an edge.     *
     * @return The function `run` returns an object of type `Cycle`.
     */
    template <typename Ratio, typename Mapping, typename Domain>
    auto run(Ratio &r_opt, Mapping &dist, Domain /* dist type */) {
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

/**
 * The function solves a network parametric problem by maximizing a parameter while satisfying a set
 * of constraints:
 *
 *  max  r
 *  s.t. dist[v] - dist[u] <= distrance(e, r)
 *       for all edges e(u, v) in gra(V, E)
 *
 * @tparam DiGraph The type of the directed graph.
 * @tparam Ratio The type representing a ratio or a fraction.
 * @tparam Fn1 The type of the function to calculate the distance.
 * @tparam Fn2 The type of the function to perform zero cancellation.
 * @tparam Mapping The type of the mapping from vertices to their distances.
 * @tparam Domain The type of the domain for distances.
 *
 * @return the optimal value of parameter r and the critical cycle.
 */
template <typename DiGraph, typename T, typename Fn1, typename Fn2, typename Mapping, typename D>
auto max_parametric(const DiGraph &gra, T &r_opt, Fn1 &&distance, Fn2 &&zero_cancel, Mapping &dist,
                    D /* dist type*/) {
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;

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
