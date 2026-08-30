// -*- coding: utf-8 -*-
#pragma once

/**
 * @file neg_cycle_q.hpp
 * @brief Negative cycle detection with constraints using Howard's method
 *
 * This module extends the basic negative cycle detection to support constrained
 * optimization problems. It implements both predecessor and successor versions
 * of Howard's algorithm, allowing for more flexible cycle detection strategies.
 *
 * Key features:
 * - Support for distance update constraints via callback functions
 * - Both predecessor-based and successor-based algorithms
 * - Flexible constraint handling for complex optimization problems
 * - Generator-based cycle enumeration for memory efficiency
 *
 * @see neg_cycle.hpp for unconstrained version
 */
#include <absl/container/flat_hash_map.h>

#include <cassert>
#include <py2cpp/gen.hpp>
#include <type_traits>
#include <utility>
#include <vector>

#include "digraph_detail.hpp"

using digraph_detail::_get_key;
using digraph_detail::_get_val;
/**
 * @brief Negative Cycle Finder with constraints using Howard's method
 *
 * This class extends the basic negative cycle detection to support constrained
 * optimization problems. It implements both predecessor and successor versions
 * of Howard's algorithm, providing flexibility in how cycles are detected and
 * how distance updates are constrained, via an update_ok callback.
 *
 * The algorithm skeleton is shared with NegCycleFinder (see
 * digraph_detail::howard_search); the constrained relaxation and the
 * verification hook are supplied as strategies here.
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Domain Numeric type for distance calculations
 */
template <typename DiGraph, typename Domain>  //
class NegCycleFinderQ {
    using Traits = digraph_detail::graph_traits<DiGraph>;
    using Node = typename Traits::Node;
    using Edge = typename Traits::Edge;
    using Cycle = typename Traits::Cycle;

    absl::flat_hash_map<Node, std::pair<Node, Edge>> _pred{};
    absl::flat_hash_map<Node, std::pair<Node, Edge>> _succ{};
    const DiGraph& _digraph;

  public:
    /**
     * @brief Initialize the negative cycle finder with a directed graph
     *
     * @param[in] digraph A directed graph represented as a nested mapping
     */
    explicit NegCycleFinderQ(const DiGraph& digraph) : _digraph{digraph} {}

    /**
     * @brief Find negative cycles using predecessor-based Howard's algorithm
     *
     * @f[
     *     d_v \gets \min(d_v,\; d_u + w(u,v)) \quad \text{s.t.} \quad
     * \text{update\_ok}(d_v^{\text{old}}, d_v^{\text{new}})
     * @f]
     *
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param[in,out] dist Initial distance estimates (often zero-initialized)
     * @param[in] get_weight Function to get weight of an edge
     * @param[in] update_ok Function to determine if distance updates are allowed
     * @return py::Generator<Cycle> Each negative cycle found as a list of edges
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto howard_pred(Mapping& dist, GetWeight get_weight, UpdateOk update_ok)
        -> py::Generator<Cycle> {
        // Strategy: constrained predecessor relaxation
        auto relax = [this, update_ok](Mapping& d, auto& gw) {
            return digraph_detail::relax_pred(this->_digraph, d, gw, this->_pred, update_ok);
        };
        // Hook: verify candidate cycles are actually negative
        auto check = [&](const auto& vtx, const auto& d, auto& gw) {
            assert(digraph_detail::is_negative(vtx, d, gw, this->_pred));
            (void)vtx;
            (void)d;
            (void)gw;
        };
        return digraph_detail::howard_search(this->_digraph, dist, std::move(get_weight),
                                             this->_pred, std::move(relax), std::move(check));
    }

    /**
     * @brief Find negative cycles using successor-based Howard's algorithm
     *
     * @f[
     *     d_u \gets \max(d_u,\; d_v - w(u,v)) \quad \text{s.t.} \quad
     * \text{update\_ok}(d_u^{\text{old}}, d_u^{\text{new}})
     * @f]
     *
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param[in,out] dist Initial distance estimates (often zero-initialized)
     * @param[in] get_weight Function to get weight of an edge
     * @param[in] update_ok Function to determine if distance updates are allowed
     * @return py::Generator<Cycle> Each negative cycle found as a list of edges
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto howard_succ(Mapping& dist, GetWeight get_weight, UpdateOk update_ok)
        -> py::Generator<Cycle> {
        // Strategy: constrained successor relaxation
        auto relax = [this, update_ok](Mapping& d, auto& gw) {
            return digraph_detail::relax_succ(this->_digraph, d, gw, this->_succ, update_ok);
        };
        // Hook: successor variant performs no negativity assertion
        auto no_check = [](const auto&, const auto&, auto&) {};
        return digraph_detail::howard_search(this->_digraph, dist, std::move(get_weight),
                                             this->_succ, std::move(relax), std::move(no_check));
    }
};
