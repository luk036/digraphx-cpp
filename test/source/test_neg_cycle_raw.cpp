// -*- coding: utf-8 -*-
#include <doctest/doctest.h>  // for ResultBuilder, TestCase

#include <cstdint>  // for uint32_t
#include <digraphx/map_adapter.hpp>
#include <digraphx/neg_cycle.hpp>  // for NegCycleFinder
#include <list>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::unordered_map;
using std::unordered_multimap;
using std::vector;

/*!
 * @brief
 *
 */
TEST_CASE("Test Negative Cycle (list of lists)") {
    // contains multiple edges
    list<pair<size_t, list<pair<size_t, double>>>> gra{
        {0, {{1, 7.0}, {2, 5.0}}}, {1, {{0, 0.0}, {2, 3.0}}}, {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}};

    NegCycleFinder ncf(gra);
    auto get_weight = [](const auto &edge) -> double { return edge; };
    auto dist = vector<double>(gra.size(), 0.0);
    auto cycle = vector<double>{};
    for (auto ci : ncf.howard(dist, std::move(get_weight))) {
        cycle = ci;
    }
    CHECK(cycle.empty());
}

/*!
 * @brief
 *
 */
TEST_CASE("Test Negative Cycle (dict of list's)") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> gra{
        {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}, {0, 6}}}};

    const unordered_map<uint32_t, double> edge_weight{{0, 7.0}, {1, 5.0}, {2, 0.0}, {3, 3.0},
                                                      {4, 1.0}, {5, 2.0}, {6, 1.0}};
    const auto get_weight
        = [&edge_weight](const auto &edge) -> double { return edge_weight.at(edge); };

    // auto dist = unordered_map<uint32_t, double>{{0, 0.0}, {1, 0.0}, {2,
    // 0.0}};
    auto dist = vector<double>(gra.size(), 0.0);
    NegCycleFinder ncf(gra);
    auto cycle = vector<uint32_t>{};
    for (auto ci : ncf.howard(dist, std::move(get_weight))) {
        cycle = ci;
    }
    CHECK(cycle.empty());
}

/*!
 * @brief
 *
 */
TEST_CASE("Test Negative Cycle (list of unordered_multimap's)") {
    // contains multiple edges
    list<pair<size_t, unordered_multimap<size_t, size_t>>> gra{
        {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}, {0, 6}}}};

    const vector<double> edge_weight{7.0, 5.0, 0.0, 3.0, 1.0, 2.0, 1.0};
    const auto get_weight
        = [&edge_weight](const auto &edge) -> double { return edge_weight.at(edge); };

    auto dist = vector<double>(gra.size(), 0.0);
    NegCycleFinder ncf(gra);
    auto cycle = vector<size_t>{};
    for (auto ci : ncf.howard(dist, std::move(get_weight))) {
        cycle = ci;
    }
    CHECK(cycle.empty());
}

/*!
 * @brief
 *
 */
TEST_CASE("Test Negative Cycle (MapAdapter of list's)") {
    // contains multiple edges
    vector<list<pair<size_t, double>>> gra{
        {{1, 7.0}, {2, 5.0}}, {{0, 0.0}, {2, 3.0}}, {{1, 1.0}, {0, 2.0}, {0, 1.0}}};
    auto get_weight = [](const auto &edge) -> double { return edge; };
    auto dist = vector<double>(gra.size(), 0.0);
    auto ga = MapConstAdapter{gra};
    NegCycleFinder ncf(ga);
    auto cycle = vector<double>{};
    for (auto ci : ncf.howard(dist, std::move(get_weight))) {
        cycle = ci;
    }
    CHECK(cycle.empty());
}
