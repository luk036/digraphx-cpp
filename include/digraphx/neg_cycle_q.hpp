// -*- coding: utf-8 -*-
#pragma once

/*!
Negative cycle detection for weighed graphs with constraints.

```svgbob
    // Example of constraint-based negative cycle detection
    +-----> a ------+
    |      |       |
    |      | -1    | 2
    |      |       | (with constraint: dist[a] - dist[b] <= 1)
    |      v       |
    |     b -------> c
    |     |  -2     |
    |     |         | (with constraint: dist[c] - dist[b] <= 2)
    |     +-----><--+
    |          1
    +---- -3 (weight)
```
**/
#include <cassert>
#include <cppcoro/generator.hpp>
#include <type_traits>  // for is_same_v
#include <unordered_map>
#include <utility>  // for pair
#include <vector>

/*!
 * @brief Negative Cycle Finder with constraints by Howard's method
 *
 * This class implements both predecessor and successor versions of Howard's
 * algorithm for negative cycle detection in directed graphs with constraints.
 * 
 * ```svgbob
 *     Predecessor version (pred):
 *         a <----- b
 *         |       |
 *         +--> c <-+
 *         
 *     Successor version (succ):
 *         a -----> b
 *         |       |
 *         v       v
 *         d <----- e
 * ```
 */
template <typename DiGraph, typename Domain>  //
class NegCycleFinderQ {
    using Node1 = decltype((*std::declval<DiGraph>().begin()).first);
    using Node = std::remove_cv_t<std::remove_reference_t<Node1>>;
    using Nbrs1 = decltype((*std::declval<DiGraph>().begin()).second);
    using Nbrs = std::remove_cv_t<std::remove_reference_t<Nbrs1>>;
    using Edge1 = decltype((*std::declval<Nbrs>().begin()).second);
    using Edge = std::remove_cv_t<std::remove_reference_t<Edge1>>;
    using Cycle = std::vector<Edge>;
    using Node2 = decltype((*std::declval<Nbrs>().begin()).first);
    using NodeTo = std::remove_cv_t<std::remove_reference_t<Node2>>;
    static_assert(std::is_same_v<Node, NodeTo>, "NodeFrom should be equal to NodeTo");

    // Predecessor dictionary: maps each node to (predecessor_node, connecting_edge)
    std::unordered_map<Node, std::pair<Node, Edge>> _pred{};

    // Successor dictionary: maps each node to (successor_node, connecting_edge)
    std::unordered_map<Node, std::pair<Node, Edge>> _succ{};

    const DiGraph &_digraph;

    /**
     * @brief Detect cycles in the current predecessor/successor graph
     * 
     * @param point_to Either _pred or _succ dictionary defining the graph edges
     * @return cppcoro::generator<Node> Each node that starts a cycle in the graph
     */
    auto _find_cycle(const std::unordered_map<Node, std::pair<Node, Edge>> &point_to) 
        -> cppcoro::generator<Node> {
        auto visited = std::unordered_map<Node, Node>{};
        
        for (const auto &[vtx, _] : this->_digraph) {
            if (visited.find(vtx) != visited.end()) {
                continue;
            }
            
            auto utx = vtx;
            visited[utx] = vtx;
            
            while (point_to.find(utx) != point_to.end()) {
                utx = point_to.at(utx).first;
                if (visited.find(utx) != visited.end()) {
                    if (visited[utx] == vtx) {
                        co_yield utx;
                    }
                    break;
                }
                visited[utx] = vtx;
            }
        }
        co_return;
    }

    /**
     * @brief Perform predecessor relaxation step (Bellman-Ford style)
     * 
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param dist Current distance estimates for each node
     * @param get_weight Function to get weight of an edge
     * @param update_ok Function to determine if distance update should be applied
     * @return bool True if any distances were updated, False otherwise
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk> 
    auto _relax_pred(Mapping &dist, GetWeight &&get_weight, UpdateOk &&update_ok) -> bool {
        auto changed = false;
        for (const auto &[utx, neighbors] : this->_digraph) {
            for (const auto &[vtx, edge] : neighbors) {
                auto distance = dist[utx] + get_weight(edge);
                if (dist[vtx] > distance && update_ok(dist[vtx], distance)) {
                    dist[vtx] = distance;
                    this->_pred[vtx] = std::make_pair(utx, edge);
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Perform successor relaxation step (reverse Bellman-Ford style)
     * 
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param dist Current distance estimates for each node
     * @param get_weight Function to get weight of an edge
     * @param update_ok Function to determine if distance update should be applied
     * @return bool True if any distances were updated, False otherwise
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk> 
    auto _relax_succ(Mapping &dist, GetWeight &&get_weight, UpdateOk &&update_ok) -> bool {
        auto changed = false;
        for (const auto &[utx, neighbors] : this->_digraph) {
            for (const auto &[vtx, edge] : neighbors) {
                auto distance = dist[vtx] - get_weight(edge);
                if (dist[utx] < distance && update_ok(dist[utx], distance)) {
                    dist[utx] = distance;
                    this->_succ[utx] = std::make_pair(vtx, edge);
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Reconstruct the cycle starting from the given node
     * 
     * @param handle Starting node of the cycle
     * @param point_to Either _pred or _succ dictionary defining the edges
     * @return Cycle List of edges forming the cycle in order
     */
    auto _cycle_list(const Node &handle, const std::unordered_map<Node, std::pair<Node, Edge>> &point_to) const -> Cycle {
        auto vtx = handle;
        auto cycle = Cycle{};
        while (true) {
            const auto &[utx, edge] = point_to.at(vtx);
            cycle.push_back(edge);
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return cycle;
    }

    /**
     * @brief Verify if the cycle starting at handle is negative
     * 
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @param handle Starting node of the cycle
     * @param dist Current distance estimates
     * @param get_weight Function to get weight of an edge
     * @return bool True if the cycle is negative, False otherwise
     */
    template <typename Mapping, typename GetWeight>
    auto _is_negative(const Node &handle, const Mapping &dist, GetWeight &&get_weight) const -> bool {
        auto vtx = handle;
        while (true) {
            const auto &[utx, edge] = this->_pred.at(vtx);
            if (dist.at(vtx) > dist.at(utx) + get_weight(edge)) {
                return true;
            }
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return false;
    }

  public:
    /**
     * @brief Initialize the negative cycle finder with a directed graph
     * 
     * @param digraph A directed graph represented as a nested mapping
     */
    explicit NegCycleFinderQ(const DiGraph &digraph) : _digraph{digraph} {}

    /**
     * @brief Find negative cycles using predecessor-based Howard's algorithm
     * 
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param dist Initial distance estimates (often zero-initialized)
     * @param get_weight Function to get weight of an edge
     * @param update_ok Function to determine if distance updates are allowed
     * @return cppcoro::generator<Cycle> Each negative cycle found as a list of edges
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto howard_pred(Mapping &dist, GetWeight &&get_weight, UpdateOk &&update_ok) 
        -> cppcoro::generator<Cycle> {
        this->_pred.clear();
        auto found = false;
        while (!found && this->_relax_pred(dist, get_weight, update_ok)) {
            for (auto vtx : this->_find_cycle(this->_pred)) {
                assert(this->_is_negative(vtx, dist, get_weight));
                co_yield this->_cycle_list(vtx, this->_pred);
                found = true;
            }
        }
        co_return;
    }

    /**
     * @brief Find negative cycles using successor-based Howard's algorithm
     * 
     * @tparam Mapping Distance mapping type
     * @tparam GetWeight Callable type for getting edge weights
     * @tparam UpdateOk Callable type for update constraint
     * @param dist Initial distance estimates (often zero-initialized)
     * @param get_weight Function to get weight of an edge
     * @param update_ok Function to determine if distance updates are allowed
     * @return cppcoro::generator<Cycle> Each negative cycle found as a list of edges
     */
    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto howard_succ(Mapping &dist, GetWeight &&get_weight, UpdateOk &&update_ok) 
        -> cppcoro::generator<Cycle> {
        this->_succ.clear();
        auto found = false;
        while (!found && this->_relax_succ(dist, get_weight, update_ok)) {
            for (auto vtx : this->_find_cycle(this->_succ)) {
                // Note: Negative verification currently disabled as in Python version
                // assert(this->_is_negative(vtx, dist, get_weight));
                co_yield this->_cycle_list(vtx, this->_succ);
                found = true;
            }
        }
        co_return;
    }
};