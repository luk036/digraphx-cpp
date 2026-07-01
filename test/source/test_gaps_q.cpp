#include <doctest/doctest.h>

#include <digraphx/min_parametric_q.hpp>
#include <limits>
#include <list>
#include <mywheel/map_adapter.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

TEST_CASE("NegCycleFinderQ howard_pred restrictive update_ok") {
    vector<vector<pair<size_t, double>>> digraph_data{{{1, 1.0}}, {{2, 1.0}}, {{0, -3.0}}};

    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    SUBCASE("always block updates") {
        auto never_ok = [](const double&, const double&) { return false; };
        size_t count = 0;
        for (auto&& ci : finder.howard_pred(dist, get_weight, never_ok)) {
            ++count;
            (void)ci;
        }
        CHECK_EQ(count, 0);
    }

    SUBCASE("only allow large improvements") {
        auto picky = [](const double& old_v, const double&) { return old_v > 100.0; };
        size_t count = 0;
        for (auto&& ci : finder.howard_pred(dist, get_weight, picky)) {
            ++count;
            (void)ci;
        }
        CHECK_EQ(count, 0);
    }

    SUBCASE("restore always allow") {
        auto always_ok = [](const double&, const double&) { return true; };
        size_t count = 0;
        for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
            CHECK_FALSE(ci.empty());
            ++count;
        }
        CHECK_GT(count, 0);
    }
}

TEST_CASE("NegCycleFinderQ howard_succ restrictive update_ok") {
    vector<vector<pair<size_t, double>>> digraph_data{{{1, -2.0}}, {{0, 1.0}}};

    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0};

    SUBCASE("always block updates") {
        auto never_ok = [](const double&, const double&) { return false; };
        size_t count = 0;
        for (auto&& ci : finder.howard_succ(dist, get_weight, never_ok)) {
            ++count;
            (void)ci;
        }
        CHECK_EQ(count, 0);
    }

    SUBCASE("always allow") {
        auto always_ok = [](const double&, const double&) { return true; };
        size_t count = 0;
        for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
            CHECK_FALSE(ci.empty());
            ++count;
        }
        CHECK_GT(count, 0);
    }
}

TEST_CASE("NegCycleFinderQ howard_pred self-loop negative") {
    vector<vector<pair<size_t, double>>> digraph_data{{{0, -2.0}}};

    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinderQ howard_succ self-loop negative") {
    vector<vector<pair<size_t, double>>> digraph_data{{{0, -2.0}}};

    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinderQ empty graph") {
    vector<vector<pair<size_t, double>>> digraph_data;
    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
        ++count;
        (void)ci;
    }
    CHECK_EQ(count, 0);

    count = 0;
    for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
        ++count;
        (void)ci;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinderQ single node no edges") {
    vector<vector<pair<size_t, double>>> digraph_data{{}};
    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
        ++count;
        (void)ci;
    }
    CHECK_EQ(count, 0);

    count = 0;
    for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
        ++count;
        (void)ci;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinderQ non-zero initial distances") {
    vector<vector<pair<size_t, double>>> digraph_data{{{1, 1.0}}, {{2, 1.0}}, {{0, -3.0}}};

    auto ga = MapAdapter{digraph_data};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto always_ok = [](const double&, const double&) { return true; };

    SUBCASE("pred with positive init") {
        auto dist = vector<double>{5.0, 5.0, 5.0};
        size_t count = 0;
        for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
            CHECK_FALSE(ci.empty());
            ++count;
        }
        CHECK_GT(count, 0);
    }

    SUBCASE("succ with positive init") {
        auto dist = vector<double>{5.0, 5.0, 5.0};
        size_t count = 0;
        for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
            CHECK_FALSE(ci.empty());
            ++count;
        }
        CHECK_GT(count, 0);
    }
}

TEST_CASE("MinParametricSolver with MapAdapter graph") {
    using EdgeData = unordered_map<string, double>;
    using NodeList = vector<pair<size_t, EdgeData>>;
    NodeList row0, row1, row2;
    row0.emplace_back(1, EdgeData{{"cost", 7.0}, {"time", 1.0}});
    row1.emplace_back(2, EdgeData{{"cost", 5.0}, {"time", 1.0}});
    row2.emplace_back(0, EdgeData{{"cost", -3.0}, {"time", 1.0}});
    vector<NodeList> graph_data{row0, row1, row2};

    auto ga = MapAdapter{graph_data};

    class MyAPI2 : public MinParametricAPI<size_t, EdgeData, double> {
      public:
        auto distance(const double& ratio, const EdgeData& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<EdgeData>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    auto api = MyAPI2{};
    auto solver = MinParametricSolver<decltype(ga), double, double>{ga, api};

    vector<double> dist(graph_data.size(), std::numeric_limits<double>::infinity());
    auto update_ok = [](const double& cur, const double& nv) { return cur > nv; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);
    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("MinParametricSolver with positive-signed cycle") {
    using EdgeData = unordered_map<string, double>;
    unordered_map<string, unordered_map<string, EdgeData>> digraph{
        {"a", {{"b", {{"cost", -5.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", 2.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, EdgeData, double> {
      public:
        auto distance(const double& ratio, const EdgeData& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<EdgeData>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) { return true; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);
    CHECK(cycle.empty());
}

TEST_CASE("MinParametricSolver with pick_one_only finds improving cycle") {
    using Edge = unordered_map<string, double>;
    unordered_map<string, unordered_map<string, Edge>> digraph{
        {"a", {{"b", {{"cost", -5.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", 2.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, Edge, double> {
      public:
        auto distance(const double& ratio, const Edge& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<Edge>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) { return true; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok, true);

    CHECK(cycle.empty());
}

TEST_CASE("MinParametricSolver with pick_one_only finds improving cycle") {
    using Edge = unordered_map<string, double>;
    unordered_map<string, unordered_map<string, Edge>> digraph{
        {"a", {{"b", {{"cost", -5.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", 2.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, Edge, double> {
      public:
        auto distance(const double& ratio, const Edge& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<Edge>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) { return true; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);
    CHECK(cycle.empty());
}

TEST_CASE("MinParametricSolver with pick_one_only finds improving cycle") {
    using Edge = unordered_map<string, double>;
    unordered_map<string, unordered_map<string, Edge>> digraph{
        {"a", {{"b", {{"cost", -5.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", 2.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, Edge, double> {
      public:
        auto distance(const double& ratio, const Edge& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<Edge>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) { return true; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok, true);

    CHECK(cycle.empty());
}

TEST_CASE("MinParametricSolver with negative cycle and infinity init") {
    using Edge = unordered_map<string, double>;
    unordered_map<string, unordered_map<string, Edge>> digraph{
        {"a", {{"b", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", -4.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, Edge, double> {
      public:
        auto distance(const double& ratio, const Edge& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<Edge>& cycle) -> double override {
            double tc = 0.0, tt = 0.0;
            for (const auto& e : cycle) {
                tc += e.at("cost");
                tt += e.at("time");
            }
            return tc / tt;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = std::numeric_limits<double>::infinity();
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double& cur, const double& nv) { return cur > nv; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);

    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("NegCycleFinderQ howard_pred with MapConstAdapter") {
    vector<list<pair<size_t, double>>> digraph{{{1, 1.0}}, {{2, 1.0}}, {{0, -3.0}}};

    auto ga = MapConstAdapter{digraph};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_pred(dist, get_weight, always_ok)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinderQ howard_succ with MapConstAdapter") {
    vector<list<pair<size_t, double>>> digraph{{{1, 1.0}}, {{2, 1.0}}, {{0, -3.0}}};

    auto ga = MapConstAdapter{digraph};
    NegCycleFinderQ<decltype(ga), double> finder(ga);

    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};
    auto always_ok = [](const double&, const double&) { return true; };

    size_t count = 0;
    for (auto&& ci : finder.howard_succ(dist, get_weight, always_ok)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}
