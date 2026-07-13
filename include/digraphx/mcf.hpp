#pragma once

/**
 * @file mcf.hpp
 * @brief Min-cost flow via cycle-cancellation descent (Bellman-Ford).
 *
 * Ported from digraphx.mcf (Python) and digraphx-rs::mcf (Rust).
 */

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/// Edge in the original cost / capacity graph.
struct MCFEdge {
    int64_t weight;
    int64_t capacity;
};

/// Edge in the residual graph.
struct ResidualEdge {
    int64_t cost;
    int64_t capacity;
    std::pair<size_t, size_t> orig;
    bool forward;
};

using MCFGraph = absl::flat_hash_map<size_t, absl::flat_hash_map<size_t, MCFEdge>>;
using MCFDemands = absl::flat_hash_map<size_t, int64_t>;
using MCFResidual = absl::flat_hash_map<size_t, absl::flat_hash_map<size_t, ResidualEdge>>;
using MCFFlow = absl::flat_hash_map<size_t, absl::flat_hash_map<size_t, int64_t>>;

// ---------------------------------------------------------------------------
// Feasible flow (greedy BFS)
// ---------------------------------------------------------------------------

static auto bfs_path(const MCFGraph& g, const MCFFlow& flow, size_t src,
                     const absl::flat_hash_set<size_t>& demand_set,
                     const MCFDemands& remaining) -> std::vector<size_t> {
    absl::flat_hash_set<size_t> visited;
    visited.insert(src);
    absl::flat_hash_map<size_t, size_t> parent;
    std::queue<size_t> queue;
    queue.push(src);

    while (!queue.empty()) {
        auto u = queue.front();
        queue.pop();

        if (demand_set.contains(u)) {
            auto it = remaining.find(u);
            if (it != remaining.end() && it->second > 0) {
                // Reconstruct path
                std::vector<size_t> path;
                auto cur = u;
                while (cur != src) {
                    path.push_back(cur);
                    cur = parent.at(cur);
                }
                path.push_back(src);
                std::reverse(path.begin(), path.end());
                return path;
            }
        }

        auto nbr_it = g.find(u);
        if (nbr_it != g.end()) {
            for (const auto& [v, edge] : nbr_it->second) {
                if (visited.contains(v)) continue;
                int64_t existing = 0;
                auto fu = flow.find(u);
                if (fu != flow.end()) {
                    auto fv = fu->second.find(v);
                    if (fv != fu->second.end()) existing = fv->second;
                }
                if (existing < edge.capacity) {
                    visited.insert(v);
                    parent[v] = u;
                    queue.push(v);
                }
            }
        }
    }
    return {};  // no path found
}

static auto find_feasible_flow(const MCFGraph& g, const MCFDemands& demands) -> MCFFlow {
    // Initialise flow to zero for every edge
    MCFFlow flow;
    for (const auto& [u, nbrs] : g) {
        auto& row = flow[u];
        for (const auto& [v, _] : nbrs) {
            row[v] = 0;
        }
    }

    MCFDemands remaining = demands;

    // Identify supply and demand nodes
    std::vector<size_t> supply_nodes;
    absl::flat_hash_set<size_t> demand_set;
    for (const auto& [node, d] : demands) {
        if (d < 0) supply_nodes.push_back(node);
        if (d > 0) demand_set.insert(node);
    }

    if (supply_nodes.empty() || demand_set.empty()) return flow;

    for (auto src : supply_nodes) {
        auto supply_amount = -remaining[src];
        while (supply_amount > 0) {
            auto path = bfs_path(g, flow, src, demand_set, remaining);
            if (path.empty()) return {};  // infeasible

            auto dst = path.back();

            // Bottleneck
            int64_t bottleneck = std::numeric_limits<int64_t>::max();
            for (size_t i = 0; i + 1 < path.size(); ++i) {
                auto u = path[i];
                auto v = path[i + 1];
                const auto& edge = g.at(u).at(v);
                auto existing = flow.at(u).at(v);
                bottleneck = std::min(bottleneck, edge.capacity - existing);
            }
            bottleneck = std::min({bottleneck, remaining.at(dst), supply_amount});
            if (bottleneck <= 0) return {};

            for (size_t i = 0; i + 1 < path.size(); ++i) {
                auto u = path[i];
                auto v = path[i + 1];
                flow[u][v] += bottleneck;
            }

            remaining[src] += bottleneck;
            remaining[dst] -= bottleneck;
            supply_amount -= bottleneck;
        }
    }

    return flow;
}

// ---------------------------------------------------------------------------
// Residual graph construction
// ---------------------------------------------------------------------------

static auto build_residual(const MCFGraph& g, const MCFFlow& flow) -> MCFResidual {
    MCFResidual residual;

    for (const auto& [u, nbrs] : g) {
        for (const auto& [v, data] : nbrs) {
            auto cap = data.capacity;
            auto wgt = data.weight;
            int64_t f = 0;
            auto fu = flow.find(u);
            if (fu != flow.end()) {
                auto fv = fu->second.find(v);
                if (fv != fu->second.end()) f = fv->second;
            }

            // Forward residual edge
            if (f < cap) {
                ResidualEdge edge{wgt, cap - f, {u, v}, true};
                auto& row = residual[u];
                auto prev = row.find(v);
                if (prev == row.end() || edge.cost < prev->second.cost) {
                    row[v] = std::move(edge);
                }
            }

            // Backward residual edge
            if (f > 0) {
                ResidualEdge edge{-wgt, f, {u, v}, false};
                auto& row = residual[v];
                auto prev = row.find(u);
                if (prev == row.end() || edge.cost < prev->second.cost) {
                    row[u] = std::move(edge);
                }
            }
        }
    }

    return residual;
}

static void update_residual_edge(MCFResidual& residual, const MCFGraph& g, const MCFFlow& flow,
                                  size_t u, size_t v) {
    // Remove stale entries
    auto ru = residual.find(u);
    if (ru != residual.end()) {
        ru->second.erase(v);
        if (ru->second.empty()) residual.erase(u);
    }
    auto rv = residual.find(v);
    if (rv != residual.end()) {
        rv->second.erase(u);
        if (rv->second.empty()) residual.erase(v);
    }

    // Rebuild
    auto gu = g.find(u);
    if (gu != g.end()) {
        auto gv = gu->second.find(v);
        if (gv != gu->second.end()) {
            auto cap = gv->second.capacity;
            auto wgt = gv->second.weight;
            int64_t f = 0;
            auto fu = flow.find(u);
            if (fu != flow.end()) {
                auto fv = fu->second.find(v);
                if (fv != fu->second.end()) f = fv->second;
            }
            if (f < cap) {
                residual[u][v] = ResidualEdge{wgt, cap - f, {u, v}, true};
            }
            if (f > 0) {
                residual[v][u] = ResidualEdge{-wgt, f, {u, v}, false};
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Negative cycle detection (Bellman-Ford)
// ---------------------------------------------------------------------------

static auto find_all_neg_cycles_bf(const MCFResidual& residual)
    -> std::vector<std::vector<ResidualEdge>> {
    // Collect all nodes
    absl::flat_hash_set<size_t> node_set;
    for (const auto& [u, nbrs] : residual) {
        node_set.insert(u);
        for (const auto& [v, _] : nbrs) {
            node_set.insert(v);
        }
    }
    std::vector<size_t> all_nodes(node_set.begin(), node_set.end());
    std::sort(all_nodes.begin(), all_nodes.end());

    auto n = all_nodes.size();
    if (n == 0) return {};

    // Map node → index
    absl::flat_hash_map<size_t, size_t> node_to_idx;
    for (size_t i = 0; i < n; ++i) node_to_idx[all_nodes[i]] = i;

    // Build flat edge list for efficient BF passes
    struct FlatEdge {
        size_t ui, vi;
        int64_t cost;
    };
    std::vector<FlatEdge> edge_list;
    for (const auto& [u, nbrs] : residual) {
        auto ui = node_to_idx[u];
        for (const auto& [v, e] : nbrs) {
            auto vi = node_to_idx[v];
            edge_list.push_back({ui, vi, e.cost});
        }
    }

    std::vector<int64_t> dist(n, 0);
    std::vector<size_t> pred_node(n, SIZE_MAX);
    std::vector<bool> updated_in_last(n, false);

    for (size_t pass = 0; pass < n; ++pass) {
        bool changed = false;
        std::fill(updated_in_last.begin(), updated_in_last.end(), false);
        for (const auto& fe : edge_list) {
            auto nd = dist[fe.ui] + fe.cost;
            if (nd < dist[fe.vi]) {
                dist[fe.vi] = nd;
                pred_node[fe.vi] = fe.ui;
                changed = true;
                if (pass == n - 1) updated_in_last[fe.vi] = true;
            }
        }
        if (!changed && pass < n - 1) return {};
    }

    bool any_updated = false;
    for (auto b : updated_in_last) if (b) { any_updated = true; break; }
    if (!any_updated) return {};

    // Build predecessor edge map for cycle reconstruction via a second BF pass
    absl::flat_hash_map<size_t, std::pair<size_t, ResidualEdge>> pred_edge;
    {
        std::vector<int64_t> d2(n, 0);
        for (size_t pass = 0; pass < n; ++pass) {
            for (const auto& [u, nbrs] : residual) {
                auto ui = node_to_idx[u];
                for (const auto& [v, e] : nbrs) {
                    auto vi = node_to_idx[v];
                    auto nd = d2[ui] + e.cost;
                    if (nd < d2[vi]) {
                        d2[vi] = nd;
                        pred_edge[v] = {u, e};
                    }
                }
            }
        }
    }

    // Extract cycles
    std::vector<std::vector<ResidualEdge>> cycles;
    absl::flat_hash_set<std::pair<size_t, size_t>> yielded_orig;
    absl::flat_hash_set<size_t> visited_trace;

    for (auto start_node : all_nodes) {
        auto si = node_to_idx[start_node];
        if (!updated_in_last[si]) continue;
        if (visited_trace.contains(start_node)) continue;

        // Trace back to find cycle start
        absl::flat_hash_set<size_t> trace_visited;
        auto u = start_node;
        while (!trace_visited.contains(u)) {
            trace_visited.insert(u);
            auto it = pred_edge.find(u);
            if (it == pred_edge.end()) break;
            u = it->second.first;
        }
        auto cycle_start = u;

        // Reconstruct cycle edges
        std::vector<ResidualEdge> cycle_edges;
        u = cycle_start;
        bool valid = true;
        for (size_t iter = 0; iter < n + 1; ++iter) {
            auto it = pred_edge.find(u);
            if (it == pred_edge.end()) { valid = false; break; }
            auto& [prev, edge_ref] = it->second;
            if (!yielded_orig.contains(edge_ref.orig)) {
                cycle_edges.push_back(edge_ref);
            }
            u = prev;
            if (u == cycle_start) break;
        }
        if (!valid) continue;

        std::reverse(cycle_edges.begin(), cycle_edges.end());

        if (!cycle_edges.empty()) {
            int64_t total_cost = 0;
            for (const auto& e : cycle_edges) total_cost += e.cost;
            if (total_cost < 0) {
                for (const auto& e : cycle_edges) {
                    yielded_orig.insert(e.orig);
                    visited_trace.insert(e.orig.first);
                }
                cycles.push_back(std::move(cycle_edges));
            }
        }
    }

    return cycles;
}

// ---------------------------------------------------------------------------
// Main MCF solver
// ---------------------------------------------------------------------------

/// Solve min-cost flow using cycle-cancellation descent.
///
/// @param g       Graph: {u: {v: MCFEdge{weight, capacity}}}
/// @param demands {node: demand} (negative = supply, positive = demand)
/// @return (total_cost, flow) or nothing if infeasible.
inline auto cycle_canceling_mcf(const MCFGraph& g, const MCFDemands& demands)
    -> std::optional<std::pair<int64_t, MCFFlow>> {

    // Stage 1: find feasible initial flow
    auto flow = find_feasible_flow(g, demands);
    if (flow.empty() && !demands.empty()) {
        // Check if actually infeasible or just no flow needed
        bool has_demand = false;
        for (const auto& [_, d] : demands) {
            if (d != 0) { has_demand = true; break; }
        }
        if (has_demand) return std::nullopt;
    }

    // Stage 2: cancel negative-cost residual cycles
    MCFResidual residual;

    while (true) {
        if (residual.empty()) {
            residual = build_residual(g, flow);
            if (residual.empty()) break;
        }

        auto cycles = find_all_neg_cycles_bf(residual);
        if (cycles.empty()) break;

        bool cancelled = false;
        for (const auto& cycle_edges : cycles) {
            int64_t bottleneck = std::numeric_limits<int64_t>::max();
            for (const auto& e : cycle_edges)
                bottleneck = std::min(bottleneck, e.capacity);
            if (bottleneck <= 0) continue;

            // Apply flow changes
            for (const auto& e : cycle_edges) {
                auto [u_orig, v_orig] = e.orig;
                if (e.forward)
                    flow[u_orig][v_orig] += bottleneck;
                else
                    flow[u_orig][v_orig] -= bottleneck;
            }

            // Update residual edges
            absl::flat_hash_set<std::pair<size_t, size_t>> seen;
            for (const auto& e : cycle_edges) {
                if (seen.insert(e.orig).second)
                    update_residual_edge(residual, g, flow, e.orig.first, e.orig.second);
            }

            cancelled = true;
            break;
        }

        if (!cancelled) break;
    }

    // Compute total cost
    int64_t total_cost = 0;
    for (const auto& [u, nbrs] : g) {
        for (const auto& [v, data] : nbrs) {
            int64_t f = 0;
            auto fu = flow.find(u);
            if (fu != flow.end()) {
                auto fv = fu->second.find(v);
                if (fv != fu->second.end()) f = fv->second;
            }
            total_cost += f * data.weight;
        }
    }

    return std::make_pair(total_cost, std::move(flow));
}
