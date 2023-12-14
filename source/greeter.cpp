#include <digraphx/greeter.h>
#include <fmt/format.h>

using namespace digraphx;

DiGraphX::DiGraphX(std::string _name) : name(std::move(_name)) {}

/**
 * The `greet` function returns a greeting message based on the provided language code.
 *
 * @param[in] lang The `lang` parameter is of type `LanguageCode`. It is an enumeration that
 * represents different language codes. The `LanguageCode` enumeration is likely defined somewhere
 * in the codebase and contains values such as `EN` (English), `DE` (German), `ES` (Spanish), and
 *
 * @return a string that is formatted based on the given language code. The specific greeting will
 * vary depending on the language code provided.
 */
std::string DiGraphX::greet(LanguageCode lang) const {
    switch (lang) {
        default:
        case LanguageCode::EN:
            return fmt::format("Hello, {}!", name);
        case LanguageCode::DE:
            return fmt::format("Hallo {}!", name);
        case LanguageCode::ES:
            return fmt::format("¡Hola {}!", name);
        case LanguageCode::FR:
            return fmt::format("Bonjour {}!", name);
    }
}
