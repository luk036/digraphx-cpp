// -*- coing: utf-8 -*-
#pragma once

/*!
Negative cycle detection for weighed graphs.
**/
#include <cassert>
#include <cppcoro/generator.hpp>
#include <functional>
#include <optional>
#include <type_traits> // for is_same_v
#include <unordered_map>
#include <utility> // for pair
#include <vector>

/*!
 * @brief negative cycle
 *
 * Note: Bellman-Ford's shortest-path algorithm (BF) is NOT the best way to
 *       detect negative cycles, because
 *
 *  1. BF needs a source node.
 *  2. BF detect whether there is a negative cycle at the fianl stage.
 *  3. BF restarts the solution (dist[utx]) every time.
 *
 * @tparam DiGraph
 */
template <typename DiGraph> //
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
  static_assert(std::is_same_v<Node, NodeTo>,
                "NodeFrom should be equal to NodeTo");

private:
  std::unordered_map<Node, std::pair<Node, Edge>> _pred;
  const DiGraph &_digraph;

public:
  /*!
   * @brief Construct a new neg Cycle Finder object
   *
   * @param[in] gra
   */
  explicit NegCycleFinder(const DiGraph &gra) : _pred(), _digraph{gra} {}

  /*!
   * @brief find negative cycle
   *
   * @tparam Mapping
   * @tparam Callable
   * @param[in,out] dist
   * @param[in] get_weight
   * @return cppcoro::generator<Cycle>
   */
  template <typename Mapping, typename Callable>
  auto howard(Mapping &dist, Callable &&get_weight)
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

private:
  /*!
   * @brief Find a cycle on policy graph
   *
   * @return cppcoro::generator<Cycle>
   */
  auto _find_cycle() -> cppcoro::generator<Node> {
    auto visited = std::unordered_map<Node, Node>{};
    for (const auto &result : this->_digraph) {
      const auto &vtx = result.first;
      if (visited.find(vtx) != visited.end()) { // contains vtx
        continue;
      }
      auto utx = vtx;
      while (true) {
        visited[utx] = vtx;
        if (this->_pred.find(utx) == this->_pred.end()) { // not contains utx
          break;
        }
        utx = this->_pred[utx].first;
        if (visited.find(utx) != visited.end()) { // contains utx
          if (visited[utx] == vtx) {
            co_yield utx;
          }
          break;
        }
      }
    }
    co_return;
  }

  /*!
   * @brief Perform one relaxation
   *
   * @tparam Mapping
   * @tparam Callable
   * @param[in,out] dist
   * @param[in] get_weight
   * @return true
   * @return false
   */
  template <typename Mapping, typename Callable>
  auto _relax(Mapping &dist, Callable &&get_weight) -> bool {
    auto changed = false;
    for (const auto &[utx, nbrs] : this->_digraph) {
      for (const auto &[vtx, edge] : nbrs) {
        auto distance = dist[utx] + get_weight(edge);
        if (dist[vtx] > distance) {
          this->_pred[vtx] = std::make_pair(utx, edge);
          dist[vtx] = distance;
          changed = true;
        }
      }
    }
    return changed;
  }

  /*!
   * @brief generate a cycle list
   *
   * @param[in] handle
   * @return Cycle
   */
  auto _cycle_list(const Node &handle) const -> Cycle {
    auto vtx = handle;
    auto cycle = Cycle{}; // TODO
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

  /*!
   * @brief check if it is really a negative cycle???
   *
   * @tparam Mapping
   * @tparam Callable
   * @param[in] handle
   * @param[in] dist
   * @param[in] get_weight
   * @return true
   * @return false
   */
  template <typename Mapping, typename Callable>
  auto _is_negative(const Node &handle, const Mapping &dist,
                    Callable &&get_weight) const -> bool {
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
};
