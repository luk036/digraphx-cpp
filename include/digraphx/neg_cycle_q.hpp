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
 * @brief Negative Cycle Finder with constraints using Howard's method
 *
 * This class extends the basic negative cycle detection to support constrained
 * optimization problems. It implements both predecessor and successor versions
 * of Howard's algorithm, providing flexibility in how cycles are detected and
 * how distance updates are constrained, via an update_ok callback.
 *
 * Algorithm variants:
 *
 * 1. Predecessor-based (howard_pred):
 *    - Traditional Bellman-Ford relaxation
 *    - Updates dist[v] based on dist[u] + weight(u,v)
 *
 * 2. Successor-based (howard_succ):
 *    - Reverse relaxation logic
 *    - Updates dist[u] based on dist[v] - weight(u,v)
 *
 * @tparam DiGraph Type of the directed graph representation
 * @tparam Domain Numeric type for distance calculations
 */
template <typename DiGraph, typename Domain>  //
class NegCycleFinderQ {
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
    std::unordered_map<Node, std::pair<Node, Edge>> _succ{};
    const DiGraph& _digraph;

    auto _find_cycle(const std::unordered_map<Node, std::pair<Node, Edge>>& point_to)
        -> py::Generator<Node> {
        auto visited = std::unordered_map<Node, Node>{};
        if constexpr (requires { this->_digraph.size(); }) visited.reserve(this->_digraph.size());
        for (const auto& entry : this->_digraph) {
            const auto& vtx = _get_key(entry);
            if (visited.contains(vtx)) continue;
            auto utx = vtx;
            visited[utx] = vtx;
            while (point_to.contains(utx)) {
                utx = point_to.at(utx).first;
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

    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto _relax_pred(Mapping& dist, GetWeight&& get_weight, UpdateOk&& update_ok) -> bool {
        auto changed = false;
        for (const auto& entry : this->_digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, this->_digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto distance = dist[utx] + std::forward<GetWeight>(get_weight)(edge);
                if (dist[vtx] > distance
                    && std::forward<UpdateOk>(update_ok)(dist[vtx], distance)) {
                    dist[vtx] = distance;
                    this->_pred.insert_or_assign(vtx, std::pair(utx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    template <typename Mapping, typename GetWeight, typename UpdateOk>
    auto _relax_succ(Mapping& dist, GetWeight&& get_weight, UpdateOk&& update_ok) -> bool {
        auto changed = false;
        for (const auto& entry : this->_digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, this->_digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto distance = dist[vtx] - std::forward<GetWeight>(get_weight)(edge);
                if (dist[utx] < distance
                    && std::forward<UpdateOk>(update_ok)(dist[utx], distance)) {
                    dist[utx] = distance;
                    this->_succ.insert_or_assign(utx, std::pair(vtx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    auto _cycle_list(const Node& handle,
                     const std::unordered_map<Node, std::pair<Node, Edge>>& point_to) const
        -> Cycle {
        auto vtx = handle;
        auto cycle = Cycle{};
        while (true) {
            const auto& [utx, edge] = point_to.at(vtx);
            cycle.emplace_back(edge);
            vtx = utx;
            if (vtx == handle) break;
        }
        return cycle;
    }

    template <typename Mapping, typename GetWeight>
    auto _is_negative(const Node& handle, const Mapping& dist, GetWeight&& get_weight) const
        -> bool {
        auto vtx = handle;
        while (true) {
            const auto& [utx, edge] = this->_pred.at(vtx);
            if (dist.at(vtx) > dist.at(utx) + std::forward<GetWeight>(get_weight)(edge))
                return true;
            vtx = utx;
            if (vtx == handle) break;
        }
        return false;
    }

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
     *     d_v \gets \min(d_v,\; d_u + w(u,v)) \quad \text{s.t.} \quad \text{update\_ok}(d_v^{\text{old}}, d_v^{\text{new}})
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
        this->_pred.clear();
        if constexpr (requires { this->_digraph.size(); })
            this->_pred.reserve(this->_digraph.size());
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
     * @f[
     *     d_u \gets \max(d_u,\; d_v - w(u,v)) \quad \text{s.t.} \quad \text{update\_ok}(d_u^{\text{old}}, d_u^{\text{new}})
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
        this->_succ.clear();
        if constexpr (requires { this->_digraph.size(); })
            this->_succ.reserve(this->_digraph.size());
        auto found = false;
        while (!found && this->_relax_succ(dist, get_weight, update_ok)) {
            for (auto vtx : this->_find_cycle(this->_succ)) {
                co_yield this->_cycle_list(vtx, this->_succ);
                found = true;
            }
        }
        co_return;
    }

};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
