#include <doctest/doctest.h>
#include <digraphx/greeter.h>
// #include <digraphx/version.h>

#include <string>

TEST_CASE("DiGraphX") {
    using namespace digraphx;

    DiGraphX digraphx("Tests");

    CHECK(digraphx.greet(LanguageCode::EN) == "Hello, Tests!");
    CHECK(digraphx.greet(LanguageCode::DE) == "Hallo Tests!");
    CHECK(digraphx.greet(LanguageCode::ES) == "¡Hola Tests!");
    CHECK(digraphx.greet(LanguageCode::FR) == "Bonjour Tests!");
}

// TEST_CASE("DiGraphX version") {
//     static_assert(std::string_view(DIGRAPHX_VERSION) == std::string_view("1.0"));
//     CHECK(std::string(DIGRAPHX_VERSION) == std::string("1.0"));
// }
