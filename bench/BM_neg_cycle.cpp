#include <chrono>
#include <cstdint>
#include <cstdio>
#include <list>
#include <utility>
#include <vector>

#include <digraphx/neg_cycle.hpp>
#include <mywheel/map_adapter.hpp>

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
    std::printf("%-12s %-10s %-6s %-8s %-12s %-8s\n",
                "Nodes", "Edges", "Found", "Weight", "Avg(ms)", "Rel");
    const size_t sizes[] = {20000, 50000, 100000, 200000, 500000, 1000000};
    const int n_runs = 5;
    double ref_ms = 0.0;
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
        if (found) for (auto w : cycle_edges) total_weight += w;
        double total_ms = 0.0;
        for (int run = 0; run < n_runs; ++run) {
            vector<double> d(bg.adj.size(), 0.0);
            auto start = std::chrono::high_resolution_clock::now();
            NegCycleFinder ncf2(g);
            for (auto const& ci : ncf2.howard(d, get_weight)) { (void)ci; }
            auto end = std::chrono::high_resolution_clock::now();
            total_ms += std::chrono::duration<double, std::milli>(end - start).count();
        }
        double avg = total_ms / n_runs;
        if (ref_ms == 0.0) ref_ms = avg;
        std::printf("%-12zu %-10zu %-6s %-8.0f %-12.2f %-8.1f\n",
                    n, bg.edge_count, found ? "yes" : "no", total_weight, avg, avg / ref_ms);
    }
    return 0;
}
