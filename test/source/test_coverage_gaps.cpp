#include <doctest/doctest.h>

#include <digraphx/logger.hpp>
#include <digraphx/min_cycle_ratio.hpp>
#include <digraphx/parametric.hpp>

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

TEST_CASE("CycleRatioAPI distance and zero_cancel") {
    using Edge = unordered_map<string, double>;
    using DiGraph = unordered_map<string, unordered_map<string, Edge>>;

    DiGraph digraph{
        {"a", {{"b", {{"cost", 5.0}, {"time", 2.0}}}}},
        {"b", {{"c", {{"cost", 3.0}, {"time", 1.0}}}}},
        {"c", {{"a", {{"cost", 2.0}, {"time", 1.0}}}}}};

    CycleRatioAPI<DiGraph, double> api(digraph);

    SUBCASE("distance with ratio=0 gives raw cost") {
        const auto& edge = digraph.at("a").at("b");
        double r = 0.0;
        CHECK_EQ(api.distance(r, edge), 5.0);
    }

    SUBCASE("distance with positive ratio") {
        const auto& edge = digraph.at("a").at("b");
        double r = 2.0;
        CHECK_EQ(api.distance(r, edge), 5.0 - 2.0 * 2.0);
    }

    SUBCASE("zero_cancel on known cycle") {
        vector<Edge> cycle;
        cycle.push_back(digraph.at("a").at("b"));
        cycle.push_back(digraph.at("b").at("c"));
        cycle.push_back(digraph.at("c").at("a"));
        CHECK_EQ(api.zero_cancel(cycle), 2.5);
    }

    SUBCASE("zero_cancel single-edge cycle") {
        DiGraph self{{"x", {{"x", {{"cost", 7.0}, {"time", 3.0}}}}}};
        CycleRatioAPI<DiGraph, double> api2(self);
        vector<Edge> cycle{self.at("x").at("x")};
        CHECK_EQ(api2.zero_cancel(cycle), 7.0 / 3.0);
    }

    SUBCASE("zero_cancel with negative cost") {
        DiGraph ng{{"p", {{"q", {{"cost", -4.0}, {"time", 2.0}}}}},
            {"q", {{"p", {{"cost", 1.0}, {"time", 1.0}}}}}};
        CycleRatioAPI<decltype(ng), double> api3(ng);
        vector<Edge> cycle;
        cycle.push_back(ng.at("p").at("q"));
        cycle.push_back(ng.at("q").at("p"));
        CHECK_EQ(api3.zero_cancel(cycle), -1.0);
    }
}

TEST_CASE("MaxParametricSolver no negative cycle") {
    using Edge = unordered_map<string, double>;
    using DiGraph = unordered_map<string, unordered_map<string, Edge>>;

    DiGraph digraph{
        {"a", {{"b", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"b", {{"c", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"c", {{"d", {{"cost", 1.0}, {"time", 1.0}}}}}};

    auto omega = CycleRatioAPI<DiGraph, double>(digraph);
    auto solver = MaxParametricSolver(digraph, omega);

    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    double r = 0.5;
    auto cycle = solver.run(r, dist, 0.0);

    CHECK(cycle.empty());
}

TEST_CASE("NegCycleFinder howard self-loop negative") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{{0, {{0, -1.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};

    size_t count = 0;
    for (auto&& ci : ncf.howard(dist, get_weight)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinder howard self-loop positive") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{{0, {{0, 1.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};

    size_t count = 0;
    for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
        ++count;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinder howard single node no edges") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{{0, {}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0};

    size_t count = 0;
    for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
        ++count;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinder howard empty graph") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph;

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{};

    size_t count = 0;
    for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
        ++count;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinder howard non-zero initial distances") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}, {2, 5.0}}},
        {1, {{2, 3.0}}},
        {2, {{0, -2.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };

    SUBCASE("positive initial distances") {
        auto dist = vector<double>{10.0, 10.0, 10.0};
        size_t count = 0;
        for (auto&& ci : ncf.howard(dist, get_weight)) {
            CHECK_FALSE(ci.empty());
            ++count;
        }
        CHECK_EQ(count, 0);
    }

    SUBCASE("negative initial distances") {
        auto dist = vector<double>{-5.0, -5.0, -5.0};
        size_t count = 0;
        for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
            ++count;
        }
        CHECK_EQ(count, 0);
    }
}

TEST_CASE("NegCycleFinder howard disconnected components") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{0, -3.0}}},
        {2, {{3, 1.0}}}, {3, {{2, 1.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0, 0.0};

    size_t count = 0;
    for (auto&& ci : ncf.howard(dist, get_weight)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinder howard zero-weight edges") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 0.0}}}, {1, {{2, 0.0}}}, {2, {{0, 0.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0};

    size_t count = 0;
    for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
        ++count;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinder howard multiple cycles only one negative") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{0, -2.0}}},
        {2, {{3, 5.0}}}, {3, {{2, 3.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>{0.0, 0.0, 0.0, 0.0};

    size_t count = 0;
    for (auto&& ci : ncf.howard(dist, get_weight)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("NegCycleFinder howard negative cycle with int weights") {
    list<pair<size_t, list<pair<size_t, int>>>> digraph{
        {0, {{1, 1}}}, {1, {{2, 1}}}, {2, {{0, -3}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> int { return edge; };
    auto dist = vector<int>{0, 0, 0};

    size_t count = 0;
    for (auto&& ci : ncf.howard(dist, get_weight)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}

TEST_CASE("min_cycle_ratio with negative-cost cycle") {
    list<pair<size_t, list<pair<size_t, int>>>> digraph{
        {0, {{1, 1}}},
        {1, {{2, 1}}},
        {2, {{0, -3}}}};

    auto get_cost = [](const auto& edge) -> int { return edge; };
    auto get_time = [](const auto&) { return 1; };

    auto dist = vector<int>(digraph.size(), 0);
    double r = 100.0;
    auto cycle = min_cycle_ratio(digraph, r, get_cost, get_time, dist, 0);

    CHECK_FALSE(cycle.empty());
    CHECK_EQ(r, doctest::Approx(-1.0 / 3.0).epsilon(0.001));
}

TEST_CASE("min_cycle_ratio with non-zero initial distance") {
    list<pair<size_t, list<pair<size_t, int>>>> digraph{
        {0, {{1, 5}, {2, 1}}},
        {1, {{0, 1}, {2, 1}}},
        {2, {{1, 1}, {0, 1}}}};

    auto get_cost = [](const auto& edge) -> int { return edge; };
    auto get_time = [](const auto&) { return 1; };

    auto dist = vector<int>(digraph.size(), 42);
    double r = 100.0;
    auto cycle = min_cycle_ratio(digraph, r, get_cost, get_time, dist, 0);

    CHECK_FALSE(cycle.empty());
    CHECK_EQ(r, 1.0);
}

TEST_CASE("NegCycleFinder integer weights no negative cycle") {
    list<pair<size_t, list<pair<size_t, int>>>> digraph{
        {0, {{1, 10}}},
        {1, {{2, 20}}},
        {2, {{0, 30}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> int { return edge; };
    auto dist = vector<int>{0, 0, 0};

    size_t count = 0;
    for ([[maybe_unused]] auto&& ci : ncf.howard(dist, get_weight)) {
        ++count;
    }
    CHECK_EQ(count, 0);
}

TEST_CASE("NegCycleFinder dict graph with negative cycle") {
    unordered_map<string, unordered_map<string, double>> digraph{
        {"x", {{"y", 1.0}}},
        {"y", {{"z", 1.0}}},
        {"z", {{"x", -3.0}}}};

    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    unordered_map<string, double> dist;
    for (const auto& [v, _] : digraph) {
        dist[v] = 0.0;
    }

    size_t count = 0;
    for (auto&& ci : ncf.howard(dist, get_weight)) {
        CHECK_FALSE(ci.empty());
        ++count;
    }
    CHECK_GT(count, 0);
}
