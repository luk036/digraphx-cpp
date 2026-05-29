#include <doctest/doctest.h>  // for TestCase, TEST_CASE

#include <cstdint>                   // for uint32_t
#include <digraphx/neg_cycle_q.hpp>  // for NegCycleFinderQ
#include <list>
#include <mywheel/map_adapter.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::unordered_map;
using std::vector;

/**
 * @brief Check if a digraph has a negative cycle using Howard's algorithm (predecessor version)
 *
 * @tparam DiGraph The digraph type
 * @tparam Mapping The distance mapping type
 * @tparam GetWeight The weight function type
 * @param digraph The digraph to check
 * @param dist A dictionary or MapAdapter with initial distances for each node
 * @param get_weight A function to extract the weight from an edge
 * @return true if a negative cycle is found, false otherwise
 */
template <typename DiGraph, typename Mapping, typename GetWeight>
auto has_negative_cycle_pred(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight)
    -> bool {
    auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
    for (const auto& cycle : finder.howard_pred(dist, std::forward<GetWeight>(get_weight),
                                                [](const auto&, const auto&) { return true; })) {
        if (!cycle.empty()) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if a digraph has a negative cycle using Howard's algorithm (successor version)
 *
 * @tparam DiGraph The digraph type
 * @tparam Mapping The distance mapping type
 * @tparam GetWeight The weight function type
 * @param digraph The digraph to check
 * @param dist A dictionary or MapAdapter with initial distances for each node
 * @param get_weight A function to extract the weight from an edge
 * @return true if a negative cycle is found, false otherwise
 */
template <typename DiGraph, typename Mapping, typename GetWeight>
auto has_negative_cycle_succ(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight)
    -> bool {
    auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
    for (const auto& cycle : finder.howard_succ(dist, std::forward<GetWeight>(get_weight),
                                                [](const auto&, const auto&) { return true; })) {
        if (!cycle.empty()) {
            return true;
        }
    }
    return false;
}

TEST_CASE("Test raw digraph by MapAdapter") {
    // Create digraph: [{1: 7, 2: 5}, {0: 0, 2: 3}, {1: 1, 0: 2}]
    vector<vector<pair<size_t, double>>> digraphph_data{
        {{1, 7.0}, {2, 5.0}}, {{0, 0.0}, {2, 3.0}}, {{1, 1.0}, {0, 2.0}}};

    auto digraph = MapAdapter{digraphph_data};
    auto dist = vector<double>{0.0, 0.0, 0.0};

    auto get_weight = [](const auto& edge) -> double { return edge; };

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test raw digraph by dict") {
    // Create digraph: {"a0": {"a1": 7, "a2": 5}, "a1": {"a0": 0, "a2": 3}, "a2": {"a1": 1, "a0":
    // 2}}
    unordered_map<std::string, unordered_map<std::string, double>> digraph{
        {std::string("a0"), {{std::string("a1"), 7.0}, {std::string("a2"), 5.0}}},
        {std::string("a1"), {{std::string("a0"), 0.0}, {std::string("a2"), 3.0}}},
        {std::string("a2"), {{std::string("a1"), 1.0}, {std::string("a0"), 2.0}}}};

    unordered_map<std::string, double> dist;
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    auto get_weight = [](const auto& edge) -> double { return edge; };

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test negative cycle") {
    // Create a digraph with a negative cycle
    // Example: 0->1:1, 1->2:1, 2->0:-3 (total cycle weight: -1)
    vector<vector<pair<size_t, double>>> digraphph_data{{{1, 1.0}}, {{2, 1.0}}, {{0, -3.0}}};

    auto digraph = MapAdapter{digraphph_data};
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto get_weight = [](const auto& edge) -> double { return edge; };

    CHECK(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test timing digraph") {
    // Create a digraph without negative cycles (similar to Python's create_test_case_timing)
    vector<vector<pair<size_t, double>>> digraphph_data{
        {{1, 2.0}, {2, 3.0}}, {{2, 1.0}}, {{0, 1.0}, {1, 1.0}}};

    auto digraph = MapAdapter{digraphph_data};
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto get_weight = [](const auto& edge) -> double { return edge; };

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test tiny digraph") {
    // Create a small digraph without negative cycles
    vector<vector<pair<size_t, double>>> digraphph_data{{{1, 1.0}}, {{2, 1.0}}, {{0, 1.0}}};

    auto digraph = MapAdapter{digraphph_data};
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto get_weight = [](const auto& edge) -> double { return edge; };

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test list of lists digraph") {
    // contains multiple edges
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 7.0}, {2, 5.0}}}, {1, {{0, 0.0}, {2, 3.0}}}, {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}};

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test dict of lists digraph") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> digraph{
        {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}, {0, 6}}}};

    const unordered_map<uint32_t, double> edge_weight{{0, 7.0}, {1, 5.0}, {2, 0.0}, {3, 3.0},
                                                      {4, 1.0}, {5, 2.0}, {6, 1.0}};

    auto get_weight = [&edge_weight](const auto& edge) -> double { return edge_weight.at(edge); };

    auto dist = unordered_map<uint32_t, double>{};
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test MapConstAdapter digraph") {
    // contains multiple edges
    vector<list<pair<size_t, double>>> digraph{
        {{1, 7.0}, {2, 5.0}}, {{0, 0.0}, {2, 3.0}}, {{1, 1.0}, {0, 2.0}, {0, 1.0}}};

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto ga = MapConstAdapter{digraph};

    CHECK_FALSE(has_negative_cycle_pred(ga, dist, get_weight));
    CHECK_FALSE(has_negative_cycle_succ(ga, dist, get_weight));
}

// ---------------------------------------------------------------------------
// Tests for find_neg_cycle_pred / find_neg_cycle_succ (node-pair cycles)
// ---------------------------------------------------------------------------

template <typename DiGraph, typename Mapping, typename GetWeight>
auto has_find_neg_cycle_pred(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight)
    -> bool {
    auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
    auto update_ok = [](const auto&, const auto&) { return true; };
    auto cycle = finder.find_neg_cycle_pred(dist, std::forward<GetWeight>(get_weight), update_ok);
    return !cycle.empty();
}

template <typename DiGraph, typename Mapping, typename GetWeight>
auto has_find_neg_cycle_succ(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight)
    -> bool {
    auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
    auto update_ok = [](const auto&, const auto&) { return true; };
    auto cycle = finder.find_neg_cycle_succ(dist, std::forward<GetWeight>(get_weight), update_ok);
    return !cycle.empty();
}

// Helper: extract edge weight from a list-based graph given a node pair
auto make_list_weight(const auto& digraph) {
    return [&digraph](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph) {
            if (e.first == utx) {
                for (const auto& n : e.second) {
                    if (n.first == vtx) return n.second;
                }
            }
        }
        return 0.0;
    };
}

TEST_CASE("Test find_neg_cycle_pred/succ with list of lists (no negative cycle)") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 7.0}, {2, 5.0}}}, {1, {{0, 0.0}, {2, 3.0}}}, {2, {{1, 1.0}, {0, 2.0}}}};

    auto get_weight = make_list_weight(digraph);
    auto dist = vector<double>{0.0, 0.0, 0.0};

    CHECK_FALSE(has_find_neg_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_find_neg_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test find_neg_cycle_pred/succ with negative cycle") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{2, 1.0}}}, {2, {{0, -3.0}}}};

    auto get_weight = make_list_weight(digraph);
    auto dist = vector<double>{0.0, 0.0, 0.0};

    CHECK(has_find_neg_cycle_pred(digraph, dist, get_weight));
    CHECK(has_find_neg_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test find_neg_cycle_pred/succ with no negative cycle") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{2, 1.0}}}, {2, {{0, 1.0}}}};

    auto get_weight = make_list_weight(digraph);
    auto dist = vector<double>{0.0, 0.0, 0.0};

    CHECK_FALSE(has_find_neg_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_find_neg_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test find_neg_cycle_pred/succ with dict graph") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> digraph{
        {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}}}};

    const unordered_map<uint32_t, double> edge_weight{{0, 7.0}, {1, 5.0}, {2, 0.0},
                                                      {3, 3.0}, {4, 1.0}, {5, 2.0}};
    // get_weight receives (utx, vtx) node pair; look up edge data from graph
    auto get_weight = [&digraph, &edge_weight](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph.at(utx)) {
            if (e.first == vtx) return edge_weight.at(e.second);
        }
        return 0.0;
    };

    auto dist = unordered_map<uint32_t, double>{};
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    CHECK_FALSE(has_find_neg_cycle_pred(digraph, dist, get_weight));
    CHECK_FALSE(has_find_neg_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test find_neg_cycle_pred/succ with dict negative cycle") {
    const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> digraph{
        {0, {{1, 0}}}, {1, {{2, 1}}}, {2, {{0, 2}}}};

    const unordered_map<uint32_t, double> edge_weight{{0, 1.0}, {1, 1.0}, {2, -3.0}};
    auto get_weight = [&digraph, &edge_weight](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph.at(utx)) {
            if (e.first == vtx) return edge_weight.at(e.second);
        }
        return 0.0;
    };

    auto dist = unordered_map<uint32_t, double>{};
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    CHECK(has_find_neg_cycle_pred(digraph, dist, get_weight));
    CHECK(has_find_neg_cycle_succ(digraph, dist, get_weight));
}

TEST_CASE("Test find_neg_cycle_pred with restrictive update_ok") {
    // Graph with negative cycle, but update_ok blocks all updates
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, -1.0}}}, {1, {{2, -1.0}}}, {2, {{0, -1.0}}}};

    // get_weight takes a node pair: look up the edge data from graph
    auto get_weight = [&digraph](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph) {
            if (e.first == utx) {
                for (const auto& n : e.second) {
                    if (n.first == vtx) return n.second;
                }
            }
        }
        return 0.0;
    };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    auto finder = NegCycleFinderQ<decltype(digraph), double>{digraph};
    // update_ok that never allows any update
    auto update_ok = [](const auto&, const auto&) { return false; };
    auto cycle = finder.find_neg_cycle_pred(dist, get_weight, update_ok);
    CHECK(cycle.empty());
}

TEST_CASE("Test find_neg_cycle_succ with restrictive update_ok") {
    // Graph with negative cycle, but update_ok blocks all updates
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, -1.0}}}, {1, {{2, -1.0}}}, {2, {{0, -1.0}}}};

    auto get_weight = [&digraph](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph) {
            if (e.first == utx) {
                for (const auto& n : e.second) {
                    if (n.first == vtx) return n.second;
                }
            }
        }
        return 0.0;
    };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    auto finder = NegCycleFinderQ<decltype(digraph), double>{digraph};
    // update_ok that never allows any update
    auto update_ok = [](const auto&, const auto&) { return false; };
    auto cycle = finder.find_neg_cycle_succ(dist, get_weight, update_ok);
    CHECK(cycle.empty());
}

TEST_CASE("Test find_neg_cycle_pred returns node-pair edges") {
    // Graph with negative cycle 0->1->2->0
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 2.0}}}, {1, {{2, 3.0}}}, {2, {{0, -6.0}}}};

    auto get_weight = [&digraph](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : digraph) {
            if (e.first == utx) {
                for (const auto& n : e.second) {
                    if (n.first == vtx) return n.second;
                }
            }
        }
        return 0.0;
    };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    auto finder = NegCycleFinderQ<decltype(digraph), double>{digraph};
    auto update_ok = [](const auto&, const auto&) { return true; };
    auto cycle = finder.find_neg_cycle_pred(dist, get_weight, update_ok);

    REQUIRE_FALSE(cycle.empty());
    // Each edge should be a node pair (u, v)
    for (const auto& edge : cycle) {
        CHECK_NE(edge.first, edge.second);  // no self-loop in result
    }
    // The cycle should have at least 2 edges
    CHECK_GE(cycle.size(), 2);
}

TEST_CASE("Test find_neg_cycle_pred/succ with MapConstAdapter") {
    vector<list<pair<size_t, double>>> digraph{
        {{1, 7.0}, {2, 5.0}}, {{0, 0.0}, {2, 3.0}}, {{1, 1.0}, {0, 2.0}}};

    auto ga = MapConstAdapter{digraph};
    auto get_weight = [&ga](const auto& edge) -> double {
        const auto [utx, vtx] = edge;
        for (const auto& e : ga.at(utx)) {
            if (e.first == vtx) return e.second;
        }
        return 0.0;
    };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    CHECK_FALSE(has_find_neg_cycle_pred(ga, dist, get_weight));
    CHECK_FALSE(has_find_neg_cycle_succ(ga, dist, get_weight));
}