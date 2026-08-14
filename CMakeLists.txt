cmake_minimum_required(VERSION 3.14...3.22)

project(
  DiGraphX
  VERSION 1.1.7
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---------------------------------------------------------------------------
# Compiler warnings (as errors)
# ---------------------------------------------------------------------------

function(target_enable_warnings TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4 /WX /utf-8 /wd4702 /wd4100)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

# Abseil headers emit warnings that -Werror escalates to errors (e.g. MSVC C4100 from prefetch.h,
# GCC -Woverflow from hashtable_control_bytes.h). Suppress on targets that include abseil headers.
# GCC coroutine frames also mis-link at -O2 (pre-existing test issue), so the tests build at -O0 to
# match the original CI behavior.
function(target_enable_abseil_suppressions TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PRIVATE /wd4100 /wd4324)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${TARGET} PRIVATE -Wno-overflow -Wno-pedantic -O0)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
      target_compile_options(${TARGET} PRIVATE -Wno-nullability-extension -Wno-gcc-compat)
    endif()
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(DIGRAPHX_BUILD_TESTS "Build tests" ON)
option(DIGRAPHX_BUILD_STANDALONE "Build standalone executable" ON)
option(DIGRAPHX_BUILD_DOCS "Build Doxygen documentation" OFF)
option(DIGRAPHX_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DIGRAPHX_ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)
option(DIGRAPHX_ENABLE_COVERAGE "Enable coverage reporting" OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# Download CPM.cmake for reproducible dependency management see
# https://github.com/cpm-cmake/CPM.cmake for more info
set(CPM_DOWNLOAD_VERSION 0.38.5)
set(CPM_HASH_SUM "192aa0ccdc57dfe75bd9e4b176bf7fb5692fd2b3e3f7b09c74856fc39572b31c")
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
  )
endif()
include(${CPM_DOWNLOAD_LOCATION})

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# fmt is a transitive dependency of mywheel/py2cpp; prefer a system-installed fmt when available.
find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
  message(STATUS "Found system fmt: ${fmt_DIR}")
  set(CPM_fmt_ADDED YES)
else()
  CPMAddPackage(
    NAME fmt
    GIT_TAG 12.1.0
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS "FMT_INSTALL YES" # create an installable target
  )
endif()

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG v1.6.4
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY YES" "PY2CPP_BUILD_TESTS OFF"
)

CPMAddPackage(
  NAME MyWheel
  GIT_TAG v1.1.6
  GITHUB_REPOSITORY luk036/mywheel-cpp
  OPTIONS "INSTALL_ONLY YES" "MYWHEEL_BUILD_TESTS OFF"
)

# spdlog is used by the compiled sources; prefer a system-installed spdlog when available.
find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
  message(STATUS "Found system spdlog: ${spdlog_DIR}")
  set(CPM_spdlog_ADDED YES)
else()
  CPMAddPackage(
    NAME spdlog
    GIT_TAG v1.17.0
    GITHUB_REPOSITORY gabime/spdlog
    OPTIONS "SPDLOG_INSTALL YES" "SPDLOG_FMT_EXTERNAL YES" # use external fmt to avoid duplicate
            # symbols
  )
endif()

# abseil requires C++ >= 17 at its own configure time. check_cxx_source_compiles does not read
# CMAKE_CXX_STANDARD, so the standard flag must be in CMAKE_CXX_FLAGS (cache) to propagate into the
# FetchContent subdirectory's compile checks.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} /std:c++20"
      CACHE STRING "" FORCE
  )
else()
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -std=c++20"
      CACHE STRING "" FORCE
  )
endif()

# abseil provides flat_hash_map used in the headers; prefer a system-installed abseil when
# available.
find_package(absl CONFIG QUIET)
if(absl_FOUND)
  message(STATUS "Found system abseil: ${absl_DIR}")
  set(SPECIFIC_ABSEIL_LIBS absl::flat_hash_map)
else()
  CPMAddPackage(
    NAME abseil-cpp
    GIT_TAG 20260107.1
    GITHUB_REPOSITORY abseil/abseil-cpp
    OPTIONS "ABSL_PROPAGATE_CXX_STD ON"
  )
  set(SPECIFIC_ABSEIL_LIBS absl::flat_hash_map)
endif()

set(SPECIFIC_LIBS Threads::Threads MyWheel::MyWheel Py2Cpp::Py2Cpp fmt::fmt spdlog::spdlog)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(
  ${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
                         "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd4702>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${SPECIFIC_LIBS})

# Any TU using spdlog must compile with spdlog's own interface definitions (SPDLOG_COMPILED_LIB,
# SPDLOG_FMT_EXTERNAL) or the header-only and compiled registries diverge.
get_target_property(SPDLOG_COMPILE_DEFS spdlog::spdlog INTERFACE_COMPILE_DEFINITIONS)
if(SPDLOG_COMPILE_DEFS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ${SPDLOG_COMPILE_DEFS})
endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_enable_warnings(${PROJECT_NAME})

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "fmt 12.1.0" "Py2Cpp 1.6.4" "MyWheel 1.1.6"
)

# ---------------------------------------------------------------------------
# Standalone executable
# ---------------------------------------------------------------------------

if(DIGRAPHX_BUILD_STANDALONE)
  file(GLOB DIGRAPHX_STANDALONE_SOURCES CONFIGURE_DEPENDS "standalone/source/*.cpp")
  add_executable(DiGraphXStandalone ${DIGRAPHX_STANDALONE_SOURCES})
  set_target_properties(DiGraphXStandalone PROPERTIES OUTPUT_NAME "DiGraphX" CXX_STANDARD 20)
  target_link_libraries(DiGraphXStandalone PRIVATE DiGraphX::DiGraphX ${SPECIFIC_LIBS})
  target_enable_warnings(DiGraphXStandalone)
  target_enable_abseil_suppressions(DiGraphXStandalone)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

if(DIGRAPHX_BUILD_TESTS)
  # Try system-installed doctest first (Ubuntu: doctest-dev, macOS: brew install doctest, Termux:
  # doctest)
  find_package(doctest CONFIG QUIET)

  if(NOT doctest_FOUND)
    CPMAddPackage(
      NAME doctest
      GITHUB_REPOSITORY doctest/doctest
      VERSION 2.5.2
      OPTIONS "DOCTEST_NO_INSTALL ON" # keep the installed prefix minimal
    )
  endif()

  # Code formatting targets (format / fix-format) via Format.cmake
  CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")

  # Property-based testing (tests only)
  CPMAddPackage(
    NAME rapidcheck
    GITHUB_REPOSITORY emil-e/rapidcheck
    GIT_TAG master
    OPTIONS "RC_BUILD_TESTS OFF" "RC_BUILD_EXAMPLES OFF" "RC_INSTALL OFF"
  )

  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "test/source/*.cpp")
  add_executable(DiGraphXTests ${TEST_SOURCES})
  target_link_libraries(
    DiGraphXTests PRIVATE doctest::doctest DiGraphX::DiGraphX ${SPECIFIC_LIBS}
                          ${SPECIFIC_ABSEIL_LIBS} rapidcheck
  )
  target_include_directories(DiGraphXTests PRIVATE ${rapidcheck_SOURCE_DIR}/include)
  target_compile_definitions(DiGraphXTests PRIVATE RAPIDCHECK_H)
  set_target_properties(DiGraphXTests PROPERTIES CXX_STANDARD 20)
  target_enable_warnings(DiGraphXTests)
  target_enable_abseil_suppressions(DiGraphXTests)
  add_test(NAME DiGraphXTests COMMAND DiGraphXTests)
endif()

# ---------------------------------------------------------------------------
# Benchmarks (nanobench, header-only)
# ---------------------------------------------------------------------------

if(DIGRAPHX_BUILD_BENCHMARKS)
  # nanobench is a header-only benchmarking library
  set(NANOBENCH_VERSION 4.3.11)
  set(NANOBENCH_DIR "${CMAKE_BINARY_DIR}/_deps/nanobench")
  set(NANOBENCH_HEADER "${NANOBENCH_DIR}/nanobench.h")

  if(NOT EXISTS "${NANOBENCH_HEADER}")
    file(MAKE_DIRECTORY "${NANOBENCH_DIR}")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/martinus/nanobench/v${NANOBENCH_VERSION}/src/include/nanobench.h"
      "${NANOBENCH_HEADER}"
      STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
      message(FATAL_ERROR "Failed to download nanobench (status ${status_code})")
    endif()
  endif()

  add_library(nanobench INTERFACE)
  target_include_directories(nanobench SYSTEM INTERFACE "${NANOBENCH_DIR}")

  file(GLOB_RECURSE ALL_BENCH_CPP CONFIGURE_DEPENDS "bench/*.cpp")
  foreach(ONE_BENCH_CPP ${ALL_BENCH_CPP})
    get_filename_component(ONE_BENCH_EXEC ${ONE_BENCH_CPP} NAME_WE)
    set(TARGET_NAME Bench_${ONE_BENCH_EXEC})

    add_executable(${TARGET_NAME} ${ONE_BENCH_CPP})
    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${ONE_BENCH_EXEC} CXX_STANDARD 20)
    target_link_libraries(
      ${TARGET_NAME} PRIVATE nanobench DiGraphX::DiGraphX ${SPECIFIC_LIBS} ${SPECIFIC_ABSEIL_LIBS}
    )
    target_enable_warnings(${TARGET_NAME})
    target_enable_abseil_suppressions(${TARGET_NAME})
    add_test(NAME ${ONE_BENCH_EXEC} COMMAND ${TARGET_NAME})
  endforeach()
endif()

# ---------------------------------------------------------------------------
# clang-tidy
# ---------------------------------------------------------------------------

if(DIGRAPHX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy)
  if(CLANG_TIDY)
    add_custom_target(
      clang-tidy
      COMMAND ${CLANG_TIDY} --extra-arg=-std=c++20 --extra-arg=-I${CMAKE_CURRENT_SOURCE_DIR}/include
              ${CMAKE_CURRENT_SOURCE_DIR}/include/digraphx/*.hpp
      COMMENT "Running clang-tidy static analysis"
    )
  else()
    message(WARNING "clang-tidy not found - analysis target unavailable")
  endif()
endif()

# ---------------------------------------------------------------------------
# Doxygen documentation
# ---------------------------------------------------------------------------

if(DIGRAPHX_BUILD_DOCS)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
    set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
    set(DOXYGEN_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

    configure_file(documentation/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    add_custom_target(
      GenerateDocs
      COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
      COMMAND echo "Docs written to: ${DOXYGEN_OUTPUT_DIRECTORY}/html"
      COMMENT "Generating Doxygen HTML documentation in ${DOXYGEN_OUTPUT_DIRECTORY}/html"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
  else()
    add_custom_target(
      GenerateDocs COMMAND ${CMAKE_COMMAND} -E echo
                           "Doxygen not found - skipping documentation generation"
    )
  endif()
endif()

# ---------------------------------------------------------------------------
# Coverage report target (GCC/Clang only - requires gcovr)
# ---------------------------------------------------------------------------

if(DIGRAPHX_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  foreach(COVERAGE_TARGET DiGraphX DiGraphXTests DiGraphXStandalone)
    if(TARGET ${COVERAGE_TARGET})
      target_compile_options(${COVERAGE_TARGET} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
    endif()
  endforeach()
  foreach(COVERAGE_EXEC DiGraphXTests DiGraphXStandalone)
    if(TARGET ${COVERAGE_EXEC})
      target_link_options(${COVERAGE_EXEC} PRIVATE -fprofile-arcs -ftest-coverage)
    endif()
  endforeach()

  find_program(GCOVR_PATH gcovr)
  if(GCOVR_PATH)
    set(extra_gcovr_args "")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
      find_program(LLVM_COV_PATH llvm-cov)
      if(LLVM_COV_PATH)
        list(APPEND extra_gcovr_args --llvm-cov-binary ${LLVM_COV_PATH})
      endif()
    endif()
    add_custom_target(
      coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND
        ${GCOVR_PATH} --root ${PROJECT_SOURCE_DIR} --html-details
        ${CMAKE_BINARY_DIR}/coverage/index.html --exclude test/ --exclude build/ ${extra_gcovr_args}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating coverage report in ${CMAKE_BINARY_DIR}/coverage/index.html"
    )
  else()
    message(
      WARNING "gcovr not found - coverage target unavailable. Install with: pip install gcovr"
    )
  endif()
endif()
