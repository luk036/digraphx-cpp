set_languages("c++20")
set_policy("build.optimization.lto", true)

add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("doctest", "fmt", "nanobench", "spdlog", "cxxopts", "abseil")

add_includedirs("include", "../py2cpp/include", "../mywheel-cpp/include")

if is_mode("release") then
    set_optimize("fastest")
end

if is_mode("coverage") then
    add_cxflags("-ftest-coverage", "-fprofile-arcs", {force = true})
end

if is_plat("linux") then
    set_warnings("all", "error")
    -- Termux/Android ships its libc++ headers under $PREFIX
    local prefix = os.getenv("PREFIX")
    if prefix then
        add_cxflags("-Wno-unused-command-line-argument", {force = true})
        add_sysincludedirs(prefix .. "/include/c++/v1", prefix .. "/include", {public = true})
    end
elseif is_plat("windows") then
    add_cxflags("/EHsc /utf-8 /W4 /WX /wd4702", {force = true})
end

target("DiGraphX")
    set_kind("static")
    add_files("source/*.cpp")
    add_packages("fmt", "spdlog", "abseil")

target("test_digraphx")
    add_deps("DiGraphX")
    add_files("test/source/*.cpp")
    add_packages("doctest", "fmt", "spdlog", "abseil")
    add_tests("default")

    -- rapidcheck is optionally provided by a CMake build
    local rapidcheck_dir = path.join(os.projectdir(), "build", "_deps", "rapidcheck-src")
    local rapidcheck_lib_dir = path.join(os.projectdir(), "build", "_deps", "rapidcheck-build")
    local rapidcheck_lib = nil

    if is_plat("windows") then
        rapidcheck_lib_dir = path.join(rapidcheck_lib_dir, "Release")
        rapidcheck_lib = path.join(rapidcheck_lib_dir, "rapidcheck.lib")
    else
        rapidcheck_lib = path.join(rapidcheck_lib_dir, "librapidcheck.a")
    end

    if os.isdir(rapidcheck_dir) and os.isfile(rapidcheck_lib) then
        add_includedirs(path.join(rapidcheck_dir, "include"))
        add_linkdirs(rapidcheck_lib_dir)
        add_links("rapidcheck")
        add_defines("RAPIDCHECK_H")
    end

target("standalone")
    add_deps("DiGraphX")
    add_files("standalone/source/*.cpp")
    add_packages("fmt", "spdlog", "cxxopts", "abseil")

target("BM_mcf")
    add_deps("DiGraphX")
    add_files("bench/BM_mcf.cpp")
    add_packages("fmt", "abseil", "nanobench")

target("BM_neg_cycle")
    add_deps("DiGraphX")
    add_files("bench/BM_neg_cycle.cpp")
    add_packages("fmt", "abseil", "nanobench")

target("BM_neg_cycle_cache")
    add_deps("DiGraphX")
    add_files("bench/BM_neg_cycle_cache.cpp")
    add_packages("fmt", "abseil", "nanobench")
