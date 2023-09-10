set_languages("c++20")

add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("doctest", {alias = "doctest"})
add_requires("fmt 7.1.3", {alias = "fmt"})
add_requires("benchmark", {alias = "benchmark"})

if is_mode("coverage") then
    add_cxflags("-ftest-coverage", "-fprofile-arcs", {force = true})
end

if is_plat("linux") then
    set_warnings("all", "error")
    add_cxflags("-Wconversion", {force = true})
elseif is_plat("windows") then
    add_cxflags("/W4 /WX /wd4819 /wd4127", {force = true})
end

target("DiGraphX")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_includedirs("../py2cpp/include", {public = true})
    add_files("source/*.cpp")
    add_packages("fmt")

target("test_digraphx")
    set_kind("binary")
    add_deps("DiGraphX")
    add_files("test/source/*.cpp")
    add_packages("doctest", "fmt")

-- target("test_ell")
--     set_kind("binary")
--     add_deps("EcGen")
--     add_includedirs("include", {public = true})
--     add_files("bench/BM_ell.cpp")
--     add_packages("range-v3")
--     add_packages("benchmark")

-- target("test_lmi")
--     set_kind("binary")
--     add_deps("EcGen")
--     add_includedirs("include", {public = true})
--     add_files("bench/BM_lmi.cpp")
--     add_packages("range-v3")
--     add_packages("benchmark")


-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

