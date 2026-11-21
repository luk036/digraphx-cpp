// -*- coding: utf-8 -*-
#pragma once

/**
 * @file digraph_detail.hpp
 * @brief Internal implementation helpers shared by the digraphx algorithms.
 *
 * This header centralises the container-adaptation helpers (_get_key/_get_val,
 * Adapter pattern), the graph type trait, and the Howard policy-iteration
 * skeleton (Template Method pattern) used by NegCycleFinder and
 * NegCycleFinderQ. It also provides the callback adapters (Strategy pattern)
 * that let the functional free functions delegate to the class-based solvers.
 */

#include <absl/container/flat_hash_map.h>

#include <cassert>
#include <limits>
#include <py2cpp/gen.hpp>
#include <type_traits>
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

    /**
     * @brief Type trait deriving Node / Edge / Cycle from any supported graph container.
     *
     * @tparam DiGraph a container of containers (map-of-maps, list-of-lists,
     *         MapAdapter-wrapped, ...)
     */
    template <typename DiGraph> struct graph_traits {
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
    };

    /**
     * @brief Find all cycles in a predecessor/successor map (policy graph).
     *
     * @tparam DiGraph graph container type
     * @tparam PointTo a map Node -> (Node, Edge) (the policy)
     * @param[in] digraph the graph being searched
     * @param[in] point_to the policy map built by relaxation
     * @return py::Generator<Node> generator yielding the start node of each cycle
     */
    template <typename DiGraph, typename PointTo>
    auto find_cycle(const DiGraph& digraph, const PointTo& point_to)
        -> py::Generator<typename graph_traits<DiGraph>::Node> {
        using Node = typename graph_traits<DiGraph>::Node;
        auto visited = absl::flat_hash_map<Node, Node>{};
        if constexpr (requires { digraph.size(); }) visited.reserve(digraph.size());
        for (const auto& entry : digraph) {
            const auto& vtx = _get_key(entry);
            // Single probe: mark the DFS root and skip if already visited.
            if (!visited.try_emplace(vtx, vtx).second) continue;
            auto utx = vtx;
            while (true) {
                auto pit = point_to.find(utx);
                if (pit == point_to.end()) break;
                utx = pit->second.first;
                auto it = visited.find(utx);
                if (it != visited.end()) {
                    if (it->second == vtx) co_yield utx;
                    break;
                }
                visited.emplace(utx, vtx);
            }
        }
        co_return;
    }

    /**
     * @brief Reconstruct the cycle edges starting from a handle node.
     *
     * @tparam PointTo a map Node -> (Node, Edge)
     * @param[in] handle starting node (must be part of a cycle)
     * @param[in] point_to the policy map
     * @return std::vector<Edge> the cycle as a list of edges
     */
    template <typename PointTo>
    auto cycle_list(const typename PointTo::key_type& handle, const PointTo& point_to)
        -> std::vector<typename PointTo::mapped_type::second_type> {
        using Edge = typename PointTo::mapped_type::second_type;
        auto vtx = handle;
        auto cycle = std::vector<Edge>{};
        cycle.reserve(point_to.size());
        while (true) {
            const auto& [utx, edge] = point_to.at(vtx);
            cycle.emplace_back(edge);
            vtx = utx;
            if (vtx == handle) break;
        }
        return cycle;
    }

    /**
     * @brief Check whether the cycle starting at `handle` is negative.
     *
     * @tparam Node node type
     * @tparam Mapping distance mapping (node -> distance)
     * @tparam GetWeight weight extraction callable
     * @tparam PointTo policy map
     * @param[in] handle starting node of the cycle
     * @param[in] dist current distance estimates
     * @param[in] get_weight weight function
     * @param[in] point_to the policy map
     * @return true if the cycle violates the triangle inequality (negative)
     */
    template <typename Node, typename Mapping, typename GetWeight, typename PointTo>
    auto is_negative_pred(const Node& handle, const Mapping& dist, GetWeight&& get_weight,
                          const PointTo& point_to) -> bool {
        auto vtx = handle;
        while (true) {
            const auto& [utx, edge] = point_to.at(vtx);
            if (dist.at(vtx) > dist.at(utx) + std::forward<GetWeight>(get_weight)(edge))
                return true;
            vtx = utx;
            if (vtx == handle) break;
        }
        return false;
    }

    /**
     * @brief Predecessor negativity check mirrored for successor relaxation.
     *
     * The successor policy keeps @f$d_u \ge d_v - w(u,v)@f$ for
     * `point_to[u] == (v, edge)`, so a negative cycle violates the inequality in
     * the opposite direction: @f$d_u < d_v - w(u,v)@f$.
     */
    template <typename Node, typename Mapping, typename GetWeight, typename PointTo>
    auto is_negative_succ(const Node& handle, const Mapping& dist, GetWeight&& get_weight,
                          const PointTo& point_to) -> bool {
        auto vtx = handle;
        while (true) {
            const auto& [utx, edge] = point_to.at(vtx);
            if (dist.at(vtx) < dist.at(utx) - std::forward<GetWeight>(get_weight)(edge))
                return true;
            vtx = utx;
            if (vtx == handle) break;
        }
        return false;
    }

    /**
     * @brief Predecessor relaxation (Bellman-Ford forward) with an update gate.
     *
     * @tparam DiGraph graph container type
     * @tparam Mapping distance mapping
     * @tparam GetWeight weight callable
     * @tparam PointTo policy map
     * @tparam UpdateOk constraint callable (old, new) -> bool
     * @param[in] digraph the graph
     * @param[in,out] dist distance estimates
     * @param[in] get_weight weight function
     * @param[in,out] pred predecessor policy map
     * @param[in] update_ok gate on distance updates
     * @return true if any distance was updated
     */
    template <typename DiGraph, typename Mapping, typename GetWeight, typename PointTo,
              typename UpdateOk>
    auto relax_pred(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight, PointTo& pred,
                    UpdateOk&& update_ok) -> bool {
        auto changed = false;
        for (const auto& entry : digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto distance = dist[utx] + std::forward<GetWeight>(get_weight)(edge);
                if (dist[vtx] > distance
                    && std::forward<UpdateOk>(update_ok)(dist[vtx], distance)) {
                    dist[vtx] = distance;
                    pred.insert_or_assign(vtx, std::pair(utx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Successor relaxation (reverse) with an update gate.
     *
     * @tparam DiGraph graph container type
     * @tparam Mapping distance mapping
     * @tparam GetWeight weight callable
     * @tparam PointTo policy map
     * @tparam UpdateOk constraint callable (old, new) -> bool
     * @param[in] digraph the graph
     * @param[in,out] dist distance estimates
     * @param[in] get_weight weight function
     * @param[in,out] succ successor policy map
     * @param[in] update_ok gate on distance updates
     * @return true if any distance was updated
     */
    template <typename DiGraph, typename Mapping, typename GetWeight, typename PointTo,
              typename UpdateOk>
    auto relax_succ(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight, PointTo& succ,
                    UpdateOk&& update_ok) -> bool {
        auto changed = false;
        for (const auto& entry : digraph) {
            const auto& utx = _get_key(entry);
            const auto& nbrs = _get_val(entry, digraph);
            for (const auto& nbr_entry : nbrs) {
                const auto& vtx = _get_key(nbr_entry);
                const auto& edge = _get_val(nbr_entry, nbrs);
                auto distance = dist[vtx] - std::forward<GetWeight>(get_weight)(edge);
                if (dist[utx] < distance
                    && std::forward<UpdateOk>(update_ok)(dist[utx], distance)) {
                    dist[utx] = distance;
                    succ.insert_or_assign(utx, std::pair(vtx, edge));
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Relaxation strategy that caches edge weights across passes.
     *
     * The parameter (and therefore every edge weight) is fixed for the whole
     * search, so the weights only need to be evaluated once.  The cache is
     * built lazily during the second pass - a single-pass search re-evaluates
     * nothing and allocates nothing - and reused from the third pass onward.
     * This mirrors the strategy used by the Python `digraphx` port.
     *
     * @tparam IsPred true for predecessor relaxation, false for successor
     * @tparam Weight result type of the weight callable
     * @tparam DiGraph graph container type
     * @tparam Mapping distance mapping
     * @tparam PointTo policy map
     * @tparam UpdateOk constraint callable (old, new) -> bool
     */
    template <bool IsPred, typename Weight, typename DiGraph, typename Mapping, typename PointTo,
              typename UpdateOk>
    class RelaxCached {
      public:
        using Node = typename graph_traits<DiGraph>::Node;

        RelaxCached(const DiGraph& digraph, PointTo& point_to, UpdateOk update_ok)
            : _digraph(digraph), _point_to(point_to), _update_ok(std::move(update_ok)) {}

        template <typename GetWeight> auto operator()(Mapping& dist, GetWeight& get_weight)
            -> bool {
            const auto build = (_pass == 1);
            if (build) _weights.clear();
            auto wit = _weights.begin();
            auto changed = false;
            for (const auto& entry : _digraph) {
                const auto& utx = _get_key(entry);
                const auto& nbrs = _get_val(entry, _digraph);
                for (const auto& nbr_entry : nbrs) {
                    const auto& vtx = _get_key(nbr_entry);
                    const auto& edge = _get_val(nbr_entry, nbrs);
                    const auto weight
                        = (_pass >= 2) ? *wit++ : static_cast<Weight>(get_weight(edge));
                    if (build) _weights.push_back(weight);
                    if constexpr (IsPred) {
                        auto distance = dist[utx] + weight;
                        if (dist[vtx] > distance && _update_ok(dist[vtx], distance)) {
                            dist[vtx] = distance;
                            _point_to.insert_or_assign(vtx, std::pair(utx, edge));
                            changed = true;
                        }
                    } else {
                        auto distance = dist[vtx] - weight;
                        if (dist[utx] < distance && _update_ok(dist[utx], distance)) {
                            dist[utx] = distance;
                            _point_to.insert_or_assign(utx, std::pair(vtx, edge));
                            changed = true;
                        }
                    }
                }
            }
            ++_pass;
            return changed;
        }

        /**
         * @brief Verify that the cycle starting at `handle` is negative.
         *
         * The predicate is orientation-specific, so it is selected here from
         * the same `IsPred` flag that selects the relaxation direction.
         */
        template <typename GetWeight>
        auto is_negative(const Node& handle, const Mapping& dist, GetWeight& get_weight) const
            -> bool {
            if constexpr (IsPred) {
                return is_negative_pred(handle, dist, get_weight, _point_to);
            } else {
                return is_negative_succ(handle, dist, get_weight, _point_to);
            }
        }

      private:
        const DiGraph& _digraph;
        PointTo& _point_to;
        UpdateOk _update_ok;
        std::vector<Weight> _weights{};
        int _pass = 0;
    };

    /**
     * @brief Template Method: Howard's policy-iteration skeleton.
     *
     * The algorithm structure is fixed here; the relaxation step and the
     * orientation-specific negativity test are both supplied by the injected
     * relaxation strategy, so every yielded cycle is verified in debug builds.
     *
     * @tparam DiGraph graph container type
     * @tparam Mapping distance mapping
     * @tparam GetWeight weight callable
     * @tparam PointTo policy map
     * @tparam Relax strategy providing `operator()(dist, get_weight)` and
     *         `is_negative(handle, dist, get_weight)`
     * @param[in] digraph the graph
     * @param[in,out] dist distance estimates
     * @param[in] get_weight weight function
     * @param[in,out] point_to policy map to populate
     * @param[in] relax the relaxation strategy
     * @return py::Generator<Cycle> generator yielding negative cycles
     */
    template <typename DiGraph, typename Mapping, typename GetWeight, typename PointTo,
              typename Relax>
    auto howard_search(const DiGraph& digraph, Mapping& dist, GetWeight get_weight,
                       PointTo& point_to, Relax relax)
        -> py::Generator<typename graph_traits<DiGraph>::Cycle> {
        point_to.clear();
        if constexpr (requires { digraph.size(); }) point_to.reserve(digraph.size());
        auto found = false;
        while (!found && relax(dist, get_weight)) {
            for (const auto& vtx : find_cycle(digraph, point_to)) {
                assert(relax.is_negative(vtx, dist, get_weight));
                co_yield cycle_list(vtx, point_to);
                found = true;
            }
        }
        co_return;
    }

    /**
     * @brief Parametric API for edges described by a (cost, time) column pair.
     *
     * @f[
     *     d_r(e) = c(e) - r \cdot t(e), \qquad
     *     r(C) = \frac{\sum_{e \in C} c(e)}{\sum_{e \in C} t(e)}
     * @f]
     *
     * The minimum-cycle-ratio API and the CSS objectives are the same
     * arithmetic with different column extractors, so it is implemented once
     * here.  A cycle with zero total time has an undefined ratio and maps to
     * @f$+\infty@f$ rather than dividing by zero.
     *
     * @tparam Edge edge type
     * @tparam Ratio ratio type
     * @tparam CostTime callable (edge) -> pair(cost, time)
     */
    template <typename Edge, typename Ratio, typename CostTime> class CostTimeAPI {
      public:
        explicit CostTimeAPI(CostTime cost_time) : _cost_time(std::move(cost_time)) {}

        auto distance(const Ratio& ratio, const Edge& edge) const -> Ratio {
            const auto [cost, time] = _cost_time(edge);
            return static_cast<Ratio>(cost) - ratio * static_cast<Ratio>(time);
        }

        auto zero_cancel(const std::vector<Edge>& cycle) const -> Ratio {
            auto total_cost = Ratio{0};
            auto total_time = Ratio{0};
            for (const auto& edge : cycle) {
                const auto [cost, time] = _cost_time(edge);
                total_cost += static_cast<Ratio>(cost);
                total_time += static_cast<Ratio>(time);
            }
            if (total_time == Ratio{0}) return std::numeric_limits<Ratio>::infinity();
            return total_cost / total_time;
        }

      private:
        CostTime _cost_time;
    };

    /// Read the "cost" / "time" keys of a mapping edge (the MCR convention).
    struct map_cost_time {
        template <typename Edge> auto operator()(const Edge& edge) const {
            return std::pair{edge.at("cost"), edge.at("time")};
        }
    };

    /**
     * @brief Strategy adapter: present a (distance, zero_cancel) callback pair
     *        as a ParametricAPI (duck-typed interface).
     *
     * @tparam Ratio parameter / ratio type
     * @tparam Edge edge type
     * @tparam Cycle cycle type (vector of edges)
     * @tparam Domain distance domain
     * @tparam Fn1 distance callable (ratio, edge) -> distance
     * @tparam Fn2 zero-cancel callable (cycle) -> ratio
     */
    template <typename Ratio, typename Edge, typename Cycle, typename Domain, typename Fn1,
              typename Fn2>
    class CallbackParametricAPI {
      public:
        CallbackParametricAPI(Fn1 distance, Fn2 zero_cancel)
            : _distance(std::move(distance)), _zero_cancel(std::move(zero_cancel)) {}

        auto distance(Ratio& r_opt, const Edge& edge) const -> Domain {
            return static_cast<Domain>(_distance(r_opt, edge));
        }
        auto zero_cancel(const Cycle& cycle) const { return _zero_cancel(cycle); }

      private:
        Fn1 _distance;
        Fn2 _zero_cancel;
    };

}  // namespace digraph_detail

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
