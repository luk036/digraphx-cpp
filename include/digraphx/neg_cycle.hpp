// -*- coing: utf-8 -*-
#pragma once

/*!
Negative cycle detection for weighed graphs.
**/
#include <cassert>
#include <cppcoro/generator.hpp>
#include <type_traits>  // for is_same_v
#include <unordered_map>
#include <utility>  // for pair
#include <vector>

/*!
 * @brief Negative Cycle Finder by Howard's method
 *
 * Howard's method is a minimum cycle ratio (MCR) algorithm that uses a policy
 * iteration algorithm to find the minimum cycle ratio of a directed graph. The
 * algorithm maintains a set of candidate cycles and iteratively updates the
 * cycle with the minimum ratio until convergence. To detect negative cycles,
 * Howard's method uses a cycle detection algorithm that is based on the
 * Bellman-Ford relaxation algorithm. Specifically, the algorithm maintains a
 * predecessor graph of the original graph and performs cycle detection on this
 * graph using the Bellman-Ford relaxation algorithm. If a negative cycle is
 * detected, the algorithm terminates and returns the cycle.
 *
 * Note: Bellman-Ford's shortest-path algorithm (BF) is NOT the best way to
 * detect negative cycles, because
 *
 *  1. BF needs a source node.
 *  2. BF detect whether there is a negative cycle at the fianl stage.
 *  3. BF restarts the solution (dist[utx]) every time.
 *
 * @tparam DiGraph
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
     * The function performs one relaxation step in a graph algorithm.
     *
     * @tparam Mapping
     * @tparam Callable
     * @param[in,out] dist A mapping object that stores the current distances from a source vertex
     * to each vertex in the graph.
     * @param[in] get_weight The `get_weight` parameter is a callable object that takes an edge as
     * input and returns the weight of that edge. It is used to calculate the distance between two
     * vertices during the relaxation process.
     *
     * @return a boolean value.
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
     * The function checks if there is a negative cycle in a graph.
     *
     * @tparam Mapping
     * @tparam Callable
     * @param[in] handle The handle parameter is a reference to a Node object.
     * @param[in] dist A mapping that stores the distances from a source node to each node in the
     * graph.
     * @param[in] get_weight The `get_weight` parameter is a callable object that takes an edge as
     * input and returns the weight of that edge.
     *
     * @return a boolean value. It returns `true` if it is a negative cycle and `false` otherwise.
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
     * The function `_cycle_list` generates a cycle list by traversing a graph starting from a given
     * node.
     *
     * @param[in] handle The `handle` parameter is of type `Node` and represents a node in a graph.
     *
     * @return a `Cycle` object.
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
     * @brief Find a cycle on policy graph
     *
     * The function `_find_cycle` finds a cycle on a policy graph and returns it as a generator.
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
     * The constructor initializes a `NegCycleFinder` object with a given `DiGraph` object.
     *
     * @param[in] gra The `gra` parameter is of type `DiGraph` and represents a directed graph. It
     * is used to initialize the `_digraph` member variable of the `NegCycleFinder` class.
     */
    explicit NegCycleFinder(const DiGraph &gra) : _digraph{gra} {}

    /**
     * The function "howard" finds a negative cycle in a graph using the Howard's algorithm.
     *
     * @tparam Mapping
     * @tparam Callable
     * @param[in,out] dist A mapping object that stores the distances between vertices in the graph.
     * @param[in] get_weight The `get_weight` parameter is a callable object that is used to
     * retrieve the weight of an edge in the graph. It takes in two arguments: the source vertex and
     * the destination vertex of the edge, and returns the weight of the edge.
     */
    template <typename Mapping, typename Callable> auto howard(Mapping &dist, Callable get_weight)
        -> cppcoro::generator<Cycle> {
        this->_pred.clear();
        auto found = false;
        while (!found && this->_relax(dist, get_weight)) {
            for (auto vtx : this->_find_cycle()) {
                assert(this->_is_negative(vtx, dist, get_weight));
                co_yield this->_cycle_list(vtx);
                found = true;
            }
        }
        co_return;
    }
};
