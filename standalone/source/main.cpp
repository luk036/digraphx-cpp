#include <iostream>
#include <string>
#include <vector>
#include <numeric>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// #include <cxxopts.hpp> // Commented out for clang-tidy - will be available after CPM fetch

auto main(int /*argc*/, const char* const* /*argv*/) -> int {
    try {
        /*
        cxxopts::Options options("DiGraphX", "A C++ library for graph algorithms, with a focus on directed graphs (digraphs).");

        options.add_options()
            ("h,help", "Print usage")
            ("v,version", "Print version")
            ("m,message", "Message to log", cxxopts::value<std::string>()->default_value("Hello, spdlog!"))
        ;
        */

        // Temporarily hardcoded for clang-tidy
        // When building with CMake, cxxopts will be available
        std::string message = "Hello, spdlog!"; // Default message
        
        // Setup spdlog
        // Console logger
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        // File logger
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("digraphx_standalone.log", true);
        file_sink->set_level(spdlog::level::trace);

        spdlog::logger logger("digraphx_logger", {console_sink, file_sink});
        logger.set_level(spdlog::level::trace);
        spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));

        // Log a message from command line or default
        spdlog::info("Application started.");
        spdlog::info("Log message: {}", message);
        spdlog::warn("This is a warning message.");
        spdlog::error("This is an error message.");

        // Example of using other includes
        std::vector<int> numbers(5);
        std::iota(numbers.begin(), numbers.end(), 1); // Fill with 1, 2, 3, 4, 5
        double sum = std::accumulate(numbers.begin(), numbers.end(), 0.0);
        spdlog::debug("Sum of numbers: {}", sum); // debug level will only show in file log by default

    } 
    // catch (const cxxopts::exceptions::exception& e) {
    //     spdlog::error("Error parsing options: {}", e.what());
    //     return 1;
    // } 
    catch (const std::exception& e) {
        spdlog::error("An unexpected error occurred: {}", e.what());
        return 1;
    }

    spdlog::info("Application finished.");
    return 0;
}