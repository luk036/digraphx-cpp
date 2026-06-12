// test NegCycleFinder::find_neg_cycle() -- node-pair weight variant
#include <doctest/doctest.h>

#include <cstdint>
#include <digraphx/neg_cycle.hpp>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::unordered_map;
using std::vector;

TEST_CASE("Test NegCycleFinder::find_neg_cycle with no negative cycle") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 7.0}, {2, 5.0}}},
        {1, {{0, 0.0}, {2, 3.0}}},
        {2, {{1, 1.0}, {0, 2.0}}}};

    auto get_weight = [&digraph](const pair<size_t, size_t>& edge) -> double {
        const auto [u, v] = edge;
        for (const auto& entry : digraph) {
            if (entry.first == u) {
                for (const auto& nbr : entry.second) {
                    if (nbr.first == v) return nbr.second;
                }
            }
        }
        return 0.0;
    };

    NegCycleFinder ncf(digraph);
    auto dist = vector<double>(digraph.size(), 0.0);
    auto cycle = ncf.find_neg_cycle(dist, get_weight);
    CHECK(cycle.empty());
}

TEST_CASE("Test NegCycleFinder::find_neg_cycle with negative cycle") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{2, 1.0}}}, {2, {{0, -3.0}}}};

    auto get_weight = [&digraph](const pair<size_t, size_t>& edge) -> double {
        const auto [u, v] = edge;
        for (const auto& entry : digraph) {
            if (entry.first == u) {
                for (const auto& nbr : entry.second) {
                    if (nbr.first == v) return nbr.second;
                }
            }
        }
        return 0.0;
    };

    NegCycleFinder ncf(digraph);
    auto dist = vector<double>(digraph.size(), 0.0);
    auto cycle = ncf.find_neg_cycle(dist, get_weight);

    REQUIRE_FALSE(cycle.empty());
    for (const auto& edge : cycle) {
        CHECK_NE(edge.first, edge.second);
    }
    CHECK_GE(cycle.size(), 2);
}

TEST_CASE("Test NegCycleFinder::find_neg_cycle with dict graph") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> digraph{
        {0, {{1, 0}, {2, 1}}},
        {1, {{0, 2}, {2, 3}}},
        {2, {{1, 4}, {0, 5}}}};

    const unordered_map<uint32_t, double> edge_weight{
        {0, 7.0}, {1, 5.0}, {2, 0.0}, {3, 3.0}, {4, 1.0}, {5, 2.0}};

    auto get_weight = [&digraph, &edge_weight](const pair<uint32_t, uint32_t>& edge) -> double {
        const auto [u, v] = edge;
        for (const auto& e : digraph.at(u)) {
            if (e.first == v) return edge_weight.at(e.second);
        }
        return 0.0;
    };

    NegCycleFinder ncf(digraph);
    auto dist = unordered_map<uint32_t, double>{};
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    auto cycle = ncf.find_neg_cycle(dist, get_weight);
    CHECK(cycle.empty());
}

TEST_CASE("Test NegCycleFinder::find_neg_cycle with dict negative cycle") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> digraph{
        {0, {{1, 0}}}, {1, {{2, 1}}}, {2, {{0, 2}}}};

    const unordered_map<uint32_t, double> edge_weight{{0, 1.0}, {1, 1.0}, {2, -3.0}};

    auto get_weight = [&digraph, &edge_weight](const pair<uint32_t, uint32_t>& edge) -> double {
        const auto [u, v] = edge;
        for (const auto& e : digraph.at(u)) {
            if (e.first == v) return edge_weight.at(e.second);
        }
        return 0.0;
    };

    NegCycleFinder ncf(digraph);
    auto dist = unordered_map<uint32_t, double>{};
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    auto cycle = ncf.find_neg_cycle(dist, get_weight);
    REQUIRE_FALSE(cycle.empty());
    CHECK_GE(cycle.size(), 2);
}
