#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

template <typename Node, typename Edge, typename Domain> class NegCycleFinder {
    using Cycle = std::vector<Edge>;
    using Graph = std::map<Node, std::map<Node, Edge>>;
    using DistMap = std::unordered_map<Node, Domain>;

    std::unordered_map<Node, std::pair<Node, Edge>> pred;
    Graph digraph;

  public:
    NegCycleFinder(const Graph& gra) : digraph(gra) {}

    std::vector<Node> find_cycle() {
        std::unordered_map<Node, Node> visited;
        std::vector<Node> cycle;

        for (const auto& vtx : digraph) {
            if (visited.find(vtx.first) == visited.end()) {
                Node utx = vtx.first;
                while (true) {
                    visited[utx] = vtx.first;
                    if (pred.find(utx) == pred.end()) {
                        break;
                    }
                    utx = pred[utx].first;
                    if (visited.find(utx) != visited.end()) {
                        if (visited[utx] == vtx.first) {
                            cycle.push_back(utx);
                        }
                        break;
                    }
                }
            }
        }

        return cycle;
    }

    bool relax(DistMap& dist, std::function<Domain(Edge)> get_weight) {
        bool changed = false;
        for (const auto& [utx, nbrs] : digraph) {
            for (const auto& [vtx, edge] : nbrs) {
                Domain distance = dist[utx] + get_weight(edge);
                if (dist[vtx] > distance) {
                    dist[vtx] = distance;
                    pred[vtx] = std::make_pair(utx, edge);
                    changed = true;
                }
            }
        }
        return changed;
    }

    std::vector<Cycle> howard(DistMap& dist, std::function<Domain(Edge)> get_weight) {
        std::vector<Cycle> cycles;
        pred.clear();

        while (relax(dist, get_weight)) {
            std::vector<Node> cycle = find_cycle();
            if (!cycle.empty()) {
                assert(is_negative(cycle[0], dist, get_weight));
                cycles.push_back(cycle);
            } else {
                break;
            }
        }

        return cycles;
    }

    Cycle cycle_list(Node handle) {
        Node vtx = handle;
        Cycle cycle;
        while (true) {
            auto [utx, edge] = pred[vtx];
            cycle.push_back(edge);
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return cycle;
    }

    bool is_negative(Node handle, DistMap& dist, std::function<Domain(Edge)> get_weight) {
        Node vtx = handle;
        while (true) {
            auto [utx, edge] = pred[vtx];
            if (dist[vtx] > dist[utx] + get_weight(edge)) {
                return true;
            }
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return false;
    }
};

template <typename Node, typename Edge, typename Ratio> class ParametricAPI {
  public:
    virtual Ratio distance(Ratio ratio, Edge edge) = 0;
    virtual Ratio zero_cancel(std::vector<Edge> cycle) = 0;
};

template <typename Node, typename Edge, typename Ratio> class MaxParametricSolver {
    using Graph = std::map<Node, std::map<Node, Edge>>;
    using DistMap = std::unordered_map<Node, typename Ratio::value_type>;
    using Cycle = std::vector<Edge>;

    NegCycleFinder<Node, Edge, typename Ratio::value_type> ncf;
    ParametricAPI<Node, Edge, Ratio>& omega;

  public:
    MaxParametricSolver(const Graph& gra, ParametricAPI<Node, Edge, Ratio>& api)
        : ncf(gra), omega(api) {}

    std::pair<Ratio, Cycle> run(DistMap& dist, Ratio ratio) {
        using D = typename Ratio::value_type;
        auto get_weight = [&](Edge e) { return D(omega.distance(ratio, e)); };

        Ratio r_min = ratio;
        Cycle c_min;

        while (true) {
            for (const auto& ci : ncf.howard(dist, get_weight)) {
                Ratio ri = omega.zero_cancel(ci);
                if (r_min > ri) {
                    r_min = ri;
                    c_min = ci;
                }
            }
            if (r_min >= ratio) {
                break;
            }

            ratio = r_min;
        }

        return std::make_pair(ratio, c_min);
    }
};

template <typename Node, typename Edge, typename Ratio> class CycleRatioAPI
    : public ParametricAPI<Node, std::map<std::string, typename Ratio::value_type>, Ratio> {
    using Graph = std::map<Node, std::map<Node, std::map<std::string, typename Ratio::value_type>>>;

    Graph gra;
    using K = typename Ratio::value_type;

  public:
    CycleRatioAPI(const Graph& gra) : gra(gra) {}

    Ratio distance(Ratio ratio, std::map<std::string, typename Ratio::value_type> edge) override {
        return K(edge["cost"]) - ratio * edge["time"];
    }

    Ratio zero_cancel(
        std::vector<std::map<std::string, typename Ratio::value_type>> cycle) override {
        K total_cost = 0;
        K total_time = 0;
        for (const auto& edge : cycle) {
            total_cost += edge["cost"];
            total_time += edge["time"];
        }
        return K(total_cost) / total_time;
    }
};

template <typename Node, typename Edge, typename Ratio> class MinCycleRatioSolver {
    using Graph = std::map<Node, std::map<Node, std::map<std::string, typename Ratio::value_type>>>;
    using DistMap = std::unordered_map<Node, typename Ratio::value_type>;
    using Cycle = std::vector<Edge>;

    Graph gra;

  public:
    MinCycleRatioSolver(const Graph& gra) : gra(gra) {}

    std::pair<Ratio, Cycle> run(DistMap& dist, Ratio r0) {
        CycleRatioAPI<Node, Edge, Ratio> omega(gra);
        MaxParametricSolver<Node, Edge, Ratio> solver(gra, omega);
        return solver.run(dist, r0);
    }
};
