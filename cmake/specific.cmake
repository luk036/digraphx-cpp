set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Try system-installed fmt first (Ubuntu: libfmt-dev, macOS: brew install fmt, Termux: fmt)
find_package(fmt CONFIG QUIET)

if(fmt_FOUND)
  message(STATUS "Found system fmt: ${fmt_DIR}")
  # Tell CPM that fmt is already handled (CPM checks CPM_PACKAGES list). Write the CACHE variable
  # directly: list(APPEND ...) creates a normal-variable shadow that does not propagate into
  # FetchContent subdirectory scopes.
  if(NOT fmt IN_LIST CPM_PACKAGES)
    set(CPM_PACKAGES
        "${CPM_PACKAGES};fmt"
        CACHE INTERNAL "" FORCE
    )
  endif()
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

# Try system-installed spdlog first (Ubuntu: libspdlog-dev, macOS: brew install spdlog, Termux:
# spdlog)
find_package(spdlog CONFIG QUIET)

if(spdlog_FOUND)
  message(STATUS "Found system spdlog: ${spdlog_DIR}")
  # Tell CPM that spdlog is already handled (write CACHE directly, see fmt above)
  if(NOT spdlog IN_LIST CPM_PACKAGES)
    set(CPM_PACKAGES
        "${CPM_PACKAGES};spdlog"
        CACHE INTERNAL "" FORCE
    )
  endif()
else()
  CPMAddPackage(
    NAME spdlog
    GIT_TAG v1.17.0
    GITHUB_REPOSITORY gabime/spdlog
    OPTIONS "SPDLOG_INSTALL YES" "SPDLOG_FMT_EXTERNAL YES" # create an installable target
  )
endif()

# Set C++ standard at project level (abseil requires this at configure time)
set(CMAKE_CXX_STANDARD 20)

# Try system-installed abseil first (Ubuntu: libabsl-dev, macOS: brew install abseil, Termux: pkg
# install abseil-cpp). Falls back to CPM build on Windows or when no system package is available.
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
