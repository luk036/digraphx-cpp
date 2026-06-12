// test min_parametric() free function and MinParametricSolver edge cases
#include <doctest/doctest.h>

#include <cstdint>
#include <digraphx/min_parametric_q.hpp>
#include <limits>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

using std::list;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

TEST_CASE("Test min_parametric free function with list digraph (no neg cycle)") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 7.0}, {2, 5.0}}},
        {1, {{0, 0.0}, {2, 3.0}}},
        {2, {{1, 1.0}, {0, 2.0}}}};

    auto distance = [](double r, const auto& edge) -> double { return edge - r; };
    auto zero_cancel = [](const auto& cycle) -> double {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge;
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = vector<double>(digraph.size(), 0.0);
    auto [ratio, cycle] = min_parametric(digraph, 0.0, distance, zero_cancel, dist, 0.0);

    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("Test min_parametric free function with dict digraph (no neg cycle)") {
    unordered_map<string, unordered_map<string, unordered_map<string, double>>> digraph{
        {"a0",
         {{"a1", {{"cost", 7.0}, {"time", 1.0}}}, {"a2", {{"cost", 5.0}, {"time", 1.0}}}}},
        {"a1",
         {{"a0", {{"cost", 0.0}, {"time", 1.0}}}, {"a2", {{"cost", 3.0}, {"time", 1.0}}}}},
        {"a2",
         {{"a1", {{"cost", 1.0}, {"time", 1.0}}}, {"a0", {{"cost", 2.0}, {"time", 1.0}}}}}};

    auto distance = [](double r, const auto& edge) -> double {
        return edge.at("cost") - r * edge.at("time");
    };
    auto zero_cancel = [](const auto& cycle) -> double {
        double total_cost = 0.0;
        double total_time = 0.0;
        for (const auto& edge : cycle) {
            total_cost += edge.at("cost");
            total_time += edge.at("time");
        }
        return total_cost / total_time;
    };

    unordered_map<string, double> dist;
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    auto [ratio, cycle] = min_parametric(digraph, 0.0, distance, zero_cancel, dist, 0.0);

    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("Test min_parametric free function pick_one_only") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 7.0}, {2, 5.0}}},
        {1, {{0, 0.0}, {2, 3.0}}},
        {2, {{1, 1.0}, {0, 2.0}}}};

    auto distance = [](double r, const auto& edge) -> double { return edge - r; };
    auto zero_cancel = [](const auto& cycle) -> double {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge;
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = vector<double>(digraph.size(), 0.0);
    auto [ratio, cycle] = min_parametric(digraph, 0.0, distance, zero_cancel, dist, 0.0, true);

    CHECK_EQ(ratio, 0.0);
}

TEST_CASE("Test min_parametric free function with negative cycle") {
    // min_parametric seeks INCREASING ratios: from r_start it finds cycles
    // whose zero_cancel ratio > r_max.  Starting at r=0, all found cycle
    // ratios are negative, so no improvement occurs — ratio stays at 0.
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{2, 1.0}}}, {2, {{0, -3.0}}}};

    auto distance = [](double r, const auto& edge) -> double { return edge - r; };
    auto zero_cancel = [](const auto& cycle) -> double {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge;
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = vector<double>(digraph.size(), 0.0);
    auto [ratio, cycle] = min_parametric(digraph, 0.0, distance, zero_cancel, dist, 0.0);

    // The algorithm still iterates howard_pred/succ internally; it just
    // can't *improve* the ratio.  Coverage is gained for the iteration
    // paths even though the result matches the initial value.
    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("Test MinParametricSolver with pick_one_only on negative cycle") {
    unordered_map<string, unordered_map<string, unordered_map<string, double>>> digraph{
        {"a0", {{"a1", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"a1", {{"a2", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"a2", {{"a0", {{"cost", -4.0}, {"time", 1.0}}}}}};

    class TestAPI : public MinParametricAPI<string, unordered_map<string, double>, double> {
      public:
        auto distance(const double& ratio,
                      const unordered_map<string, double>& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<unordered_map<string, double>>& cycle) -> double override {
            double total_cost = 0.0;
            double total_time = 0.0;
            for (const auto& edge : cycle) {
                total_cost += edge.at("cost");
                total_time += edge.at("time");
            }
            return total_cost / total_time;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0.0;
    }

    auto api = TestAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) -> bool { return true; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok, true);

    // Same reasoning: cycle ratios are < 0, so no improvement over r=0.
    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("Test MinParametricSolver with restrictive update_ok blocks detection") {
    list<pair<size_t, list<pair<size_t, double>>>> digraph{
        {0, {{1, 1.0}}}, {1, {{2, 1.0}}}, {2, {{0, -3.0}}}};

    class ListAPI : public MinParametricAPI<size_t, double, double> {
      public:
        auto distance(const double& ratio, const double& edge) -> double override {
            return edge - ratio;
        }
        auto zero_cancel(const vector<double>& cycle) -> double override {
            double cost = 0;
            for (const auto& edge : cycle) {
                cost += edge;
            }
            return cost / static_cast<double>(cycle.size());
        }
    };

    auto dist = vector<double>(digraph.size(), 0.0);
    auto api = ListAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double&, const double&) -> bool { return false; };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);

    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}

TEST_CASE("Test MinParametricSolver with negative cycle and infinity init") {
    unordered_map<string, unordered_map<string, unordered_map<string, double>>> digraph{
        {"a0", {{"a1", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"a1", {{"a2", {{"cost", 1.0}, {"time", 1.0}}}}},
        {"a2", {{"a0", {{"cost", -4.0}, {"time", 1.0}}}}}};

    class MyAPI : public MinParametricAPI<string, unordered_map<string, double>, double> {
      public:
        auto distance(const double& ratio,
                      const unordered_map<string, double>& edge) -> double override {
            return edge.at("cost") - ratio * edge.at("time");
        }
        auto zero_cancel(const vector<unordered_map<string, double>>& cycle) -> double override {
            double total_cost = 0.0;
            double total_time = 0.0;
            for (const auto& edge : cycle) {
                total_cost += edge.at("cost");
                total_time += edge.at("time");
            }
            return total_cost / total_time;
        }
    };

    unordered_map<string, double> dist;
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = std::numeric_limits<double>::infinity();
    }

    auto api = MyAPI{};
    auto solver = MinParametricSolver<decltype(digraph), double, double>{digraph, api};
    auto update_ok = [](const double& current, const double& new_val) -> bool {
        return current > new_val;
    };

    auto [ratio, cycle] = solver.run(dist, 0.0, update_ok);

    // Same reasoning: cycle ratios are negative, cannot improve from r=0.
    CHECK_EQ(ratio, 0.0);
    CHECK(cycle.empty());
}
