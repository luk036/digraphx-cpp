// -*- coding: utf-8 -*-
#include <cstdint> // for uint32_t
#include <digraphx/map_adaptor.hpp>
#include <digraphx/min_cycle_ratio.hpp> // for NegCycleFinder
#include <doctest/doctest.h>            // for ResultBuilder, TestCase
#include <list>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::unordered_map;
// using std::unordered_multimap;
using std::vector;

/*!
 * @brief
 *
 */
TEST_CASE("Test minimum mean cycle (list of lists)") {
  // contains multiple edges
  list<pair<size_t, list<pair<size_t, int>>>> gra{
      {0, {{1, 5}, {2, 1}}}, {1, {{0, 1}, {2, 1}}}, {2, {{1, 1}, {0, 1}}}};

  const auto get_cost = [&](const auto &edge) -> int { return edge; };
  const auto get_time = [&](const auto & /*edge*/) -> int { return 1; };

  auto dist = vector<double>(gra.size(), 0.0);
  auto r = 100.0;
  const auto c = min_cycle_ratio(gra, r, get_cost, get_time, dist);
  CHECK(!c.empty());
  // CHECK_EQ(c.size(), 2);
  CHECK_EQ(r, 1.0);
}

/*!
 * @brief
 *
 */
TEST_CASE("Test minimum cost-to-time ratio (dict of list's)") {
  const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> gra{
      {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}}}};
  const vector<int> edge_cost{5, 1, 1, 1, 1, 1};
  const vector<int> edge_time{1, 1, 1, 1, 1, 1};

  auto get_cost = [&edge_cost](const auto &edge) -> int {
    return edge_cost.at(edge);
  };
  auto get_time = [&edge_time](const auto &edge) -> int {
    return edge_time.at(edge);
  };

  auto dist = vector<double>(gra.size(), 0.0);
  auto r = 100.0;
  const auto c =
      min_cycle_ratio(gra, r, std::move(get_cost), std::move(get_time), dist);
  CHECK(!c.empty());
  // CHECK_EQ(c.size(), 2);
  CHECK_EQ(r, 1.0);
}

// /*!
//  * @brief
//  *
//  */
// TEST_CASE("Test Negative Cycle (list of unordered_multimap's)") {
//   // contains multiple edges
//   list<pair<size_t, unordered_multimap<size_t, size_t>>> gra{
//       {0, {{1, 0}, {2, 1}}},
//       {1, {{0, 2}, {2, 3}}},
//       {2, {{1, 4}, {0, 5}, {0, 6}}}};

//   const vector<double> edge_weight{7.0, 5.0, 0.0, 3.0, 1.0, 2.0, 1.0};

//   const auto get_weight = [&edge_weight](const auto &edge) -> double {
//     return edge_weight.at(edge);
//   };

//   auto dist = vector<double>{0.0, 0.0, 0.0};
//   NegCycleFinder ncf(gra);
//   const auto cycle = ncf.howard(dist, std::move(get_weight));
//   CHECK(cycle.empty());
// }

// /*!
//  * @brief
//  *
//  */
// TEST_CASE("Test Negative Cycle (MapAdaptor of list's)") {
//   // contains multiple edges
//   vector<list<pair<size_t, double>>> gra{{{1, 7.0}, {2, 5.0}},
//                                          {{0, 0.0}, {2, 3.0}},
//                                          {{1, 1.0}, {0, 2.0}, {0, 1.0}}};
//   auto get_weight = [](const auto &edge) -> double { return edge; };
//   auto dist = vector<double>{0.0, 0.0, 0.0};
//   auto ga = MapConstAdaptor{gra};
//   NegCycleFinder ncf(ga);
//   const auto cycle = ncf.howard(dist, std::move(get_weight));
//   CHECK(cycle.empty());
// }
