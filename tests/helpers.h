#pragma once

#include <hse/graph.h>
#include <hse/simulator.h>

namespace hse {

graph parse_hse_string(const std::string &hse_str);
void print_enabled(const graph &g, const simulator &sim);

} // namespace hse
