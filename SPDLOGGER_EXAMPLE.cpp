// -*- coding: utf-8 -*-
/**
 * @file SPDLOGGER_EXAMPLE.cpp
 * @brief Example demonstrating spdlog integration with DiGraphX
 *
 * This example shows how to use the digraphx::log_with_spdlog() wrapper function
 * to log messages to a file, as well as how to use direct spdlog API for
 * more advanced logging scenarios.
 */

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <digraphx/logger.hpp>
#include <digraphx/neg_cycle.hpp>
#include <iostream>
#include <list>
#include <vector>

using std::cout;
using std::endl;
using std::list;
using std::pair;
using std::vector;

int main() {
    cout << "========================================" << '\n';
    cout << "  DiGraphX Spdlogger Example            " << '\n';
    cout << "========================================" << '\n';

    // Example 1: Basic logging with wrapper function
    cout << "\nExample 1: Basic logging" << '\n';
    cout << "----------------------------" << '\n';
    digraphx::log_with_spdlog("Application started");
    digraphx::log_with_spdlog("Initializing graph...");

    // Example 2: Logging with negative cycle detection
    cout << "\nExample 2: Logging with negative cycle detection" << '\n';
    cout << "------------------------------------------------" << '\n';

    using DiGraph = list<pair<size_t, list<pair<size_t, double>>>>;

    // Create a simple graph
    DiGraph digraph{
        {0, {{1, 7.0}, {2, 5.0}}}, {1, {{0, 0.0}, {2, 3.0}}}, {2, {{1, 1.0}, {0, 2.0}, {0, 1.0}}}};

    cout << "Created graph with " << digraph.size() << " nodes" << '\n';
    digraphx::log_with_spdlog("Created graph with positive weights");

    // Perform negative cycle detection
    cout << "Running negative cycle detection..." << '\n';
    NegCycleFinder ncf(digraph);
    auto get_weight = [](const auto& edge) -> double { return edge; };
    auto dist = vector<double>(digraph.size(), 0.0);

    size_t cycle_count = 0;
    for ([[maybe_unused]] auto const& ci : ncf.howard(dist, std::move(get_weight))) {
        ++cycle_count;
    }

    cout << "Negative cycles found: " << cycle_count << '\n';
    digraphx::log_with_spdlog("Negative cycle detection completed");

    // Example 3: Using direct spdlog for advanced features
    cout << "\nExample 3: Direct spdlog usage" << '\n';
    cout << "---------------------------------" << '\n';

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
        cout << "Direct spdlog test completed" << '\n';
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Direct spdlog error: " << ex.what() << '\n';
    }

    // Example 4: Multiple log levels
    cout << "\nExample 4: Testing different log levels" << '\n';
    cout << "-----------------------------------------" << '\n';

    digraphx::log_with_spdlog("Final message - application completed successfully");

    cout << "\n========================================" << '\n';
    cout << "Summary" << '\n';
    cout << "========================================" << '\n';
    cout << "Check the following log files:" << '\n';
    cout << "  - digraphx.log (wrapper function)" << '\n';
    cout << "  - example.log (direct spdlog)" << '\n';
    cout << "\nAll examples completed successfully!" << '\n';
    cout << "========================================" << '\n';

    return 0;
}
