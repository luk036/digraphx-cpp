#include <doctest/doctest.h>

#include <cstdint>
#include <digraphx/css.hpp>
#include <map>
#include <utility>
#include <vector>

namespace {

    using TimingGraph = std::map<uint32_t, std::map<uint32_t, timing_edge>>;
    using RatioGraph = std::map<uint32_t, std::map<uint32_t, ratio_edge>>;
    using CPropGraph = std::map<uint32_t, std::map<uint32_t, cprop_edge>>;
    using WeightGraph = std::map<uint32_t, std::map<uint32_t, double>>;

    constexpr auto kHi = 1.0e6;

}  // namespace

TEST_CASE("Test min_clock_period") {
    const TimingGraph gra{
        {0, {{1, {7.0, 's'}}, {2, {2.0, 'h'}}}},
        {1, {{0, {4.0, 'h'}}, {2, {0.0, 'p'}}}},
        {2, {{0, {3.0, 's'}}}},
    };
    auto dist = std::vector<double>(3, 0.0);
    auto tcp = kHi;
    const auto cycle = min_clock_period(gra, tcp, dist);
    CHECK_FALSE(cycle.empty());
    CHECK_EQ(tcp, doctest::Approx(5.0));
}

TEST_CASE("Test even with raw weights") {
    constexpr auto tcp = 7.5;
    const WeightGraph gra{
        {0, {{2, 6.0}, {3, 6.0}}},
        {1, {{2, 6.0}, {4, tcp - 3.0}}},
        {2, {{0, tcp - 7.0}, {1, tcp - 9.0}, {3, 6.0}}},
        {3, {{4, 8.0}, {2, tcp - 6.0}, {0, tcp - 6.0}}},
        {4, {{3, tcp - 8.0}, {1, 3.0}}},
    };
    auto dist = std::vector<double>(5, 0.0);
    auto beta = kHi;
    const auto cost_time = [](double edge) -> std::pair<double, double> { return {edge, 1.0}; };
    const auto cycle = css_ratio(gra, beta, cost_time, dist);
    CHECK_FALSE(cycle.empty());
    CHECK_EQ(beta, doctest::Approx(1.0));
}

TEST_CASE("Test even") {
    constexpr auto tcp = 6.5;
    const TimingGraph gra{
        {0, {{1, {tcp - 7.0, 's'}}, {2, {2.0, 'h'}}}},
        {1, {{0, {4.0, 'h'}}, {2, {0.0, 'p'}}}},
        {2, {{0, {tcp - 3.0, 's'}}}},
    };
    auto dist = std::vector<double>(3, 0.0);
    auto beta = kHi;
    const auto cycle = even(gra, beta, dist);
    CHECK_FALSE(cycle.empty());
    CHECK_EQ(beta, doctest::Approx(1.5));
}

TEST_CASE("Test even on a multi-node graph") {
    constexpr auto tcp = 7.5;
    const TimingGraph gra{
        {0, {{2, {6.0, 'h'}}, {3, {6.0, 'h'}}}},
        {1, {{2, {6.0, 'h'}}, {5, {0.0, 'p'}}}},
        {2, {{0, {tcp - 7.0, 's'}}, {1, {tcp - 9.0, 's'}}, {3, {6.0, 's'}}}},
        {3, {{4, {8.0, 'h'}}, {2, {tcp - 6.0, 's'}}, {0, {tcp - 6.0, 's'}}}},
        {4, {{3, {tcp - 8.0, 's'}}, {5, {3.0, 'h'}}}},
        {5, {{4, {tcp - 3.0, 's'}}}},
    };
    auto dist = std::vector<double>(6, 0.0);
    auto beta = kHi;
    even(gra, beta, dist);
    CHECK_EQ(beta, doctest::Approx(1.0));
}

TEST_CASE("Test prop") {
    constexpr auto tcp = 7.5;
    const RatioGraph gra{
        {0, {{3, {6.0, 2.5}}, {2, {6.0, 3.1}}}},
        {1, {{2, {6.0, 2.5}}, {4, {tcp - 3.0, 1.1}}}},
        {2, {{0, {tcp - 7.0, 1.5}}, {1, {tcp - 9.0, 4.1}}, {3, {6.0, 2.5}}}},
        {3, {{0, {tcp - 6.0, 3.1}}, {2, {tcp - 6.0, 3.1}}, {4, {8.0, 1.5}}}},
        {4, {{1, {3.0, 1.0}}, {3, {tcp - 8.0, 4.1}}}},
    };
    auto dist = std::vector<double>(5, 0.0);
    auto beta = kHi;
    prop(gra, beta, dist);
    CHECK_EQ(beta, doctest::Approx(0.32258064516129037));
}

TEST_CASE("Test cprop") {
    const CPropGraph gra{
        {0, {{1, {4.0, 1.0}}}},
        {1, {{0, {2.0, 1.0}}}},
    };
    auto dist = std::vector<double>(2, 0.0);
    auto beta = kHi;
    cprop(gra, beta, dist);
    CHECK_EQ(beta, doctest::Approx(3.0));
}

TEST_CASE("Test min_clock_period on an alternative graph") {
    const TimingGraph gra{
        {0, {{1, {2.0, 's'}}, {2, {1.5, 'h'}}}},
        {1, {{2, {3.0, 's'}}, {0, {2.0, 'h'}}}},
        {2, {{0, {4.0, 's'}}, {1, {3.0, 'h'}}}},
    };
    auto dist = std::vector<double>(3, 0.0);
    auto tcp = kHi;
    const auto cycle = min_clock_period(gra, tcp, dist);
    CHECK_FALSE(cycle.empty());
    CHECK_EQ(tcp, doctest::Approx(3.0));
}

TEST_CASE("Test prop on an alternative graph") {
    constexpr auto tcp = 4.5;
    const RatioGraph gra{
        {0, {{1, {tcp - 2.0, 3.1}}, {2, {1.5, 0.7}}}},
        {1, {{2, {tcp - 3.0, 4.1}}, {0, {2.0, 2.2}}}},
        {2, {{0, {tcp - 4.0, 5.1}}, {1, {3.0, 1.5}}}},
    };
    auto dist = std::vector<double>(3, 0.0);
    auto beta = kHi;
    prop(gra, beta, dist);
    CHECK_EQ(beta, doctest::Approx(0.3448275862069039));
}

TEST_CASE("Test min_clock_period on a five-node ratio graph") {
    const TimingGraph gra{
        {0, {{3, {6.0, 's'}}, {2, {7.0, 's'}}}},
        {1, {{2, {9.0, 's'}}, {4, {3.0, 'h'}}}},
        {2, {{0, {6.0, 'h'}}, {1, {6.0, 'h'}}, {3, {6.0, 's'}}}},
        {3, {{4, {8.0, 's'}}, {0, {6.0, 'h'}}, {2, {6.0, 'h'}}}},
        {4, {{1, {3.0, 's'}}, {3, {8.0, 'h'}}}},
    };
    auto dist = std::vector<double>(5, 0.0);
    auto tcp = kHi;
    const auto cycle = min_clock_period(gra, tcp, dist);
    CHECK_FALSE(cycle.empty());
    CHECK_EQ(tcp, doctest::Approx(6.5));
}
