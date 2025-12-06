// -*- coing: utf-8 -*-
#pragma once

/*!
 * @file neg_cycle.hpp
 * @brief Negative cycle detection for weighted directed graphs
 * 
 * This module implements Howard's method for efficient negative cycle detection
 * in directed graphs. It provides a policy iteration algorithm that maintains
 * candidate cycles and iteratively updates them until convergence.
 * 
 * Key features:
 * - Efficient negative cycle detection without requiring a source node
 * - Policy iteration approach for faster convergence
 * - Generator-based cycle enumeration for memory efficiency
 * - Template-based design for various graph representations
 * 
 * Example usage:
 * ```cpp
 * // Define a directed graph with weighted edges
 * std::unordered_map<int, std::unordered_map<int, double>> graph = {
 *     {0, {{1, 2.0}, {2, 3.0}}},
 *     {1, {{2, -5.0}}},  // Creates negative cycle 0->1->2->0
 *     {2, {{0, 1.0}}}
 * };
 * 
 * // Create negative cycle finder
 * NegCycleFinder finder(graph);
 * 
 * // Initialize distances (typically all zeros)
 * std::unordered_map<int, double> dist;
 * for (const auto& [node, _] : graph) {
 *     dist[node] = 0.0;
 * }
 * 
 * // Find negative cycles
 * for (const auto& cycle : finder.howard(dist, [](const auto& edge) { return edge; })) {
 *     std::cout << "Found negative cycle with " << cycle.size() << " edges\n";
 * }
 * ```
 * 
 * Performance characteristics:
 * - Time complexity: O(V * E * C) where C is the number of policy iterations
 * - Space complexity: O(V + E) for storing predecessor information
 * - Typically faster than Bellman-Ford for dense graphs with many cycles
 * 
 * @see Howard, R. A. (1960). Dynamic programming and Markov processes.
 */
#include <cassert>
#include <cppcoro/generator.hpp>
#include <type_traits>  // for is_same_v
#include <unordered_map>
#include <utility>  // for pair
#include <vector>

/*!
 * @brief Negative Cycle Finder using Howard's policy iteration method
 * 
 * This class implements Howard's algorithm for efficient negative cycle detection
 * in directed graphs. Unlike traditional Bellman-Ford approaches, Howard's method
 * uses policy iteration to maintain a set of candidate cycles and iteratively
 * improves them until convergence.
 * 
 * Algorithm overview:
 * 1. Initialize a policy (predecessor mapping)
 * 2. Perform relaxation steps to improve the policy
 * 3. Detect cycles in the current policy graph
 * 4. Verify if detected cycles are negative
 * 5. Yield negative cycles and continue until no improvements possible
 * 
 * Advantages over Bellman-Ford:
 * - No source node required
 * - Detects negative cycles during iteration, not just at the end
 * - Reuses previous distance estimates instead of restarting
 * - Often faster for graphs with many negative cycles
 * 
 * Template requirements for DiGraph:
 * - Must be iterable with begin()/end()
 * - Each element must be a pair: (node, neighbors_mapping)
 * - neighbors_mapping must be iterable with begin()/end()
 * - Each neighbor must be a pair: (target_node, edge_data)
 * 
 * @tparam DiGraph Type of the directed graph representation
 */
template <typename DiGraph>  //
class NegCycleFinder {
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

    std::unordered_map<Node, std::pair<Node, Edge>> _pred{};
    const DiGraph &_digraph;

    /**
     * @brief Perform one Bellman-Ford relaxation step on the graph
     * 
     * This method implements a single relaxation iteration that attempts to improve
     * distance estimates by considering all edges in the graph. For each edge (u,v),
     * it checks if dist[v] > dist[u] + weight(u,v) and updates if true.
     * 
     * The relaxation is the core operation that drives Howard's algorithm toward
     * finding negative cycles. Each successful relaxation updates the predecessor
     * mapping, which defines the current policy.
     * 
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight extraction function
     * @param[in,out] dist Current distance estimates for each node
     * @param[in] get_weight Function that extracts weight from an edge
     * @return true if any distances were updated, false if no changes occurred
     */
    template <typename Mapping, typename Callable> auto _relax(Mapping &dist, Callable &&get_weight)
        -> bool {
        auto changed = false;
        for (const auto &[utx, neighbors] : this->_digraph) {
            for (const auto &[vtx, edge] : neighbors) {
                auto distance = dist[utx] + get_weight(edge);
                if (dist[vtx] > distance) {
                    dist[vtx] = distance;
                    this->_pred[vtx] = std::make_pair(utx, edge);
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Verify if a cycle starting from handle is negative
     * 
     * This method traverses the cycle starting from the given node and checks
     * if the total weight around the cycle is negative. It uses the predecessor
     * mapping to follow the cycle and the current distance estimates to verify
     * the negative cycle property.
     * 
     * A cycle is negative if for some edge (u,v) in the cycle:
     * dist[v] > dist[u] + weight(u,v)
     * 
     * This verification is crucial because the policy iteration might find
     * cycles that are not actually negative due to the nature of the relaxation
     * process.
     * 
     * @tparam Mapping Type of the distance mapping
     * @tparam Callable Type of the weight extraction function  
     * @param[in] handle Starting node of the cycle to verify
     * @param[in] dist Current distance estimates
     * @param[in] get_weight Function that extracts weight from an edge
     * @return true if the cycle is negative, false otherwise
     */
    template <typename Mapping, typename Callable>
    auto _is_negative(const Node &handle, const Mapping &dist, Callable &&get_weight) const
        -> bool {
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

    /**
     * @brief Extract the cycle edges starting from the given node
     * 
     * This method reconstructs a complete cycle by following the predecessor
     * mapping starting from the handle node. It traverses the cycle until it
     * returns to the starting node, collecting all edges along the way.
     * 
     * The resulting cycle is ordered as encountered during traversal, which
     * provides a consistent representation of the cycle structure.
     * 
     * @param[in] handle Starting node of the cycle (must be part of a cycle)
     * @return Cycle A vector of edges forming the complete cycle
     */
    auto _cycle_list(const Node &handle) const -> Cycle {
        auto vtx = handle;
        auto cycle = Cycle{};
        while (true) {
            const auto &[utx, edge] = this->_pred.at(vtx);
            cycle.push_back(edge);
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return cycle;
    }

    /**
     * @brief Find all cycles in the current predecessor policy graph
     * 
     * This method searches the predecessor graph (defined by the current policy)
     * for cycles. It uses a visited tracking approach to efficiently detect cycles
     * without redundant work.
     * 
     * The algorithm works by:
     * 1. Visiting each unvisited node as a potential cycle start
     * 2. Following predecessor links until either:
     *    - A visited node is reached (cycle detected)
     *    - A node with no predecessor is reached (path ends)
     * 3. Yielding nodes that complete cycles back to their start
     * 
     * Using a generator allows memory-efficient cycle enumeration, as cycles
     * are produced on-demand rather than stored all at once.
     * 
     * @return cppcoro::generator<Node> Generator yielding nodes that start cycles
     */
    auto _find_cycle() -> cppcoro::generator<Node> {
        auto visited = std::unordered_map<Node, Node>{};
        for (const auto &result : this->_digraph) {
            const auto &vtx = result.first;
            if (visited.find(vtx) != visited.end()) {  // contains vtx
                continue;
            }
            auto utx = vtx;
            visited[utx] = vtx;
            while (this->_pred.find(utx) != this->_pred.end()) {
                utx = this->_pred[utx].first;
                if (visited.find(utx) != visited.end()) {  // contains utx
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

  public:
    /**
     * @brief Construct a Negative Cycle Finder for the given graph
     * 
     * Creates a new NegCycleFinder instance that will operate on the provided
     * directed graph. The graph is stored by reference, so it must remain valid
     * for the lifetime of the NegCycleFinder object.
     * 
     * @param[in] gra The directed graph to search for negative cycles
     */
    explicit NegCycleFinder(const DiGraph &gra) : _digraph{gra} {}

    /**
     * @brief Execute Howard's algorithm to find negative cycles
     * 
     * This is the main method that implements Howard's policy iteration algorithm
     * for negative cycle detection. It repeatedly performs relaxation steps and
     * cycle detection until no more negative cycles can be found.
     * 
     * Algorithm flow:
     * 1. Clear predecessor mapping (start fresh)
     * 2. While relaxation makes changes and no negative cycles found:
     *    a. Perform relaxation step to improve policy
     *    b. Search for cycles in current policy graph
     *    c. For each cycle found, verify it's negative
     *    d. Yield all negative cycles found
     * 
     * The method uses a generator to efficiently return cycles as they are found,
     * avoiding the need to store all cycles simultaneously.
     * 
     * @tparam Mapping Type of the distance mapping (node -> distance)
     * @tparam Callable Type of the weight extraction function
     * @param[in,out] dist Initial and updated distance estimates
     * @param[in] get_weight Function to extract weight from an edge
     * @return cppcoro::generator<Cycle> Generator yielding negative cycles
     */
    template <typename Mapping, typename Callable> auto howard(Mapping &dist, const Callable &get_weight)
        -> cppcoro::generator<Cycle> {
        this->_pred.clear();
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
};
