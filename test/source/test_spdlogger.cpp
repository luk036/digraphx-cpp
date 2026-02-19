// -*- coding: utf-8 -*-
#include <doctest/doctest.h>

#include <digraphx/logger.hpp>
#include <digraphx/neg_cycle.hpp>

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using std::list;
using std::pair;
using std::vector;

TEST_CASE("spdlogger integration test") {
    std::cout << "Testing spdlogger integration..." << std::endl;

    // Test 1: Basic logging with wrapper function
    digraphx::log_with_spdlog("Test 1: Basic logging");
    std::cout << "Logged a message to digraphx.log" << std::endl;

    // Test 2: Logging with negative cycle detection
    digraphx::log_with_spdlog("Test 2: Testing with negative cycle detection");

    using Graph = list<pair<size_t, list<pair<size_t, double>>>>;
    Graph gra{{0, {{1, 7.0}, {2, 5.0}}}, {1, {{0, 0.0}, {2, 3.0}}},
              {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}};

    NegCycleFinder ncf(gra);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>(gra.size(), 0.0);

    size_t cycle_count = 0;
    for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
        ++cycle_count;
    }

    digraphx::log_with_spdlog("Negative cycle detection completed");

    // Test 3: Direct spdlog usage (for comparison)
    try {
        auto logger = spdlog::basic_logger_mt("test_direct", "test_direct.log");
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        logger->flush_on(spdlog::level::info);
        logger->info("Direct spdlog test message");
        logger->flush();
        std::cout << "Direct spdlog test completed" << std::endl;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Direct spdlog error: " << ex.what() << std::endl;
    }

    // Test 4: Verify log files exist
    std::ifstream wrapper_log("digraphx.log");
    CHECK(wrapper_log.is_open());

    std::ifstream direct_log("test_direct.log");
    CHECK(direct_log.is_open());

    std::cout << "Check digraphx.log for the logged messages" << std::endl;
}