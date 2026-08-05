#include <digraphx/mcf.hpp>

#include <iostream>

auto main() -> int {
    MCFGraph g;
    MCFDemands d;
    g[0][1] = MCFEdge{2, 1};
    d[0] = -1;
    d[1] = 1;
    auto result = cycle_canceling_mcf(g, d);
    const auto ok = result.has_value() && (result->second.size() == 1);

    std::cout << "digraphx installed test: mcf flow edges=" << result->second.size() << "\n";
    return ok ? 0 : 1;
}
