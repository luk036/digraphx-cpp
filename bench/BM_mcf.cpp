#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>

#include <digraphx/mcf.hpp>

// Van der Corput sequence
static double vdc(uint32_t n, uint32_t base) {
    double v = 0.0;
    double denom = 1.0;
    while (n > 0) {
        denom *= static_cast<double>(base);
        auto remainder = static_cast<double>(n % base);
        n /= base;
        v += remainder / denom;
    }
    return v;
}

// Build spareTSV-scale graph: 155 primal + 40 spare = 195 nodes
static auto build_spare_tsv_graph() -> std::pair<MCFGraph, MCFDemands> {
    constexpr size_t n = 155;
    constexpr size_t m = 40;
    constexpr size_t t = n + m;

    MCFGraph g;
    for (size_t i = 0; i <= t; ++i) g[i] = {};

    std::vector<std::pair<double, double>> pos;
    pos.reserve(t);
    for (size_t i = 0; i < t; ++i) {
        pos.emplace_back(vdc(static_cast<uint32_t>(i), 2),
                         vdc(static_cast<uint32_t>(i), 3));
    }

    auto n_int = static_cast<int>(std::sqrt(static_cast<double>(t)));
    double eta = 1.6 / static_cast<double>(n_int - 1);

    for (size_t i = 0; i < t; ++i) {
        for (size_t j = i + 1; j < t; ++j) {
            double dx = pos[i].first - pos[j].first;
            double dy = pos[i].second - pos[j].second;
            double d = std::sqrt(dx * dx + dy * dy);
            if (d <= eta) {
                int64_t w = static_cast<int64_t>(d * 100.0);
                g[i][j] = MCFEdge{w, 4};
                g[j][i] = MCFEdge{w, 4};
            }
        }
    }
    for (size_t i = n; i < t; ++i) {
        g[i][t] = MCFEdge{0, 4};
    }

    MCFDemands demands;
    for (size_t i = 0; i < n; ++i) demands[i] = -1;
    demands[t] = static_cast<int64_t>(n);
    return {g, demands};
}

int main() {
    auto [g, demands] = build_spare_tsv_graph();

    // Count edges
    size_t edge_count = 0;
    for (const auto& [u, nbrs] : g) edge_count += nbrs.size();
    std::printf("Graph: %zu nodes, %zu edges\n", g.size(), edge_count);

    // Warmup
    auto warmup = cycle_canceling_mcf(g, demands);
    if (!warmup) {
        std::printf("INFEASIBLE\n");
        return 1;
    }
    std::printf("Cost: %lld (expected 1108)\n",
                static_cast<long long>(warmup->first));

    // Benchmark
    constexpr int N_RUNS = 5;
    double total_ms = 0.0;
    for (int run = 0; run < N_RUNS; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = cycle_canceling_mcf(g, demands);
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration<double, std::milli>(end - start).count();
        total_ms += ms;

        if (!result) {
            std::printf("Run %d: INFEASIBLE\n", run);
            return 1;
        }
        std::printf("  Run %d: %.2f ms (cost=%lld)\n", run, ms,
                    static_cast<long long>(result->first));
    }

    std::printf("\nC++ average: %.2f ms (over %d runs)\n",
                total_ms / N_RUNS, N_RUNS);
    return 0;
}
