# Adding Abseil (absl::flat_hash_map) to digraphx-cpp

This document explains how to configure both build systems (xmake and CMake)
to use Google Abseil's `absl::flat_hash_map`, including the pitfalls
encountered and their solutions.

## Source Changes

Unlike lds-gen-cpp where `std::unordered_map` was used only in a `.cpp` file,
digraphx-cpp uses it in **public headers**, so the Abseil types become part of
the public API.

### Headers

| File | Changes |
|---|---|
| `include/digraphx/neg_cycle.hpp` | `#include <unordered_map>` → `#include <absl/container/flat_hash_map.h>` |
| | `std::unordered_map<Node, ...>` → `absl::flat_hash_map<Node, ...>` (2 occurrences) |
| `include/digraphx/neg_cycle_q.hpp` | Same include + type replacements (5 occurrences) |

### Test files (10 files)

Each had `using std::unordered_map;` changed to `using absl::flat_hash_map;`,
and all bare `unordered_map` references replaced throughout.

---

## xmake Configuration

### Changes to `xmake.lua`

**1. Declare the package** (at the top, with other requires):

```lua
add_requires("abseil", {alias = "abseil"})
```

**2. Use the package** on every target that includes the changed headers
(since abseil types are in public headers, ANY target including
`neg_cycle.hpp` or `neg_cycle_q.hpp` needs the package):

```lua
target("DiGraphX")
    -- ...
    add_packages("fmt", "spdlog", "abseil")

target("test_digraphx")
    -- ...
    add_packages("doctest", "fmt", "spdlog", "abseil")

target("standalone")
    -- ...
    add_packages("fmt", "spdlog", "cxxopts", "abseil")
```

### Build & Test

```bash
xmake -y -j 10         # configure + build
xmake test             # run tests
```

---

## CMake (CPM.cmake) Configuration

### Overview

Six files were touched:

| File | Role |
|---|---|
| `cmake/specific.cmake` | Fetch abseil-cpp via CPM, set up variables |
| `CMakeLists.txt` (root) | Add include dir + MSVC suppressions |
| `test/CMakeLists.txt` | Link abseil targets + warning suppressions |
| `standalone/CMakeLists.txt` | Link abseil targets to standalone executable |

### 1. `cmake/specific.cmake` — Dependency Declaration

```cmake
# C++ standard MUST be set at project level before abseil-cpp's CMake runs.
# Abseil performs compile checks for C++17/20 support via CMAKE_CXX_STANDARD.
set(CMAKE_CXX_STANDARD 20)

# Fetch abseil-cpp (LTS release, Jan 2026)
CPMAddPackage(
  NAME abseil-cpp
  GIT_TAG 20260107.1
  GITHUB_REPOSITORY abseil/abseil-cpp
  OPTIONS "ABSL_PROPAGATE_CXX_STD ON"
)

# After CPMAddPackage, ${abseil-cpp_SOURCE_DIR} is defined.
list(APPEND SPECIFIC_INCLUDES "${abseil-cpp_SOURCE_DIR}")

# Abseil CMake targets to link to *executables* (not the library).
set(SPECIFIC_ABSEIL_LIBS absl::flat_hash_map)

# Regular libs — NOT abseil, to avoid export-set conflicts.
set(SPECIFIC_LIBS Threads::Threads MyWheel::MyWheel Py2Cpp::Py2Cpp fmt::fmt spdlog::spdlog)
```

### 2. Root `CMakeLists.txt` — Library Target

```cmake
# After the target is created and linked:

# Link regular libs — no abseil here.
target_link_libraries(${PROJECT_NAME} PRIVATE ${SPECIFIC_LIBS})

# Add abseil include dir PRIVATE.
target_include_directories(${PROJECT_NAME} PRIVATE ${SPECIFIC_INCLUDES})

# Strict compiler warnings (PRIVATE — does NOT propagate).
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(DiGraphX PRIVATE -Wall -Wpedantic -Wextra -Werror)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(DiGraphX PRIVATE -Wall -Wpedantic -Wextra -Werror)
elseif(MSVC)
  target_compile_options(DiGraphX PRIVATE /utf-8 /W4 /WX /wd4702 /wd4100)
endif()
```

### 3. `test/CMakeLists.txt` — Test Executable

```cmake
target_link_libraries(${PROJECT_NAME}
  doctest::doctest
  DiGraphX::DiGraphX
  ${SPECIFIC_LIBS}
  ${SPECIFIC_ABSEIL_LIBS}     # <-- abseil linked HERE
)

# Strict flags + Abseil warning suppressions on the test target.
# Order matters: strict first, then suppress (last flag wins on GCC/Clang).
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wno-nullability-extension -Wno-gcc-compat)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wno-pedantic -Wno-overflow)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W4 /WX /wd4702 /wd4100)
endif()
```

### 4. `standalone/CMakeLists.txt` — Standalone Executable

```cmake
target_link_libraries(${PROJECT_NAME}
  DiGraphX::DiGraphX
  cxxopts::cxxopts
  ${SPECIFIC_LIBS}
  ${SPECIFIC_ABSEIL_LIBS}     # <-- abseil linked HERE
)
```

### Build & Test

```bash
# Use a CPM source cache to avoid repeated network downloads
export CPM_SOURCE_CACHE="$HOME/.cache/CPM"

cmake -S all -B build
cmake --build build --config Debug -j8
ctest --test-dir build -C Debug --output-on-failure
```

---

## Pitfalls & Rationale

### Pitfall 1: `CMAKE_CXX_STANDARD` must be set at project level

Abseil's CMake (`AbseilDll.cmake`) checks `CMAKE_CXX_STANDARD >= 17` at
**configure time**.  The project originally set `CXX_STANDARD 20` only at the
target level (`set_target_properties`), which is invisible to abseil-cpp's
CMake.  Without `set(CMAKE_CXX_STANDARD 20)` in `specific.cmake`, configure
fails with:

```
The compiler defaults to or is configured for C++ < 17. C++ >= 17 is required.
```

### Pitfall 2: `absl::flat_hash_map` is NOT purely header-only

While `flat_hash_map` is mostly implemented in headers, its template
instantiation pulls in non-inline symbols from compiled abseil libraries:

- `absl::raw_log_internal::RawLog`
- `absl::container_internal::PrepareInsertSmallNonSoo`
- `absl::container_internal::IterateOverFullSlots`
- `absl::container_internal::AllocateBackingArray`
- etc.

These live in `.lib`/`.a` files (`absl_raw_hash_set`, `absl_hash`,
`absl_base`, etc.).  Simply adding the include directory is not enough — the
final executable must **link** against `absl::flat_hash_map` (which
transitively pulls in all required abseil static libraries).

### Pitfall 3: Cannot link abseil targets to the library target (export set)

The root `CMakeLists.txt` uses `packageProject()` from
PackageProject.cmake, which sets up a CMake install/export set.  If the
library target declares a `target_link_libraries(... absl::flat_hash_map)`,
CMake's `install(EXPORT)` tries to include `absl_flat_hash_map` in the export
set.  Since abseil-cpp is not installed alongside the project, this fails:

```
install(EXPORT "DiGraphXTargets" ...) includes target "DiGraphX" which
requires target "absl_flat_hash_map" that is not in any export set.
```

**Solution**: Keep abseil out of the library's `target_link_libraries`.
Instead:

- Add the abseil **include directory** to the library target PRIVATE
  (so source files can `#include <absl/container/flat_hash_map.h>`).
- Link `absl::flat_hash_map` only to the **executable** targets
  (test runner, standalone binary) where the symbols need to be resolved.

This is safe because:
- The library is a **static library** — it does not resolve symbols
  at build time; only the final executable does.
- The abseil headers are still available for compilation via the PRIVATE
  include directory.
- The export set remains clean — the installed target has no abseil
  dependency recorded.

### Pitfall 4: MSVC warnings-as-errors from Abseil headers

Abseil's own headers emit **C4702** (unreachable code) and **C4100**
(unreferenced formal parameter) on MSVC.  Since the project enforces `/WX`
(all warnings as errors), these must be suppressed:

```cmake
# On the DiGraphX library target (root CMakeLists.txt)
elseif(MSVC)
  target_compile_options(DiGraphX PRIVATE /utf-8 /W4 /WX /wd4702 /wd4100)

# On the test target (test/CMakeLists.txt)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W4 /WX /wd4702 /wd4100)
```

### Pitfall 5: GCC/Clang warnings-as-errors from Abseil headers (flag ordering)

Abseil's own headers emit:

| Platform | Warning | Cause |
|---|---|---|
| GCC (Ubuntu) | `-Wpedantic` | `__int128` is a GCC extension |
| GCC (Ubuntu) | `-Woverflow` | `_mm_set1_epi8(0x80)` char overflow |
| Clang (macOS) | `-Wnullability-extension` | `_Nonnull` is a Clang extension |
| Clang (macOS) | `-Wgcc-compat` | `enable_if` is a Clang extension |

The project enforces `-Wall -Wpedantic -Wextra -Werror` for code quality.
These flags must be set on the **test executable target** (not propagated
PUBLIC from the library) because of a **flag ordering** requirement.

**Critical**: `-Wno-pedantic` must appear **after** `-Wpedantic` on the
compiler command line.  If `-Wpedantic` comes from PUBLIC propagation, it is
appended AFTER the target's own PRIVATE flags, re-enabling the warning.

```cmake
# WRONG: PUBLIC propagation puts -Wpedantic AFTER -Wno-pedantic
target_compile_options(DiGraphX PUBLIC -Wall -Wpedantic -Wextra -Werror)
target_compile_options(DiGraphXTests PRIVATE -Wno-pedantic)
# Result: ... -Wno-pedantic ... -Wall -Wpedantic ... ← pedantic re-enabled!

# CORRECT: Set both on the same target, suppressions last
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wpedantic -Wextra -Werror)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-pedantic -Wno-overflow)
# Result: ... -Wall -Wpedantic ... -Wno-pedantic -Wno-overflow ← pedantic suppressed!
```

The same pattern applies for MSVC suppressions (`/wd` flags are order-
independent, but keeping them together is cleaner).

### Pitfall 6: Instable network / CPM downloads

GitHub clones can fail with `RPC failed; curl 56 schannel` or
`early EOF`.  Mitigations:

```bash
# Set CPM_SOURCE_CACHE to avoid re-downloading on clean builds
export CPM_SOURCE_CACHE="$HOME/.cache/CPM"

# Or on Windows (PowerShell)
$env:CPM_SOURCE_CACHE = "$HOME\.cache\CPM"
```
