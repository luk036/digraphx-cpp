// -*- coding: utf-8 -*-
#include <doctest/doctest.h>

#ifdef RAPIDCHECK_H
#    include <rapidcheck.h>

#    include <cstdint>
#    include <digraphx/neg_cycle.hpp>
#    include <mywheel/map_adapter.hpp>
#    include <list>
#    include <unordered_map>
#    include <vector>

using std::list;
using std::pair;
using std::unordered_map;
using std::vector;

// Helper function to create a random graph with positive weights only
auto create_random_positive_graph(size_t num_nodes, size_t num_edges) {
    using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
    Graph gra;

    // Create nodes
    for (size_t i = 0; i < num_nodes; ++i) {
        gra.push_back({i, {}});
    }

    // Add random edges with positive weights
    if (num_edges > 0) {
        for (size_t i = 0; i < num_edges; ++i) {
            auto from = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes)));
            auto to = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes)));
            auto weight = static_cast<double>(*rc::gen::positive<double>());

            // Find the node to add edge from
            for (auto& node : gra) {
                if (node.first == from) {
                    node.second.push_back({to, weight});
                    break;
                }
            }
        }
    }

    return gra;
}

// Helper function to create a random graph with a known negative cycle
auto create_graph_with_negative_cycle(size_t num_nodes) {
    using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
    Graph gra;

    // Create nodes
    for (size_t i = 0; i < num_nodes; ++i) {
        gra.push_back({i, {}});
    }

    // Add edges to create a negative cycle: 0 -> 1 -> 2 -> 0
    // Edge weights: 1.0, -3.0, 1.0 (total: -1.0)
    for (auto& node : gra) {
        if (node.first == 0) {
            node.second.push_back({1, 1.0});
        } else if (node.first == 1) {
            node.second.push_back({2, -3.0});
        } else if (node.first == 2) {
            node.second.push_back({0, 1.0});
        }
    }

    // Add some random positive edges
    if (num_nodes > 3) {
        for (auto& node : gra) {
            if (node.first >= 3) {
                auto to = static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes)));
                auto weight = static_cast<double>(*rc::gen::positive<double>());
                node.second.push_back({to, weight});
            }
        }
    }

    return gra;
}

TEST_CASE("Property-based test: Positive weight graph has no negative cycles") {
    rc::check("Graph with positive weights has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, 20));

                  auto gra = create_random_positive_graph(num_nodes, num_edges);
                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(gra.size(), 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Graph with negative cycle detects it") {
    rc::check("Graph with negative cycle is detected",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(3, 10));

                  auto gra = create_graph_with_negative_cycle(num_nodes);
                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(gra.size(), 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count > static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Empty graph has no cycles") {
    rc::check("Empty graph has no negative cycles",
              []() {
                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>{};

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Single node graph has no cycles") {
    rc::check("Single node graph has no negative cycles",
              []() {
                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra{{0, {}}};

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>{0.0};

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Self-loop with negative weight is detected") {
    rc::check("Self-loop with negative weight is detected as negative cycle",
              []() {
                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra{{0, {{0, -1.0}}}};

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>{0.0};

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count > static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Self-loop with positive weight is not detected") {
    rc::check("Self-loop with positive weight is not detected as negative cycle",
              []() {
                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra{{0, {{0, 1.0}}}};

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>{0.0};

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Graph with isolated nodes has no cycles") {
    rc::check("Graph with isolated nodes has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create isolated nodes
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Linear chain has no cycles") {
    rc::check("Linear chain graph has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create linear chain: 0 -> 1 -> 2 -> ... -> n-1
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  for (size_t i = 0; i < num_nodes - 1; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              node.second.push_back({i + 1, 1.0});
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Bidirectional edge chain has no cycles") {
    rc::check("Bidirectional edge chain has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create bidirectional chain
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  for (size_t i = 0; i < num_nodes; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              if (i > 0) {
                                  node.second.push_back({i - 1, 1.0});
                              }
                              if (i < num_nodes - 1) {
                                  node.second.push_back({i + 1, 1.0});
                              }
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Distance initialization doesn't affect cycle detection") {
    rc::check("Different distance initializations yield same cycle detection results",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(3, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create nodes
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  // Add random positive edges
                  for (size_t i = 0; i < num_nodes; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              auto to = static_cast<size_t>(
                                  *rc::gen::inRange(0, static_cast<int>(num_nodes)));
                              if (to != i) {
                                  auto weight = static_cast<double>(*rc::gen::positive<double>());
                                  node.second.push_back({to, weight});
                              }
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };

                  // Test with different initializations
                  auto dist1 = vector<double>(num_nodes, 0.0);
                  auto dist2 = vector<double>(num_nodes, 100.0);
                  auto dist3 = vector<double>(num_nodes, -100.0);

                  size_t cycle_count1 = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist1, get_weight)) {
                      ++cycle_count1;
                  }

                  size_t cycle_count2 = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist2, get_weight)) {
                      ++cycle_count2;
                  }

                  size_t cycle_count3 = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist3, get_weight)) {
                      ++cycle_count3;
                  }

                  RC_ASSERT(cycle_count1 == cycle_count2);
                  RC_ASSERT(cycle_count2 == cycle_count3);
              });
}

TEST_CASE("Property-based test: Zero-weight edges don't create negative cycles") {
    rc::check("Graph with zero-weight edges has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create nodes
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  // Add edges with zero weights
                  for (size_t i = 0; i < num_nodes; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              auto to = static_cast<size_t>(
                                  *rc::gen::inRange(0, static_cast<int>(num_nodes)));
                              node.second.push_back({to, 0.0});
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Multiple negative edges still need to form cycle") {
    rc::check("Multiple negative edges don't necessarily form a negative cycle",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(4, 10));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create nodes
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  // Add negative edges that don't form a cycle
                  // Create a DAG with negative edges
                  for (size_t i = 0; i < num_nodes - 1; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              auto weight = static_cast<double>(*rc::gen::negative<double>());
                              node.second.push_back({i + 1, weight});
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Complete graph with positive weights has no cycles") {
    rc::check("Complete graph with positive weights has no negative cycles",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 8));

                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra;

                  // Create complete graph with positive weights
                  for (size_t i = 0; i < num_nodes; ++i) {
                      gra.push_back({i, {}});
                  }

                  for (size_t i = 0; i < num_nodes; ++i) {
                      for (auto& node : gra) {
                          if (node.first == i) {
                              for (size_t j = 0; j < num_nodes; ++j) {
                                  if (i != j) {
                                      auto weight = static_cast<double>(*rc::gen::positive<double>());
                                      node.second.push_back({j, weight});
                                  }
                              }
                              break;
                          }
                      }
                  }

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

TEST_CASE("Property-based test: Multiple calls to howard are idempotent") {
    rc::check("Multiple calls to howard with same graph yield same results",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));
                  auto num_edges = static_cast<size_t>(*rc::gen::inRange(0, 20));

                  auto gra = create_random_positive_graph(num_nodes, num_edges);
                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(gra.size(), 0.0);

                  size_t cycle_count1 = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, get_weight)) {
                      ++cycle_count1;
                  }

                  size_t cycle_count2 = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, get_weight)) {
                      ++cycle_count2;
                  }

                  RC_ASSERT(cycle_count1 == cycle_count2);
              });
}

TEST_CASE("Property-based test: Negative cycle edge sum is negative") {
    rc::check("When a negative cycle is found, the sum of edge weights is negative",
              []() {
                  using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
                  Graph gra{{0, {{1, 1.0}}}, {1, {{2, -3.0}}}, {2, {{0, 1.0}}}};

                  NegCycleFinder ncf(gra);
                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(gra.size(), 0.0);

                  for (auto const& cycle : ncf.howard(dist, get_weight)) {
                      double sum = 0.0;
                      for (const auto& edge : cycle) {
                          sum += get_weight(edge);
                      }
                      RC_ASSERT(sum < 0.0);
                  }
              });
}

TEST_CASE("Property-based test: MapAdapter works correctly with RapidCheck tests") {
    rc::check("MapAdapter wrapper works correctly for negative cycle detection",
              []() {
                  auto num_nodes = static_cast<size_t>(*rc::gen::inRange(1, 10));

                  using RawGraph = vector<list<pair<size_t, double>>>;
                  RawGraph gra(num_nodes);

                  // Create graph with positive weights
                  for (size_t i = 0; i < num_nodes; ++i) {
                      auto num_outgoing =
                          static_cast<size_t>(*rc::gen::inRange(0, static_cast<int>(num_nodes)));
                      for (size_t j = 0; j < num_outgoing; ++j) {
                          auto to = static_cast<size_t>(
                              *rc::gen::inRange(0, static_cast<int>(num_nodes)));
                          auto weight = static_cast<double>(*rc::gen::positive<double>());
                          gra[i].push_back({to, weight});
                      }
                  }

                  auto get_weight = [](const auto& edge) -> double { return edge; };
                  auto dist = vector<double>(num_nodes, 0.0);
                  auto ga = MapConstAdapter{gra};
                  NegCycleFinder ncf(ga);

                  size_t cycle_count = 0;
                  for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
                      ++cycle_count;
                  }

                  RC_ASSERT(cycle_count == static_cast<size_t>(0));
              });
}

#endif  // RAPIDCHECK_H