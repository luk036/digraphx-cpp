# Adding Abseil (absl::flat_hash_map) to lds-gen-cpp

This document explains how to configure both build systems (xmake and CMake)
to use Google Abseil's `absl::flat_hash_map`, including the pitfalls
encountered and their solutions.

## Source Change

In `source/sphere_n.cpp`:

```cpp
// Before:
#include <unordered_map>
// ...
static std::unordered_map<unsigned int, std::vector<double>> tp_cache;

// After:
#include <absl/container/flat_hash_map.h>
// ...
static absl::flat_hash_map<unsigned int, std::vector<double>> tp_cache;
```

The header `sphere_n.hpp` does **not** expose any Abseil types, so the
include stays in the `.cpp` only.

---

## xmake Configuration

### Changes to `xmake.lua`

**1. Declare the package** (at the top, with other requires):

```lua
add_requires("abseil", {alias = "abseil"})
```

**2. Use the package** on the `LdsGen` target:

```lua
target("LdsGen")
    -- ...
    add_packages("fmt", "spdlog", "abseil")
```

**3. Suppress MSVC warnings** from Abseil headers (Windows only):

```lua
    if is_plat("windows") then
        add_cxflags("/wd4702", {force = true})
    end
```

### Why `/wd4702`?

The project compiles with `/WX` (warnings-as-errors) on MSVC.  Abseil's own
headers (`container_memory.h`, `raw_hash_set.h`) emit **C4702 (unreachable
code)**.  Without suppressing this, the build breaks.  The flag is scoped to
the `LdsGen` target so it does not affect other targets.

### Build & Test

```bash
xmake -y -j 10         # configure + build
xmake test             # run tests
```

---

## CMake (CPM.cmake) Configuration

### Overview

Four files were touched:

| File | Role |
|---|---|
| `cmake/specific.cmake` | Fetch abseil-cpp via CPM, set up variables |
| `CMakeLists.txt` (root) | Add include dir + MSVC suppressions to LdsGen |
| `test/CMakeLists.txt` | Link abseil targets to test executable |
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

# Regular libs (fmt, spdlog) — NOT abseil, to avoid export-set conflicts.
set(SPECIFIC_LIBS fmt::fmt spdlog::spdlog)
```

### 2. Root `CMakeLists.txt` — Library Target

```cmake
# After the target is created and linked:

# Link regular libs (fmt, spdlog) — no abseil here.
target_link_libraries(${PROJECT_NAME} PRIVATE ${SPECIFIC_LIBS})

# Add abseil include dir PRIVATE — the library sources can #include
# <absl/...> but dependents don't need it.
target_include_directories(${PROJECT_NAME} PRIVATE ${SPECIFIC_INCLUDES})

# Suppress MSVC warnings from Abseil headers
target_compile_options(${PROJECT_NAME} PRIVATE
  "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd4702;/wd4100>")
```

### 3. `test/CMakeLists.txt` — Test Executable

```cmake
target_link_libraries(${PROJECT_NAME}
  doctest::doctest
  LdsGen::LdsGen
  ${SPECIFIC_LIBS}
  ${SPECIFIC_ABSEIL_LIBS}     # <-- abseil linked HERE
)
```

### 4. `standalone/CMakeLists.txt` — Standalone Executable

```cmake
target_link_libraries(${PROJECT_NAME}
  LdsGen::LdsGen
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

These live in `.lib` files (`absl_raw_hash_set`, `absl_hash`, `absl_base`,
etc.).  Simply adding the include directory is not enough — the final
executable must **link** against `absl::flat_hash_map` (which transitively
pulls in all required abseil static libraries).

### Pitfall 3: Cannot link abseil targets to the LdsGen *library* target

The root `CMakeLists.txt` uses `packageProject()` from
PackageProject.cmake, which sets up a CMake install/export set.  If LdsGen
declares a `target_link_libraries(... absl::flat_hash_map)`, CMake's
`install(EXPORT)` tries to include `absl_flat_hash_map` in the export set.
Since abseil-cpp is not installed alongside LdsGen, this fails:

```
install(EXPORT "LdsGenTargets" ...) includes target "LdsGen" which
requires target "absl_flat_hash_map" that is not in any export set.
```

**Solution**: Keep abseil out of the library's `target_link_libraries`.
Instead:

- Add the abseil **include directory** to the library target PRIVATE
  (so `sphere_n.cpp` can `#include <absl/container/flat_hash_map.h>`).
- Link `absl::flat_hash_map` only to the **executable** targets
  (test runner, standalone binary) where the symbols need to be resolved.

This is safe because:
- The LdsGen library is a **static library** — it does not resolve symbols
  at build time; only the final executable does.
- The abseil headers are still available for compilation via the PRIVATE
  include directory.
- The export set remains clean — LdsGen's installed target has no abseil
  dependency recorded.

### Pitfall 4: MSVC warns-as-errors from Abseil headers

Abseil's own headers emit C4702 (unreachable code) and C4100 (unreferenced
formal parameter) on MSVC.  Since the project enforces `/WX` (all warnings
as errors), these must be suppressed:

```cmake
# Root CMakeLists.txt (PRIVATE to the LdsGen target)
target_compile_options(${PROJECT_NAME} PRIVATE
  "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd4702;/wd4100>")

# xmake.lua (on the LdsGen target)
add_cxflags("/wd4702", {force = true})
```

### Pitfall 5: Instable network / CPM downloads

GitHub clones can fail with `RPC failed; curl 56 schannel` or
`early EOF`.  Mitigations:

```bash
# Set CPM_SOURCE_CACHE to avoid re-downloading on clean builds
export CPM_SOURCE_CACHE="$HOME/.cache/CPM"

# Or on Windows (PowerShell)
$env:CPM_SOURCE_CACHE = "$HOME\.cache\CPM"
```
