// -*- coding: utf-8 -*-
#pragma once

#include <utility>

#include "parametric.hpp"

/**
 * @file css.hpp
 * @brief Clock Skew Scheduling (CSS) objectives as one min-ratio engine.
 *
 * Single-parameter clock-skew scheduling, expressed as one minimum
 * cost-to-time ratio cycle engine. Every member of the survey's family --
 * minimum clock period, EVEN, PROP, and C-PROP -- is the *same* parametric
 * potential problem with a different (cost, time) column; they differ only in
 * the graph that is built, not in the solver:
 *
 * @verbatim
 *   Problem   cost                          time
 *   Min CP    -delay (setup) / +delay (hold) 1 (setup) / 0 (hold)
 *   EVEN      delay                          1 / 0 (pass-through)
 *   PROP      cost                           time
 *   C-PROP    mu                             sigma
 * @endverbatim
 *
 * css_ratio() solves @f$\min_C \sum \mathrm{cost} / \sum \mathrm{time}@f$ for
 * any such columns and is the single code path shared by the wrappers.
 */

/** @brief Edge for the timing objectives (EVEN, minimum clock period). */
struct timing_edge {
    double delay = 0.0;
    char type = 's';  ///< 's' setup, 'h' hold, 'p' pass-through
};

/** @brief Edge carrying a (cost, time) column pair (PROP). */
struct ratio_edge {
    double cost = 0.0;
    double time = 1.0;
};

/** @brief Edge carrying a (mu, sigma) column pair (C-PROP). */
struct cprop_edge {
    double mu = 0.0;
    double sigma = 1.0;
};

/**
 * @brief Minimum cost-to-time ratio cycle over generic (cost, time) columns.
 *
 * @f[
 *     \lambda^* = \min_{C} \frac{\sum_{e \in C} \mathrm{cost}(e)}
 *                               {\sum_{e \in C} \mathrm{time}(e)}
 * @f]
 *
 * This is the single code path for every CSS objective; the wrappers below
 * only choose the (cost, time) columns. A cycle whose total time is zero is
 * ignored (its ratio is @f$+\infty@f$).
 *
 * @tparam Graph Type of the directed graph
 * @tparam T Numeric type for the ratio
 * @tparam CostTime Callable (edge) -> pair(cost, time)
 * @tparam Mapping Type of the distance mapping (vertex -> distance)
 * @param[in] gra The input graph
 * @param[in,out] r0 Initial and final minimum ratio estimate
 * @param[in] cost_time Maps an edge to its (cost, time) column values
 * @param[in,out] dist Distance mapping used in the algorithm
 * @return auto The cycle achieving the minimum ratio
 */
template <typename Graph, typename T, typename CostTime, typename Mapping>
auto css_ratio(const Graph& gra, T& r0, CostTime cost_time, Mapping& dist) {
    using Edge = typename digraph_detail::graph_traits<Graph>::Edge;

    digraph_detail::CostTimeAPI<Edge, T, CostTime> omega{std::move(cost_time)};
    auto solver = MaxParametricSolver(gra, omega);
    return solver.run(r0, dist, T{0});
}

/**
 * @brief EVEN: maximum uniform slack.
 *
 * Edges are @ref timing_edge; a @c 'p' (pass-through) edge contributes delay
 * but no slack, so it has time zero.
 *
 * @tparam Graph Type of the directed graph (edge = timing_edge)
 * @tparam T Numeric type for the slack
 * @tparam Mapping Type of the distance mapping
 * @param[in] gra The input graph
 * @param[in,out] beta Initial and final maximum uniform slack
 * @param[in,out] dist Distance mapping used in the algorithm
 * @return auto The critical cycle
 */
template <typename Graph, typename T, typename Mapping>
auto even(const Graph& gra, T& beta, Mapping& dist) {
    auto cost_time = [](const timing_edge& edge) -> std::pair<double, double> {
        return {edge.delay, edge.type == 'p' ? 0.0 : 1.0};
    };
    return css_ratio(gra, beta, cost_time, dist);
}

/**
 * @brief PROP: variance-proportional slack (edges carry cost / time).
 *
 * @tparam Graph Type of the directed graph (edge = ratio_edge)
 * @tparam T Numeric type for the slack
 * @tparam Mapping Type of the distance mapping
 * @param[in] gra The input graph
 * @param[in,out] beta Initial and final maximum slack parameter
 * @param[in,out] dist Distance mapping used in the algorithm
 * @return auto The critical cycle
 */
template <typename Graph, typename T, typename Mapping>
auto prop(const Graph& gra, T& beta, Mapping& dist) {
    auto cost_time = [](const ratio_edge& edge) -> std::pair<double, double> {
        return {edge.cost, edge.time};
    };
    return css_ratio(gra, beta, cost_time, dist);
}

/**
 * @brief C-PROP: minimum cost-to-time ratio cycle (edges carry mu / sigma).
 *
 * C-PROP reduces to EVEN when all @c sigma are equal.
 *
 * @tparam Graph Type of the directed graph (edge = cprop_edge)
 * @tparam T Numeric type for the ratio
 * @tparam Mapping Type of the distance mapping
 * @param[in] gra The input graph
 * @param[in,out] beta Initial and final maximum ratio parameter
 * @param[in,out] dist Distance mapping used in the algorithm
 * @return auto The critical cycle
 */
template <typename Graph, typename T, typename Mapping>
auto cprop(const Graph& gra, T& beta, Mapping& dist) {
    auto cost_time
        = [](const cprop_edge& edge) -> std::pair<double, double> { return {edge.mu, edge.sigma}; };
    return css_ratio(gra, beta, cost_time, dist);
}

/**
 * @brief Minimum feasible clock period.
 *
 * Edges are @ref timing_edge with @c type @c 's' (setup) or @c 'h' (hold).
 * The smallest feasible period is the *maximum* cycle ratio, so it is computed
 * as the negated minimum ratio of the transformed (cost, time) columns.
 *
 * @tparam Graph Type of the directed graph (edge = timing_edge)
 * @tparam T Numeric type for the period
 * @tparam Mapping Type of the distance mapping
 * @param[in] gra The input graph
 * @param[in,out] tcp Initial and final clock period estimate
 * @param[in,out] dist Distance mapping used in the algorithm
 * @return auto The critical cycle
 */
template <typename Graph, typename T, typename Mapping>
auto min_clock_period(const Graph& gra, T& tcp, Mapping& dist) {
    auto cost_time = [](const timing_edge& edge) -> std::pair<double, double> {
        return edge.type == 's' ? std::pair<double, double>{-edge.delay, 1.0}
                                : std::pair<double, double>{edge.delay, 0.0};
    };
    auto cycle = css_ratio(gra, tcp, cost_time, dist);
    tcp = -tcp;
    return cycle;
}
