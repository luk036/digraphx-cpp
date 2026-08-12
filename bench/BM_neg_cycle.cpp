#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <cstdint>
#include <cstdio>
#include <digraphx/neg_cycle.hpp>
#include <list>
#include <mywheel/map_adapter.hpp>
#include <string>
#include <utility>
#include <vector>

using std::list;
using std::pair;
using std::vector;

struct BenchGraph {
    vector<list<pair<size_t, double>>> adj;
    size_t edge_count;
};

static auto build_graph(size_t n_nodes, int k = 3) -> BenchGraph {
    vector<list<pair<size_t, double>>> g(n_nodes);
    size_t edge_count = 0;
    for (size_t i = 0; i < n_nodes; ++i) {
        for (int d = 1; d <= k; ++d) {
            auto j = (i + static_cast<size_t>(d)) % n_nodes;
            double w = static_cast<double>(((i + 1) * 7 + (j + 1) * 13) % 100 + 1);
            g[i].emplace_back(j, w);
            edge_count += 1;
        }
    }
    if (n_nodes > 2) {
        g[0].emplace_back(1, -5.0);
        g[1].emplace_back(2, -5.0);
        g[2].emplace_back(0, -5.0);
        edge_count += 3;
    }
    return {std::move(g), edge_count};
}

int main() {
    std::printf("=== digraphx-cpp: NegCycleFinder (Howard) ===\n");
    const size_t sizes[] = {20000, 50000, 100000, 200000, 500000, 1000000};

    ankerl::nanobench::Bench bench;
    bench.title("NegCycleFinder (Howard)")
        .unit("op")
        .warmup(3)
        .epochs(10)
        .minEpochIterations(5);

    for (auto n : sizes) {
        auto bg = build_graph(n);
        auto g = MapConstAdapter(bg.adj);
        auto get_weight = [](const auto& edge) { return edge; };
        vector<double> dist(bg.adj.size(), 0.0);
        NegCycleFinder ncf(g);
        vector<double> cycle_edges;
        double total_weight = 0.0;
        for (auto const& ci : ncf.howard(dist, get_weight)) {
            cycle_edges = ci;
        }
        bool found = !cycle_edges.empty();
        if (found)
            for (auto w : cycle_edges) total_weight += w;
        std::printf("Nodes=%-8zu Edges=%-10zu Found=%-4s Weight=%.0f\n", n, bg.edge_count,
                    found ? "yes" : "no", total_weight);

        bench.run("n=" + std::to_string(n), [&] {
            vector<double> d(bg.adj.size(), 0.0);
            NegCycleFinder ncf2(g);
            vector<double> cycle;
            for (auto const& ci : ncf2.howard(d, get_weight)) {
                cycle = ci;
            }
            ankerl::nanobench::doNotOptimizeAway(cycle);
        });
    }
    return 0;
}
