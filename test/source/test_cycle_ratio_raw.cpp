#include <doctest/doctest.h>  // for ResultBuilder, TestCase

#include <cstdint>  // for uint32_t
#include <digraphx/map_adapter.hpp>
#include <digraphx/min_cycle_ratio.hpp>  // for NegCycleFinder
#include <list>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::unordered_map;
using std::vector;

/*!
 * @brief
 *
 */
TEST_CASE("Test minimum mean cycle (list of lists)") {
    // contains multiple edges
    list<pair<size_t, list<pair<size_t, int>>>> gra{
        {0, {{1, 5}, {2, 1}}}, {1, {{0, 1}, {2, 1}}}, {2, {{1, 1}, {0, 1}}}};

    const auto get_cost = [](const auto &edge) -> int { return edge; };
    const auto get_time = [](const auto & /*edge*/) { return 1; };

    auto dist = vector<int>(gra.size(), 0);
    auto r = 100.0;
    const auto c = min_cycle_ratio(gra, r, get_cost, get_time, dist, 0);
    CHECK(!c.empty());
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

    auto get_cost = [&edge_cost](const auto &edge) -> int { return edge_cost.at(edge); };
    auto get_time = [&edge_time](const auto &edge) -> int { return edge_time.at(edge); };

    auto dist = vector<int>(gra.size(), 0);
    auto r = 100.0;
    const auto cycle = min_cycle_ratio(gra, r, std::move(get_cost), std::move(get_time), dist, 0);
    CHECK(!cycle.empty());
    CHECK_EQ(r, 1.0);
}

