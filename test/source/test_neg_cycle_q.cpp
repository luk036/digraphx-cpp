#include <doctest/doctest.h>  // for TestCase, TEST_CASE

#include <cstdint>  // for uint32_t
#include <digraphx/map_adapter.hpp>
#include <digraphx/neg_cycle_q.hpp>  // for NegCycleFinderQ
#include <list>
#include <unordered_map>
#include <vector>
#include <string>

using std::list;
using std::pair;
using std::unordered_map;
using std::vector;

// /**
//  * @brief Check if a digraph has a negative cycle using Howard's algorithm (predecessor version)
//  * 
//  * @tparam DiGraph The graph type
//  * @tparam Mapping The distance mapping type
//  * @tparam GetWeight The weight function type
//  * @param digraph The graph to check
//  * @param dist A dictionary or MapAdapter with initial distances for each node
//  * @param get_weight A function to extract the weight from an edge
//  * @return true if a negative cycle is found, false otherwise
//  */
// template <typename DiGraph, typename Mapping, typename GetWeight>
// auto has_negative_cycle_pred(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight) -> bool {
//     auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
//     for ([[maybe_unused]] const auto& _ : finder.howard_pred(dist, get_weight, [](const auto& , const auto& ) { return true; })) {
//         return true;
//     }
//     return false;
// }

// /**
//  * @brief Check if a digraph has a negative cycle using Howard's algorithm (successor version)
//  * 
//  * @tparam DiGraph The graph type
//  * @tparam Mapping The distance mapping type
//  * @tparam GetWeight The weight function type
//  * @param digraph The graph to check
//  * @param dist A dictionary or MapAdapter with initial distances for each node
//  * @param get_weight A function to extract the weight from an edge
//  * @return true if a negative cycle is found, false otherwise
//  */
// template <typename DiGraph, typename Mapping, typename GetWeight>
// auto has_negative_cycle_succ(const DiGraph& digraph, Mapping& dist, GetWeight&& get_weight) -> bool {
//     auto finder = NegCycleFinderQ<DiGraph, double>{digraph};
//     for ([[maybe_unused]] const auto& _ : finder.howard_succ(dist, get_weight, [](const auto& , const auto& ) { return true; })) {
//         return true;
//     }
//     return false;
// }

// TEST_CASE("Test raw graph by MapAdapter") {
//     // Create graph: [{1: 7, 2: 5}, {0: 0, 2: 3}, {1: 1, 0: 2}]
//     vector<vector<pair<size_t, double>>> graph_data{
//         {{1, 7.0}, {2, 5.0}},
//         {{0, 0.0}, {2, 3.0}}, 
//         {{1, 1.0}, {0, 2.0}}
//     };
    
//     auto digraph = MapAdapter{graph_data};
//     auto dist = vector<double>{0.0, 0.0, 0.0};
    
//     auto get_weight = [](const auto& edge) -> double { return edge; };
    
//     CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
// }

// TEST_CASE("Test raw graph by dict") {
//     // Create graph: {"a0": {"a1": 7, "a2": 5}, "a1": {"a0": 0, "a2": 3}, "a2": {"a1": 1, "a0": 2}}
//     unordered_map<std::string, unordered_map<std::string, double>> digraph {
//         {std::string("a0"), {{std::string("a1"), 7.0}, {std::string("a2"), 5.0}}},
//         {std::string("a1"), {{std::string("a0"), 0.0}, {std::string("a2"), 3.0}}},
//         {std::string("a2"), {{std::string("a1"), 1.0}, {std::string("a0"), 2.0}}}
//     };
    
//     unordered_map<std::string, double> dist;
//     for (const auto& [vtx, _] : digraph) {
//         dist[vtx] = 0.0;
//     }
    
//     auto get_weight = [](const auto& edge) -> double { return edge; };
    
//     CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
// }

// TEST_CASE("Test negative cycle") {
//     // Create a graph with a negative cycle
//     // Example: 0->1:1, 1->2:1, 2->0:-3 (total cycle weight: -1)
//     vector<vector<pair<size_t, double>>> graph_data{
//         {{1, 1.0}},
//         {{2, 1.0}},
//         {{0, -3.0}}
//     };
    
//     auto digraph = MapAdapter{graph_data};
//     auto dist = vector<double>{0.0, 0.0, 0.0};
//     auto get_weight = [](const auto& edge) -> double { return edge; };
    
//     CHECK(has_negative_cycle_pred(digraph, dist, get_weight));
//     CHECK(has_negative_cycle_succ(digraph, dist, get_weight));
// }

// TEST_CASE("Test timing graph") {
//     // Create a graph without negative cycles (similar to Python's create_test_case_timing)
//     vector<vector<pair<size_t, double>>> graph_data{
//         {{1, 2.0}, {2, 3.0}},
//         {{2, 1.0}},
//         {{0, 1.0}, {1, 1.0}}
//     };
    
//     auto digraph = MapAdapter{graph_data};
//     auto dist = vector<double>{0.0, 0.0, 0.0};
//     auto get_weight = [](const auto& edge) -> double { return edge; };
    
//     CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
// }

// TEST_CASE("Test tiny graph") {
//     // Create a small graph without negative cycles
//     vector<vector<pair<size_t, double>>> graph_data{
//         {{1, 1.0}},
//         {{2, 1.0}},
//         {{0, 1.0}}
//     };
    
//     auto digraph = MapAdapter{graph_data};
//     auto dist = vector<double>{0.0, 0.0, 0.0};
//     auto get_weight = [](const auto& edge) -> double { return edge; };
    
//     CHECK_FALSE(has_negative_cycle_pred(digraph, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(digraph, dist, get_weight));
// }

// TEST_CASE("Test list of lists graph") {
//     // contains multiple edges
//     list<pair<size_t, list<pair<size_t, double>>>> gra{
//         {0, {{1, 7.0}, {2, 5.0}}}, 
//         {1, {{0, 0.0}, {2, 3.0}}}, 
//         {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}
//     };

//     auto get_weight = [](const auto& edge) -> double { return edge; };
//     auto dist = vector<double>{0.0, 0.0, 0.0};
    
//     CHECK_FALSE(has_negative_cycle_pred(gra, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(gra, dist, get_weight));
// }

// TEST_CASE("Test dict of lists graph") {
//     const unordered_map<uint32_t, list<pair<uint32_t, uint32_t>>> gra{
//         {0, {{1, 0}, {2, 1}}}, 
//         {1, {{0, 2}, {2, 3}}}, 
//         {2, {{1, 4}, {0, 5}, {0, 6}}}
//     };

//     const unordered_map<uint32_t, double> edge_weight{
//         {0, 7.0}, {1, 5.0}, {2, 0.0}, {3, 3.0},
//         {4, 1.0}, {5, 2.0}, {6, 1.0}
//     };
    
//     auto get_weight = [&edge_weight](const auto& edge) -> double { 
//         return edge_weight.at(edge); 
//     };

//     auto dist = unordered_map<uint32_t, double>{};
//     for (const auto& [vtx, _] : gra) {
//         dist[vtx] = 0.0;
//     }
    
//     CHECK_FALSE(has_negative_cycle_pred(gra, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(gra, dist, get_weight));
// }

// TEST_CASE("Test MapConstAdapter graph") {
//     // contains multiple edges
//     vector<list<pair<size_t, double>>> gra{
//         {{1, 7.0}, {2, 5.0}}, 
//         {{0, 0.0}, {2, 3.0}}, 
//         {{1, 1.0}, {0, 2.0}, {0, 1.0}}
//     };
    
//     auto get_weight = [](const auto& edge) -> double { return edge; };
//     auto dist = vector<double>{0.0, 0.0, 0.0};
//     auto ga = MapConstAdapter{gra};
    
//     CHECK_FALSE(has_negative_cycle_pred(ga, dist, get_weight));
//     CHECK_FALSE(has_negative_cycle_succ(ga, dist, get_weight));
// }