#include <doctest/doctest.h>

#include <cstdint>  // for uint32_t
#include <digraphx/parametric.hpp>
#include <list>
#include <unordered_map>
#include <vector>

template <typename DiGraph> void run_parametric_test(const DiGraph& digraph) {
    auto distance = [](double r, const auto& edge) { return edge - r; };

    auto zero_cancel = [](const auto& cycle) {
        double cost = 0;
        for (const auto& edge : cycle) {
            cost += edge;
        }
        return cost / static_cast<double>(cycle.size());
    };

    auto dist = std::vector<double>(digraph.size(), 0.0);
    double r = 100.0;
    max_parametric(digraph, r, distance, zero_cancel, dist, 0.0);
    CHECK_EQ(r, 1.0);
}

TEST_CASE("Test Parametric Search") {
    SUBCASE("list of lists") {
        std::list<std::pair<size_t, std::list<std::pair<size_t, int>>>> digraph{
            {0, {{1, 5}, {2, 1}}}, {1, {{0, 1}, {2, 1}}}, {2, {{1, 1}, {0, 1}}}};
        run_parametric_test(digraph);
    }

    SUBCASE("dict of list's") {
        const std::unordered_map<uint32_t, std::list<std::pair<uint32_t, uint32_t>>> digraph{
            {0, {{1, 0}, {2, 1}}}, {1, {{0, 2}, {2, 3}}}, {2, {{1, 4}, {0, 5}}}};
        run_parametric_test(digraph);
    }
}
