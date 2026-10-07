#define ANKERL_NANOBENCH_IMPLEMENT
#include <absl/container/flat_hash_map.h>
#include <fmt/format.h>
#include <nanobench.h>

#include <digraphx/neg_cycle.hpp>
#include <list>
#include <mywheel/map_adapter.hpp>
#include <string>
#include <utility>
#include <vector>

using std::list;
using std::pair;
using std::vector;

// Reverse chain 0 <- 1 <- ... <- n-1 with a long negative cycle back to 0.
// The relaxation must propagate one hop per pass, so the search needs O(n)
// passes - the regime where caching the edge weights pays off.  Weights are
// looked up in a hash table to emulate a non-trivial weight function.
struct CacheBenchGraph {
    vector<list<pair<size_t, size_t>>> adj;
    absl::flat_hash_map<size_t, double> weights;
};

static auto build_graph(size_t n_nodes) -> CacheBenchGraph {
    vector<list<pair<size_t, size_t>>> adj(n_nodes);
    absl::flat_hash_map<size_t, double> weights;
    size_t eid = 0;
    for (size_t i = 1; i < n_nodes; ++i) {
        adj[i].emplace_back(i - 1, eid);
        weights.emplace(eid, 1.0);
        ++eid;
    }
    adj[0].emplace_back(n_nodes - 1, eid);
    weights.emplace(eid, -static_cast<double>(n_nodes));
    return {std::move(adj), std::move(weights)};
}

int main() {
    fmt::print("=== digraphx-cpp: NegCycleFinder multi-pass weight cache ===\n");
    const size_t sizes[] = {500, 1000, 2000};

    ankerl::nanobench::Bench bench;
    bench.title("NegCycleFinder (multi-pass)").unit("op").warmup(2).epochs(5).minEpochIterations(3);

    for (auto n : sizes) {
        auto bg = build_graph(n);
        auto g = MapConstAdapter(bg.adj);
        auto get_weight = [&weights = bg.weights](const auto& edge) { return weights.at(edge); };
        NegCycleFinder ncf(g);
        vector<double> dist(n, 0.0);
        vector<size_t> cycle_edges;
        for (auto const& ci : ncf.howard(dist, get_weight)) {
            cycle_edges = ci;
        }
        bool found = !cycle_edges.empty();
        double total_weight = 0.0;
        for (auto e : cycle_edges) total_weight += bg.weights.at(e);
        fmt::print("Nodes={:<6} Edges={:<6} Found={:<4} Weight={:.0f}\n", n, bg.weights.size(),
                   found ? "yes" : "no", total_weight);

        bench.run("n=" + std::to_string(n), [&] {
            vector<double> d(n, 0.0);
            NegCycleFinder ncf2(g);
            vector<size_t> cycle;
            for (auto const& ci : ncf2.howard(d, get_weight)) {
                cycle = ci;
            }
            ankerl::nanobench::doNotOptimizeAway(cycle);
        });
    }
    return 0;
}
