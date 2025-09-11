#include <doctest/doctest.h>
#include <cstdint>  // for uint32_t
#include <digraphx/parametric.hpp>
#include <list>
#include <vector>
#include <unordered_map>

using std::list;
using std::pair;
using std::vector;
using std::unordered_map;

TEST_CASE("Test Parametric Search (list of lists)") {
    list<pair<size_t, list<pair<size_t, int>>>> gra{
        {0, {{1, 5}, {2, 1}}}, {1, {{0, 1}, {2, 1}}}, {2, {{1, 1}, {0, 1}}}};

    auto distance = [](double r, const auto& edge) {
        return edge - r;
    };

    auto zero_cancel = [](const auto& cycle) {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge;
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = vector<double>(gra.size(), 0.0);
    double r = 100.0;
    max_parametric(gra, r, distance, zero_cancel, dist, 0.0);
    CHECK_EQ(r, 1.0);
}

TEST_CASE("Test Parametric Search (dict of list's)") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> gra{
        {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}}}};
    const vector<int> edge_cost{5, 1, 1, 1, 1, 1};

    auto distance = [&edge_cost](double r, const auto& edge) {
        return edge_cost.at(edge) - r;
    };

    auto zero_cancel = [&edge_cost](const auto& cycle) {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge_cost.at(edge);
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = vector<double>(gra.size(), 0.0);
    double r = 100.0;
    max_parametric(gra, r, distance, zero_cancel, dist, 0.0);
    CHECK_EQ(r, 1.0);
}
