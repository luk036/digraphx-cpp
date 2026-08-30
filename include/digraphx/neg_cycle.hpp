// -*- coing: utf-8 -*-
#pragma once

/**
 * @file neg_cycle.hpp
 * @brief Negative cycle detection for weighted directed graphs
 *
 * This module implements Howard's method for efficient negative cycle detection
 * in directed graphs. It provides a policy iteration algorithm that maintains
 * candidate cycles and iteratively updates them until convergence.
 *
 * @dot
 *   digraph neg_cycle {
 *     bgcolor="transparent";
 *     rankdir=LR;
 *     node [shape=circle, style=filled, fillcolor="#d4e6f1"];
 *     edge [fontsize=10];
 *     x [label="X"];
 *     y [label="Y"];
 *     z [label="Z"];
 *     x -> y [label="w=2", color="#27ae60"];
 *     y -> z [label="w=3", color="#27ae60"];
 *     z -> x [label="w=-6", color="#e74c3c", fontcolor="#e74c3c"];
 *     note [shape=note, fillcolor="#fadbd8", label="Negative cycle!\n2 + 3 + (-6) = -1 < 0"];
 *     z -> note [style=dashed, color="#888", constraint=false];
 *     { rank=same; x; y; z; }
 *   }
 * @enddot
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
 * @brief Negative Cycle Finder using Howard's policy iteration method
 *
 * This class implements Howard's algorithm for efficient negative cycle detection
 * in directed graphs. Unlike traditional Bellman-Ford approaches, Howard's method
 * uses policy iteration to maintain a set of candidate cycles and iteratively
 * improves them until convergence.
 *
 * A directed graph is assumed to be a container of containers.
 * Supports unordered_map, list-of-pairs, SimpleDiGraphS, and
 * MapAdapter-wrapped containers (see _get_key / _get_val helpers).
 *
 * @tparam DiGraph Type of the directed graph representation
 */
template <typename DiGraph>  //
class NegCycleFinder {
    using Traits = digraph_detail::graph_traits<DiGraph>;
    using Node = typename Traits::Node;
    using Edge = typename Traits::Edge;
    using Cycle = typename Traits::Cycle;

    absl::flat_hash_map<Node, std::pair<Node, Edge>> _pred{};
    const DiGraph& _digraph;

  public:
    /**
     * @brief Construct a Negative Cycle Finder for the given graph
     *
     * @param[in] digraph The directed graph to search for negative cycles
     */
    explicit NegCycleFinder(const DiGraph& digraph) : _digraph{digraph} {}

    /**
     * @brief Execute Howard's algorithm to find negative cycles
     *
     * Repeatedly performs relaxation and cycle detection until no more
     * negative cycles can be found. Yields cycles as they are discovered.
     *
     * @f[
     *     \text{Howard's policy iteration: relax } \to \text{ find cycles } \to \text{ verify
     * negativity}
     * @f]
     *
     * @dot
     *   digraph howard_iter {
     *     rankdir=TB; bgcolor="transparent";
     *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
     *     relax [label="1. Relaxation\n(Bellman-Ford)", fillcolor="#a9cce3"];
     *     cycle [label="2. Find cycles in\npredecessor graph"];
     *     check_neg [label="3. Check if\nnegative?", shape=diamond, fillcolor="#f9e79f"];
     *     yield [label="Yield\nnegative cycle", fillcolor="#7fb3d8"];
     *     update [label="4. Update\ndistances"];
     *     relax -> cycle -> check_neg;
     *     check_neg -> yield [label="Yes", color="#e74c3c"];
     *     check_neg -> update [label="No", color="#27ae60"];
     *     update -> relax;
     *   }
     * @enddot
     *
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight extraction function
     * @param[in,out] dist Initial and updated distance estimates
     * @param[in] get_weight Function to extract weight from an edge
     * @return py::Generator<Cycle> Generator yielding negative cycles
     */
    template <typename Mapping, typename Callable> auto howard(Mapping& dist, Callable get_weight)
        -> py::Generator<Cycle> {
        // Strategy: unconstrained predecessor relaxation (always allow updates)
        auto relax = [this](Mapping& d, auto& gw) {
            return digraph_detail::relax_pred(this->_digraph, d, gw, this->_pred,
                                              [](const auto&, const auto&) { return true; });
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
};
