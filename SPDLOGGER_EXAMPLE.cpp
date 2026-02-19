// -*- coding: utf-8 -*-
/**
 * @file SPDLOGGER_EXAMPLE.cpp
 * @brief Example demonstrating spdlog integration with DiGraphX
 *
 * This example shows how to use the digraphx::log_with_spdlog() wrapper function
 * to log messages to a file, as well as how to use direct spdlog API for
 * more advanced logging scenarios.
 */

#include <digraphx/logger.hpp>
#include <digraphx/neg_cycle.hpp>

#include <list>
#include <vector>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using std::list;
using std::pair;
using std::vector;
using std::cout;
using std::endl;

int main() {
    cout << "========================================" << endl;
    cout << "  DiGraphX Spdlogger Example            " << endl;
    cout << "========================================" << endl;

    // Example 1: Basic logging with wrapper function
    cout << "\nExample 1: Basic logging" << endl;
    cout << "----------------------------" << endl;
    digraphx::log_with_spdlog("Application started");
    digraphx::log_with_spdlog("Initializing graph...");

    // Example 2: Logging with negative cycle detection
    cout << "\nExample 2: Logging with negative cycle detection" << endl;
    cout << "------------------------------------------------" << endl;

    using Graph = list<pair<size_t, list<pair<size_t, double>>>>;

    // Create a simple graph
    Graph gra{
        {0, {{1, 7.0}, {2, 5.0}}},
        {1, {{0, 0.0}, {2, 3.0}}},
        {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}
    };

    cout << "Created graph with " << gra.size() << " nodes" << endl;
    digraphx::log_with_spdlog("Created graph with positive weights");

    // Perform negative cycle detection
    cout << "Running negative cycle detection..." << endl;
    NegCycleFinder ncf(gra);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>(gra.size(), 0.0);

    size_t cycle_count = 0;
    for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
        ++cycle_count;
    }

    cout << "Negative cycles found: " << cycle_count << endl;
    digraphx::log_with_spdlog("Negative cycle detection completed");

    // Example 3: Using direct spdlog for advanced features
    cout << "\nExample 3: Direct spdlog usage" << endl;
    cout << "---------------------------------" << endl;

    try {
        // Create a logger with custom configuration
        auto logger = spdlog::basic_logger_mt("example_logger", "example.log");
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
        logger->flush_on(spdlog::level::info);

        // Log at different levels
        logger->debug("Debug message - detailed information");
        logger->info("Info message - general information");
        logger->warn("Warning message - potential issues");
        logger->error("Error message - error conditions");
        logger->critical("Critical message - severe errors");

        logger->flush();
        cout << "Direct spdlog test completed" << endl;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Direct spdlog error: " << ex.what() << endl;
    }

    // Example 4: Multiple log levels
    cout << "\nExample 4: Testing different log levels" << endl;
    cout << "-----------------------------------------" << endl;

    digraphx::log_with_spdlog("Final message - application completed successfully");

    cout << "\n========================================" << endl;
    cout << "Summary" << endl;
    cout << "========================================" << endl;
    cout << "Check the following log files:" << endl;
    cout << "  - digraphx.log (wrapper function)" << endl;
    cout << "  - example.log (direct spdlog)" << endl;
    cout << "\nAll examples completed successfully!" << endl;
    cout << "========================================" << endl;

    return 0;
}
