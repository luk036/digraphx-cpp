set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG v1.6.3
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY YES" # create an installable target
)

CPMAddPackage(
  NAME MyWheel
  GIT_TAG v1.1.5
  GITHUB_REPOSITORY luk036/mywheel-cpp
  OPTIONS "INSTALL_ONLY YES" # create an installable target
)

CPMAddPackage(
  NAME spdlog
  GIT_TAG v1.17.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS "SPDLOG_INSTALL YES" # create an installable target
)

# Set C++ standard at project level (abseil requires this at configure time)
set(CMAKE_CXX_STANDARD 20)

# Try system-installed abseil first (Ubuntu: libabsl-dev, macOS: brew install abseil,
# Termux: pkg install abseil-cpp). Falls back to CPM build on Windows or when no system
# package is available.
find_package(absl CONFIG QUIET)

if(absl_FOUND)
  message(STATUS "Found system abseil: ${absl_DIR}")
  # Propagate include path from the imported target (needed for library compilation)
  get_target_property(_absl_inc absl::flat_hash_map INTERFACE_INCLUDE_DIRECTORIES)
  if(_absl_inc)
    list(APPEND SPECIFIC_INCLUDES "${_absl_inc}")
  endif()
  set(SPECIFIC_ABSEIL_LIBS absl::flat_hash_map)
else()
  # Add abseil for flat_hash_map (used in neg_cycle.hpp / neg_cycle_q.hpp)
  CPMAddPackage(
    NAME abseil-cpp
    GIT_TAG 20260107.1
    GITHUB_REPOSITORY abseil/abseil-cpp
    OPTIONS "ABSL_PROPAGATE_CXX_STD ON"
  )
  # flat_hash_map needs abseil include dirs (headers) plus abseil libs at link time
  list(APPEND SPECIFIC_INCLUDES "${abseil-cpp_SOURCE_DIR}")
  # Abseil targets to link to executables (not the library, to avoid export set conflicts)
  set(SPECIFIC_ABSEIL_LIBS absl::flat_hash_map)
endif()
set(SPECIFIC_LIBS Threads::Threads MyWheel::MyWheel Py2Cpp::Py2Cpp fmt::fmt spdlog::spdlog)
