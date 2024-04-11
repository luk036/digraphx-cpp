#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

template <typename Node, typename Edge, typename Domain> class NegCycleFinder {
  public:
    using Cycle = std::vector<Edge>;
    using Graph = std::unordered_map<Node, std::unordered_map<Node, Edge>>;
    using Dist = std::unordered_map<Node, Domain>;
    using GetWeight = std::function<Domain(Edge)>;

    NegCycleFinder(const Graph& digraph) : digraph(digraph) {}

    bool relax(Dist& dist, const GetWeight& get_weight) {
        bool changed = false;
        for (const auto& [utx, neighbors] : digraph) {
            for (const auto& [vtx, edge] : neighbors) {
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

    bool is_negative(const Node& handle, const Dist& dist, const GetWeight& get_weight) {
        Node vtx = handle;
        while (true) {
            const auto& [utx, edge] = pred[vtx];
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

    Cycle cycle_list(const Node& handle) {
        Node vtx = handle;
        Cycle cycle;
        while (true) {
            const auto& [utx, edge] = pred[vtx];
            cycle.push_back(edge);
            vtx = utx;
            if (vtx == handle) {
                break;
            }
        }
        return cycle;
    }

    std::vector<Cycle> howard(Dist& dist, const GetWeight& get_weight) {
        pred.clear();
        std::vector<Cycle> cycles;
        while (relax(dist, get_weight)) {
            for (const auto& vtx : find_cycle()) {
                assert(is_negative(vtx, dist, get_weight));
                cycles.push_back(cycle_list(vtx));
            }
        }
        return cycles;
    }

  private:
    Graph digraph;
    std::unordered_map<Node, std::pair<Node, Edge>> pred;

    std::vector<Node> find_cycle() {
        std::unordered_map<Node, Node> visited;
        std::vector<Node> cycles;
        for (const auto& [vtx, _] : digraph) {
            if (visited.find(vtx) == visited.end()) {
                Node utx = vtx;
                visited[utx] = vtx;
                while (pred.find(utx) != pred.end()) {
                    const auto& [next_utx, _] = pred[utx];
                    if (visited.find(next_utx) != visited.end()) {
                        if (visited[next_utx] == vtx) {
                            cycles.push_back(next_utx);
                        }
                        break;
                    }
                    visited[next_utx] = vtx;
                    utx = next_utx;
                }
            }
        }
        return cycles;
    }
};

int main() {
    using Node = std::string;
    using Edge = int;
    using Domain = int;
    using Cycle = std::vector<Edge>;
    using Graph = std::unordered_map<Node, std::unordered_map<Node, Edge>>;
    using Dist = std::unordered_map<Node, Domain>;
    using GetWeight = std::function<Domain(Edge)>;

    Graph digraph = {{"a0", {{"a1", 7}, {"a2", 5}}},
                     {"a1", {{"a0", 0}, {"a2", 3}}},
                     {"a2", {{"a1", 1}, {"a0", 2}}}};

    Dist dist;
    for (const auto& [vtx, _] : digraph) {
        dist[vtx] = 0;
    }

    GetWeight get_weight = [](Edge edge) { return edge; };

    NegCycleFinder<Node, Edge, Domain> finder(digraph);
    std::vector<Cycle> cycles = finder.howard(dist, get_weight);

    for (const auto& cycle : cycles) {
        std::cout << "Cycle: ";
        for (const auto& edge : cycle) {
            std::cout << edge << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
