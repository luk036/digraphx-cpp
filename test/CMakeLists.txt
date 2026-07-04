cmake_minimum_required(VERSION 3.14...3.22)

project(DiGraphXTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
include(../cmake/specific.cmake)

CPMAddPackage("gh:doctest/doctest@2.5.2")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")
CPMAddPackage(
  NAME rapidcheck
  GITHUB_REPOSITORY emil-e/rapidcheck
  GIT_TAG master
  OPTIONS "RC_BUILD_TESTS OFF" "RC_BUILD_EXAMPLES OFF" "RC_INSTALL OFF"
)

if(TEST_INSTALLED_VERSION)
  find_package(DiGraphX REQUIRED)
else()
  CPMAddPackage(NAME DiGraphX SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} doctest::doctest DiGraphX::DiGraphX ${SPECIFIC_LIBS} ${SPECIFIC_ABSEIL_LIBS})

# Add RapidCheck support if available
if(rapidcheck_ADDED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE RAPIDCHECK_H)
  if(TARGET rapidcheck::rapidcheck)
    target_link_libraries(${PROJECT_NAME} rapidcheck::rapidcheck)
  else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${rapidcheck_SOURCE_DIR}/include)
    if(TARGET rapidcheck)
      target_link_libraries(${PROJECT_NAME} rapidcheck)
    endif()
  endif()
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# enable compiler warnings (PRIVATE on DiGraphX, so they don't propagate)
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(DiGraphX PRIVATE -Wall -Wpedantic -Wextra -Werror)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(DiGraphX PRIVATE -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(DiGraphX PRIVATE /utf-8 /W4 /WX)
  endif()
  # Apply strict flags + Abseil warning suppressions on the test target
  # Order: strict first, then suppress (last flag wins on GCC/Clang)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-nullability-extension -Wno-gcc-compat)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-pedantic -Wno-overflow)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W4 /WX /wd4702 /wd4100)
  endif()
endif()

# ---- Add DiGraphXTests ----

enable_testing()

# Add a single test that runs all tests (both doctest and RapidCheck) Note: We disable doctest
# auto-discovery to avoid conflicts with RapidCheck
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(DiGraphX PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(DiGraphX PUBLIC -fprofile-arcs -ftest-coverage)
endif()
