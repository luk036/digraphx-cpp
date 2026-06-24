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
#include <cassert>
#include <py2cpp/gen.hpp>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4702)
#endif

namespace digraph_detail {

    // Get the key from an iteration element:
    // - For pair-like (unordered_map, list<pair>): .first
    // - For direct (SimpleDiGraphS nodes): the element itself
    template <typename T> decltype(auto) _get_key(const T& entry) {
        if constexpr (requires { entry.first; }) {
            return entry.first;
        } else {
            return entry;
        }
    }

    // Get the value from an iteration element:
    // - For pair-like: .second
    // - For direct: .at(key) on the container
    template <typename T, typename Container>
    decltype(auto) _get_val(const T& entry, const Container& c) {
        if constexpr (requires { entry.second; }) {
            return entry.second;
        } else {
            return c.at(entry);
        }
    }

}  // namespace digraph_detail

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
 * Algorithm overview:
 * 1. Initialize a policy (predecessor mapping)
 * 2. Perform relaxation steps to improve the policy
 * 3. Detect cycles in the current policy graph
 * 4. Verify if detected cycles are negative
 * 5. Yield negative cycles and continue until no improvements possible
 *
 * @tparam DiGraph Type of the directed graph representation
 */
template <typename DiGraph>  //
class NegCycleFinder {
    using ItemsT = decltype(std::declval<const DiGraph&>());
    using Elem = decltype(*std::declval<ItemsT>().begin());
    using Node
        = std::remove_cv_t<std::remove_reference_t<decltype(_get_key(std::declval<Elem>()))>>;
    using NbrFunc = decltype(_get_val(std::declval<Elem>(), std::declval<const DiGraph&>()));
    using Nbrs = std::remove_cv_t<std::remove_reference_t<NbrFunc>>;
    using NbrItemsT = decltype(std::declval<const Nbrs&>());
    using NbrElem = decltype(*std::declval<NbrItemsT>().begin());
    using Edge = std::remove_cv_t<std::remove_reference_t<decltype(_get_val(
        std::declval<NbrElem>(), std::declval<const Nbrs&>()))>>;
    using Cycle = std::vector<Edge>;

    std::unordered_map<Node, std::pair<Node, Edge>> _pred{};
    const DiGraph& _digraph;

    /**
     * @brief Perform one Bellman-Ford relaxation step on the graph
     *
     * For each edge (u,v), checks if dist[v] > dist[u] + weight(u,v)
     * and updates the distance and predecessor if true.
     *
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight extraction function
     * @param[in,out] dist Current distance estimates for each node
     * @param[in] get_weight Function that extracts weight from an edge
     * @return true if any distances were updated, false if no changes occurred
     */
    template <typename Mapping, typename Callable> auto _relax(Mapping& dist, Callable&& get_weight)
        -> bool {
        auto changed = false;
        for (const auto& entry : this->_digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, this->_digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto distance = dist[utx] + std::forward<Callable>(get_weight)(edge);
                if (dist[vtx] > distance) {
                    dist[vtx] = distance;
                    this->_pred.insert_or_assign(vtx, std::pair(utx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Verify if a cycle starting from handle is negative
     *
     * Traverses the cycle in the predecessor map and checks if any edge
     * violates the triangle inequality: dist[v] > dist[u] + weight(u,v).
     *
     * @tparam Mapping Type of the distance mapping
     * @tparam Callable Type of the weight extraction function
     * @param[in] handle Starting node of the cycle to verify
     * @param[in] dist Current distance estimates
     * @param[in] get_weight Function that extracts weight from an edge
     * @return true if the cycle is negative, false otherwise
     */
    template <typename Mapping, typename Callable>
    auto _is_negative(const Node& handle, const Mapping& dist, Callable&& get_weight) const
        -> bool {
        auto vtx = handle;
        while (true) {
            const auto& [utx, edge] = this->_pred.at(vtx);
            if (dist.at(vtx) > dist.at(utx) + std::forward<Callable>(get_weight)(edge)) return true;
            vtx = utx;
            if (vtx == handle) break;
        }
        return false;
    }

    /**
     * @brief Extract the cycle edges starting from the given node
     *
     * Reconstructs a complete cycle by following predecessor links
     * starting from the handle node until returning to it.
     *
     * @param[in] handle Starting node of the cycle (must be part of a cycle)
     * @return Cycle A vector of edges forming the complete cycle
     */
    auto _cycle_list(const Node& handle) const -> Cycle {
        auto vtx = handle;
        auto cycle = Cycle{};
        while (true) {
            const auto& [utx, edge] = this->_pred.at(vtx);
            cycle.emplace_back(edge);
            vtx = utx;
            if (vtx == handle) break;
        }
        return cycle;
    }

    /**
     * @brief Extract cycle as node pairs (for find_neg_cycle compatibility)
     *
     * Reconstructs a cycle as a sequence of (u, v) node pairs by following
     * the predecessor mapping, instead of collecting edge data values.
     *
     * @param[in] handle Starting node of the cycle
     * @return std::vector<std::pair<Node, Node>> Cycle as node-pair edges
     */
    auto _cycle_list_node_pairs(const Node& handle) const -> std::vector<std::pair<Node, Node>> {
        auto vtx = handle;
        auto cycle = std::vector<std::pair<Node, Node>>{};
        while (true) {
            const auto& [utx, edge] = this->_pred.at(vtx);
            cycle.emplace_back(utx, vtx);
            vtx = utx;
            if (vtx == handle) break;
        }
        return cycle;
    }

    /**
     * @brief Relaxation using node-pair weight function (for find_neg_cycle)
     *
     * Like _relax but calls get_weight(pair{utx, vtx}) instead of
     * get_weight(edge_data), for compatibility with network_oracle.hpp.
     *
     * @tparam Mapping Type of the distance mapping
     * @tparam Callable Type of the weight function taking pair<Node,Node>
     * @param[in,out] dist Distance estimates
     * @param[in] get_weight Weight function taking pair<Node,Node>
     * @return true if any distances were updated
     */
    template <typename Mapping, typename Callable>
    auto _relax_node_pairs(Mapping& dist, Callable&& get_weight) -> bool {
        auto changed = false;
        for (const auto& entry : this->_digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, this->_digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto weight = get_weight(std::pair<Node, Node>{utx, vtx});
                auto distance = dist[utx] + weight;
                if (dist[vtx] > distance) {
                    dist[vtx] = distance;
                    this->_pred.insert_or_assign(vtx, std::pair(utx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Verify negative cycle using node-pair weight function
     *
     * Like _is_negative but calls get_weight(pair{utx, vtx}) instead of
     * get_weight(edge_data).
     *
     * @tparam Mapping Type of the distance mapping
     * @tparam Callable Type of the weight function taking pair<Node,Node>
     * @param[in] handle Node in the cycle to verify
     * @param[in] dist Current distance estimates
     * @param[in] get_weight Weight function taking pair<Node,Node>
     * @return true if the cycle is negative
     */
    template <typename Mapping, typename Callable>
    auto _is_negative_node_pairs(const Node& handle, const Mapping& dist,
                                 Callable&& get_weight) const -> bool {
        auto vtx = handle;
        while (true) {
            const auto& [utx, edge] = this->_pred.at(vtx);
            auto weight = get_weight(std::pair<Node, Node>{utx, vtx});
            if (dist.at(vtx) > dist.at(utx) + weight) return true;
            vtx = utx;
            if (vtx == handle) break;
        }
        return false;
    }

    /**
     * @brief Find all cycles in the current predecessor policy graph
     *
     * Searches the predecessor graph for cycles using visited tracking.
     * Yields nodes that complete cycles back to their start.
     *
     * @return py::Generator<Node> Generator yielding nodes that start cycles
     */
    auto _find_cycle() -> py::Generator<Node> {
        auto visited = std::unordered_map<Node, Node>{};
        if constexpr (requires { this->_digraph.size(); }) visited.reserve(this->_digraph.size());
        for (const auto& entry : this->_digraph) {
            const auto& vtx = _get_key(entry);
            if (visited.contains(vtx)) continue;
            auto utx = vtx;
            visited[utx] = vtx;
            while (this->_pred.contains(utx)) {
                utx = this->_pred[utx].first;
                auto it = visited.find(utx);
                if (it != visited.end()) {
                    if (it->second == vtx) co_yield utx;
                    break;
                }
                visited[utx] = vtx;
            }
        }
        co_return;
    }

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
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight extraction function
     * @param[in,out] dist Initial and updated distance estimates
     * @param[in] get_weight Function to extract weight from an edge
     * @return py::Generator<Cycle> Generator yielding negative cycles
     */
    template <typename Mapping, typename Callable> auto howard(Mapping& dist, Callable get_weight)
        -> py::Generator<Cycle> {
        this->_pred.clear();
        if constexpr (requires { this->_digraph.size(); })
            this->_pred.reserve(this->_digraph.size());
        auto found = false;
        while (!found && this->_relax(dist, get_weight)) {
            for (const auto vtx : this->_find_cycle()) {
                assert(this->_is_negative(vtx, dist, get_weight));
                co_yield this->_cycle_list(vtx);
                found = true;
            }
        }
        co_return;
    }

    /**
     * @brief Find one negative cycle (Bellman-Ford style, node-pair weights)
     *
     * Alternative entry point returning a single cycle as vector of (u, v)
     * node pairs. Uses a weight function taking the edge as a node pair.
     * Returns empty vector if no negative cycle exists.
     *
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight function (edge pair -> weight)
     * @param[in,out] dist Distance estimates (updated during search)
     * @param[in] get_weight Weight function taking std::pair<Node,Node>
     * @return std::vector<std::pair<Node, Node>> Cycle edges or empty
     */
    template <typename Mapping, typename Callable>
    auto find_neg_cycle(Mapping& dist, Callable&& get_weight)
        -> std::vector<std::pair<Node, Node>> {
        this->_pred.clear();
        if constexpr (requires { this->_digraph.size(); })
            this->_pred.reserve(this->_digraph.size());
        while (this->_relax_node_pairs(dist, get_weight)) {
            for (const auto vtx : this->_find_cycle()) {
                assert(this->_is_negative_node_pairs(vtx, dist, get_weight));
                return this->_cycle_list_node_pairs(vtx);
            }
        }
        return {};
    }
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
