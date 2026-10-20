// -*- coding: utf-8 -*-
#pragma once

/**
 * @file parametric_detail.hpp
 * @brief Shared parametric-search loop and its injected strategies.
 *
 * MaxParametricSolver and MinParametricSolver run the *same* loop: find the
 * cycles at the current ratio, move the ratio to the best cycle ratio, and stop
 * when no cycle improves it.  Only two things vary, and both are injected here:
 *
 * - the direction policy (`KeepSmaller` for the maximum problem, `KeepLarger`
 *   for the minimum problem) decides which ratio wins and when to stop;
 * - the search policy (`HowardRound` / `AlternatingRound`) produces the cycles
 *   of one round and owns its finder, so the policy maps persist across rounds.
 *
 * Being a header of its own keeps `digraph_detail.hpp` free of any dependency
 * on the finders, which include it in turn.
 */

#include <utility>

#include "neg_cycle.hpp"
#include "neg_cycle_q.hpp"

namespace digraph_detail {

    /// Keep the smallest cycle ratio; stop once the incumbent stops falling.
    struct KeepSmaller {
        template <typename Ratio> auto better(const Ratio& candidate, const Ratio& best) const
            -> bool {
            return candidate < best;
        }

        template <typename Ratio> auto converged(const Ratio& best, const Ratio& current) const
            -> bool {
            return best >= current;
        }
    };

    /// Keep the largest cycle ratio; stop once the incumbent stops rising.
    struct KeepLarger {
        template <typename Ratio> auto better(const Ratio& candidate, const Ratio& best) const
            -> bool {
            return candidate > best;
        }

        template <typename Ratio> auto converged(const Ratio& best, const Ratio& current) const
            -> bool {
            return best <= current;
        }
    };

    /**
     * @brief Produce the negative cycles of one round via Howard's method.
     *
     * @tparam DiGraph graph container type
     * @tparam Omega parametric API providing `distance`
     * @tparam Domain distance domain
     */
    template <typename DiGraph, typename Omega, typename Domain> class HowardRound {
      public:
        HowardRound(const DiGraph& digraph, Omega& omega) : _ncf(digraph), _omega(omega) {}

        /**
         * @brief Run one relaxation round, forwarding each cycle to `visit`.
         *
         * @param[in,out] dist distance estimates
         * @param[in] ratio current parameter value
         * @param[in] visit callable (cycle) -> bool; false stops the round
         */
        template <typename Mapping, typename Ratio, typename Visit>
        auto operator()(Mapping& dist, Ratio& ratio, Visit&& visit) -> void {
            auto get_weight = [this, &ratio](const auto& edge) -> Domain {
                return static_cast<Domain>(this->_omega.distance(ratio, edge));
            };
            for (auto&& cycle : this->_ncf.howard(dist, get_weight)) {
                if (!visit(std::move(cycle))) break;
            }
        }

      private:
        NegCycleFinder<DiGraph> _ncf;
        Omega& _omega;
    };

    /**
     * @brief Produce the negative cycles of one round, alternating directions.
     *
     * Alternating predecessor and successor relaxation is what lets the
     * constrained solver escape the corner where one direction alone keeps
     * re-finding the same cycle.
     *
     * @tparam DiGraph graph container type
     * @tparam Omega parametric API providing `distance`
     * @tparam Domain distance domain
     * @tparam UpdateOk callable (old, new) -> bool gating distance updates
     */
    template <typename DiGraph, typename Omega, typename Domain, typename UpdateOk>
    class AlternatingRound {
      public:
        AlternatingRound(const DiGraph& digraph, Omega& omega, UpdateOk update_ok)
            : _ncf(digraph), _omega(omega), _update_ok(std::move(update_ok)) {}

        template <typename Mapping, typename Ratio, typename Visit>
        auto operator()(Mapping& dist, Ratio& ratio, Visit&& visit) -> void {
            auto get_weight = [this, &ratio](const auto& edge) -> Domain {
                return static_cast<Domain>(this->_omega.distance(ratio, edge));
            };
            if (this->_reverse) {
                consume(this->_ncf.howard_succ(dist, get_weight, this->_update_ok), visit);
            } else {
                consume(this->_ncf.howard_pred(dist, get_weight, this->_update_ok), visit);
            }
            this->_reverse = !this->_reverse;
        }

      private:
        template <typename Cycles, typename Visit>
        auto consume(Cycles&& cycles, Visit& visit) -> void {
            for (auto&& cycle : cycles) {
                if (!visit(std::move(cycle))) break;
            }
        }

        NegCycleFinderQ<DiGraph, Domain> _ncf;
        Omega& _omega;
        UpdateOk _update_ok;
        bool _reverse = true;
    };

    /**
     * @brief Template Method: the parametric-search loop.
     *
     * Repeatedly finds the negative cycles at the current ratio and moves the
     * ratio to the best cycle ratio found, until no cycle improves it.
     *
     * @tparam Cycle cycle type (vector of edges)
     * @tparam Mapping distance mapping
     * @tparam Ratio parameter / ratio type
     * @tparam Direction policy with `better(candidate, best)` and
     *         `converged(best, current)`
     * @tparam Round search policy providing `operator()(dist, ratio, visit)`
     * @tparam OnCycle callable (cycle) -> Ratio
     * @param[in,out] dist distance estimates
     * @param[in] ratio initial parameter value
     * @param[in] direction direction policy
     * @param[in,out] round search policy (keeps its finder state)
     * @param[in] on_cycle maps a cycle to its ratio
     * @param[in] first_only stop each round after the first improvement
     * @return std::pair<Ratio, Cycle> final ratio and critical cycle
     */
    template <typename Cycle, typename Mapping, typename Ratio, typename Direction, typename Round,
              typename OnCycle>
    auto parametric_search(Mapping& dist, Ratio ratio, Direction direction, Round& round,
                           OnCycle on_cycle, bool first_only = false) -> std::pair<Ratio, Cycle> {
        auto ratio_best = ratio;
        auto cycle_best = Cycle{};
        auto cycle = Cycle{};

        while (true) {
            auto visit = [&](auto&& candidate) -> bool {
                auto ratio_i = on_cycle(candidate);
                if (!direction.better(ratio_i, ratio_best)) return true;
                ratio_best = ratio_i;
                cycle_best = std::forward<decltype(candidate)>(candidate);
                return !first_only;
            };
            round(dist, ratio, visit);

            if (direction.converged(ratio_best, ratio)) break;

            cycle = std::move(cycle_best);
            ratio = ratio_best;
        }

        return {ratio, std::move(cycle)};
    }

}  // namespace digraph_detail
