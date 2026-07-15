#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <digraphx/mcf.hpp>
#include <vector>

using absl::flat_hash_map;
using std::vector;

// Helper: build the small spareTSV fixture (12 nodes, 9 primal + 3 spare + sink)
static auto build_small_fixture() -> std::pair<MCFGraph, MCFDemands> {
    // Van der Corput bases (2,3); T=12; eta=0.8 (1.6/(sqrt(12)-1))
    const double x_vals[] = {0.0, 0.5, 0.25, 0.75, 0.125, 0.625, 0.375, 0.875,
                             0.0625, 0.5625, 0.3125, 0.8125};
    const double y_vals[] = {0.0, 0.3333333333333333, 0.6666666666666666, 0.1111111111111111,
                             0.4444444444444444, 0.7777777777777777, 0.2222222222222222,
                             0.5555555555555556, 0.8888888888888888, 0.037037037037037035,
                             0.37037037037037035, 0.7037037037037037};

    const size_t t = 12;
    double eta = 0.8;

    MCFGraph g;
    for (size_t i = 0; i <= t; ++i) g[i] = {};

    for (size_t i = 0; i < t; ++i) {
        for (size_t j = i + 1; j < t; ++j) {
            double dx = x_vals[i] - x_vals[j];
            double dy = y_vals[i] - y_vals[j];
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= eta) {
                int64_t w = static_cast<int64_t>(dist * 100.0);  // trunc toward zero
                g[i][j] = MCFEdge{w, 4};
                g[j][i] = MCFEdge{w, 4};
            }
        }
    }
    for (size_t i = 9; i < t; ++i) {
        g[i][t] = MCFEdge{0, 4};
    }

    MCFDemands demands;
    for (size_t i = 0; i < 9; ++i) demands[i] = -1;
    demands[t] = 9;
    return {g, demands};
}

TEST_CASE("MCF simple chain") {
    MCFGraph g;
    g[0] = {{1, MCFEdge{1, 5}}};
    g[1] = {{2, MCFEdge{1, 5}}};
    g[2] = {};

    MCFDemands d{{0, -2}, {2, 2}};
    auto result = cycle_canceling_mcf(g, d);
    REQUIRE(result.has_value());
    CHECK_EQ(result->first, 4);
}

TEST_CASE("MCF two paths") {
    MCFGraph g;
    g[0] = {{1, MCFEdge{1, 5}}, {2, MCFEdge{5, 5}}};
    g[1] = {{3, MCFEdge{2, 5}}};
    g[2] = {{3, MCFEdge{1, 5}}};
    g[3] = {};

    MCFDemands d{{0, -2}, {3, 2}};
    auto result = cycle_canceling_mcf(g, d);
    REQUIRE(result.has_value());
    CHECK_EQ(result->first, 6);
}

TEST_CASE("MCF negative cycle cancellation") {
    MCFGraph g;
    g[0] = {{1, MCFEdge{10, 3}}, {2, MCFEdge{1, 5}}};
    g[1] = {{2, MCFEdge{1, 3}}};
    g[2] = {{3, MCFEdge{1, 5}}};
    g[3] = {{1, MCFEdge{-8, 3}}};

    MCFDemands d{{0, -3}, {3, 3}};
    auto result = cycle_canceling_mcf(g, d);
    REQUIRE(result.has_value());
    CHECK_EQ(result->first, -6);
}

TEST_CASE("MCF spareTSV fixture - exact flow match") {
    auto [g, demands] = build_small_fixture();
    auto result = cycle_canceling_mcf(g, demands);
    REQUIRE(result.has_value());

    auto [cost, flow] = result.value();
    // Cost must match Python/Rust reference (264)
    CHECK_EQ(cost, 264);

    // Flow assignment must match Python/Rust exactly
    CHECK_EQ(flow[0][9], 1);
    CHECK_EQ(flow[1][10], 1);
    CHECK_EQ(flow[2][10], 1);
    CHECK_EQ(flow[3][9], 1);
    CHECK_EQ(flow[4][10], 1);
    CHECK_EQ(flow[5][11], 1);
    CHECK_EQ(flow[6][9], 1);
    CHECK_EQ(flow[7][11], 1);
    CHECK_EQ(flow[8][10], 1);
    CHECK_EQ(flow[9][12], 3);
    CHECK_EQ(flow[10][12], 4);
    CHECK_EQ(flow[11][12], 2);
}

TEST_CASE("MCF infeasible") {
    MCFGraph g;
    g[0] = {{1, MCFEdge{1, 1}}};
    g[1] = {};

    MCFDemands d{{0, -2}, {1, 2}};
    auto result = cycle_canceling_mcf(g, d);
    CHECK(!result.has_value());
}

TEST_CASE("MCF empty graph") {
    MCFGraph g;
    MCFDemands d;
    auto result = cycle_canceling_mcf(g, d);
    REQUIRE(result.has_value());
    CHECK_EQ(result->first, 0);
}
